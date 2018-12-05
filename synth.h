#ifndef _SYNTH_H_
#define _SYNTH_H_

#define KEYS_VOICED         8   /* number of simultaneouse keys voiced */
#define SAMP_RATE           48000
#define NUM_CHAN	        2
#define FRAMES_PER_BUFFER   2048
#define FS_AMPL             0.5 /* -6 dB FS */
#define ATTACK_FACTOR       0.99800 /* attack time constant of 10 ms */
#define DECAY_FACTOR        0.99998 /* decay time constant of 1.0 sec */
//#define DROP_LEVEL          0.01  /* -40 dBFS */

/* for prev and this tones */
typedef struct {
    int key; /* index into freq array */   

    int buffer_len; /* the length of the ring_buffer, always bigger than string_len*/
    double string_len; /* the length of the string being simulated.
                        * Due to freqeuncy spacing, this can be fractional.*/
    double* ring_buffer; /* The delay line as a double array.*/
    int write_index; /* the index of the delay line where the next sample should be written */
    double read_index; /* the position of the end of the current delay line*/

    double attack_factor; /* to avoid pop at the onset */
    double decay_factor; /* proportional to tone wavelength */
    double attack_amp; /* save attack amplitude */
    double z; // state of the filter
} Tone;

/* data structure to pass to callback */
typedef struct {
    double *freq;
    int num_chan;
    int samp_rate;
    int num_keys; //number of keys voiced
    Tone tone[KEYS_VOICED];
    int this_key;
#if (DB_WAV_OUT)
    SNDFILE *sndfile; //for debug file
#endif
} Buf;

#endif 