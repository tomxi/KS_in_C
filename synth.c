#define DB_WAV_OUT      0   /* write output to wav file for debugging */

#include <stdio.h>
#include <stdlib.h> 	/* malloc() */
#include <unistd.h>     /* sleep() */
#include <stdbool.h>	/* true, false */
#include <string.h>		/* memset() */
#include <ctype.h>		/* lolower() */
#include <math.h>		/* sin() */
#include <sndfile.h>	/* libsndfile */
#include <portaudio.h>	/* portaudio */
#include <ncurses.h> 	/* This library is for getting input without hitting return */
#include "synth.h"
#include "support.h"
#include "freq.h"

/* PortAudio callback function protoype */
static int paCallback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData );

int main(int argc, char *argv[])
{
    int ch;
    int key2index[128];
    int samp_rate, num_chan;
	/* my data structure */
	Buf buf, *pb = &buf;
	/* PortAudio data structures */
    PaStream *stream;
    PaError err;
    PaStreamParameters outputParams;
    PaStreamParameters inputParams;
#if (DB_WAV_OUT)
    /* libsndfile data structures */
    //SNDFILE *sndfile;
    SF_INFO sfinfo;

	/* zero libsndfile structures */
	memset(&sfinfo, 0, sizeof(sfinfo));
#endif

    /* initialize PortAudio parameters */
    samp_rate = SAMP_RATE;
    num_chan = NUM_CHAN;

    /* initialize key table */
    init_key_index(key2index);

    /* initialize struct parameters */
    pb->freq = freq; //pb->freq now points to array in freq.h include file
    pb->this_key = -1;
    pb->num_chan = num_chan;
    pb->samp_rate = samp_rate;
    for (int i=0; i<KEYS_VOICED; i++) {
        pb->tone[i].key = -1;
        pb->tone[i].attack_amp = 0.0;
    }

	/* Initializing PortAudio */
    err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        return -1;
    }

    /* Input stream parameters */
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency =
        Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    /* Ouput stream parameters */
    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = num_chan;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency =
        Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    /* Open audio stream */
    err = Pa_OpenStream(&stream,
        &inputParams, /* no input */
        &outputParams,
        samp_rate, FRAMES_PER_BUFFER,
        paNoFlag, /* flags */
        paCallback,
        &buf);

    if (err != paNoError) {
        printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        return -1;
    }

     /* Start audio stream */
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf(  "PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        return -1;
    }

#if (DB_WAV_OUT)
        /* open debug output file */
        memset(&sfinfo, 0, sizeof(sfinfo));
        sfinfo.samplerate = samp_rate;
    	sfinfo.channels = num_chan;
    	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        if ( (pb->sndfile = sf_open ("test_file.wav", SFM_WRITE, &sfinfo)) == NULL ) {
            fprintf (stderr, "Error: could not open test wav file\n");
            return -1;
        }
#endif

	/* Initialize ncurses
     * to permit interactive character input
     */
	initscr(); /* Start curses mode */
	cbreak();  /* Line buffering disabled */
	noecho(); /* Comment this out if you want to echo characters when typing */
    printw("Welcome to my guitar synthesizer using Karplus-Strong!\n");
    printw("Keyboard to piano key mapping is:\n");
    printw("1234567890-=    -> C3 to B3\n");
    printw("qwertyuiop[]    -> C4 to B4\n");
    printw("asdfghjkl;'<CR> -> C5 to B5\n");
    printw("Space Bar to stop playing oldest tone\n");
    printw("'.' to quit\n");
    mvprintw(8, 16, "Key: ");
	refresh();
    bool done = false;
	while (!done) {
        char msg[32];
        ch = getch(); // wait for next keypress
        /* process key press */
        /*********YOUR CODE HERE*************/
        switch (ch) {
            case '.':
                msg[0] = '\0';
                done = true;
                break;
            case ' ':
                rm_key(pb);
                strcpy(msg, "Removed Key.");
                break;
            default:
                if (key2index[ch] == -1) {
                    strcpy(msg, "Invalid Key.");
                } else {
                    pb->this_key = key2index[ch];
                    add_key(pb);
                    sprintf(msg, "Key %3d,    ", pb->this_key);
                }
        }
        move(7,0);
        printw("Polyphony: %3d. Active keys:", pb->num_keys);
        for (int i=0; i < KEYS_VOICED; i++) {
            printw(" %3d", pb->tone[i].key);
        }
        mvprintw(8, 0, msg);
        mvprintw(8, 16, "New key: ");
        refresh(); //display NCurses screen buffer on terminal screen
	}
	/* End curses mode  */
	endwin();

#if (DB_WAV_OUT)
    /* close debugging output wav file */
    sf_close (pb->sndfile);
#endif

    /* Stop stream */
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        printf(  "PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    /* Close stream */
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        printf(  "PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    /* Terminate PortAudio */
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
        printf("\nExiting.\n");
        exit(1);
    }

    return 0;
}

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may be called at interrupt level on some machines so don't do anything
 * in the routine that requires significant resources.
 */
static int paCallback(
    const void *inputBuffer, 
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    Buf *pb = (Buf *)userData; /* Cast pointer to data passed through stream */
    float *output = (float *)outputBuffer;
    int num_chan = pb->num_chan;
    Tone *pt = &pb->tone[0]; /* tone list */

    /* synthesize tones */
    for (int i=0; i<framesPerBuffer; i++) {
        /* for each frame */
        float v = 0;
        for (int n=0; n < pb->num_keys; n++) {
            v += FS_AMPL * tic(&pt[n]) * (1 - pt[n].attack_amp);
            pt[n].attack_amp *= pt[n].attack_factor; // Apply attack
        }
        for (int ch = 0; ch < num_chan; ch++) {
            output[num_chan*i + ch] = v;
        }
    }

#if (DB_WAV_OUT)
    /* write to debugging file */
    sf_writef_float (pb->sndfile, output, framesPerBuffer);
#endif
    return 0;
}

