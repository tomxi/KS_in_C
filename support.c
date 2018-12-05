#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   //pow()
#include <ncurses.h>
#include "synth.h"
#include "support.h"

static void shift_keys(Tone *p)
{
    /* loop to shift tone list down by one place */
    /* key list is Tone struct array */
    for (int i=0; i<KEYS_VOICED-1; i++) {
        p[i] = p[i+1];
    }
    /* top list entry is initialized to inactive */
    p[KEYS_VOICED - 1].key = -1;
}

void add_key(Buf *pb)
{
    /* shift key list down and add key */
    double fs, f0;
    Tone *pt = pb->tone;

    if (pb->num_keys >= KEYS_VOICED) {
        /* at max, so shift tone array */
        shift_keys(pb->tone);
        /* set number of keys to max */
        pb->num_keys = KEYS_VOICED;
    }
    else {
        /* increment number of keys */
        pb->num_keys++;
    }

    /* add new key information */
    fs = pb->samp_rate;
    f0 = pb->freq[pb->this_key];
    init_tone(&pt[pb->num_keys-1], pb->this_key, fs/f0);
}

void init_tone(Tone* pt, int this_key, double string_len) {
    pt->key = this_key;
    pt->z = 0;
    pt->string_len = string_len;
    pt->buffer_len = (int) round(pt->string_len) + 1; // +1 to enable fractional delay
    pt->ring_buffer = (double *) malloc(pt->buffer_len * sizeof(double));
    for (int i = 0; i < pt->buffer_len; i++) {
        pt->ring_buffer[i] = ((double) rand() / (RAND_MAX)) - 0.5;
    }

    pt->write_index = pt->buffer_len - 1;
    pt->read_index = pt->string_len;

    pt->attack_factor = ATTACK_FACTOR;
    pt->decay_factor = pow(DECAY_FACTOR, pt->string_len);
    pt->attack_amp = 1.0;
}

void rm_key(Buf *pb)
{
    /* remove oldest key by shifting key list */
    double* temp_buf_ptr;
    if (pb->num_keys > 0) {
        temp_buf_ptr = pb->tone[0].ring_buffer;
        shift_keys(pb->tone);
        free(temp_buf_ptr);
        pb->num_keys--;
    }
}

double tic(Tone *pt) {
    double out_sample;

    out_sample = fractional_buffer_read(pt);

    // Low pass filter and attenuation
    double filtered = 0.5 * out_sample + 0.5 * pt->z;
    pt->z = out_sample;
    pt->ring_buffer[pt->write_index] = filtered * pt->decay_factor;

    // update index
    if (pt->write_index == 0) {
        pt->write_index = pt->buffer_len - 1;
    } else {
        pt->write_index--;
    }
    if (pt->read_index < 1) {
        pt->read_index = pt->read_index + pt->buffer_len - 1;
    } else {
        pt->read_index -= 1;
    }
    return out_sample;
}

double fractional_buffer_read(Tone *pt) {
    int tapout_whole = (int) floor(pt->read_index);
    double tapout_frac = pt->read_index - tapout_whole;
    double out_sample;

    out_sample = (1-tapout_frac) * pt->ring_buffer[tapout_whole] +
                 tapout_frac * pt->ring_buffer[(tapout_whole + 1) % pt->buffer_len];
    return out_sample;
}

void init_key_index(int *key2index)
{
    /* Middle three octaves of piano keyboard
     *
     * qwertyQWERTY -> C3 to B3
     * asdfghASDFGH -> C4 to B4
     * zxcvbnZXCVBN -> C5 to B5
     *
     * all other keys are unrecognized
     */
    int i;
    for (i=0; i<128; i++) {
        /* there are 128 possible ASCII characters
         * array index values are ASCII value
         * initialize to make all entries invalid (-1)
         */
        key2index[i] = -1;
    }
    /* overwrite valid keys with a key index that is >= 0 */
    i = 0;
    key2index['1'] = i++;
    key2index['2'] = i++;
    key2index['3'] = i++;
    key2index['4'] = i++;
    key2index['5'] = i++;
    key2index['6'] = i++;

    key2index['7'] = i++;
    key2index['8'] = i++;
    key2index['9'] = i++;
    key2index['0'] = i++;
    key2index['-'] = i++;
    key2index['='] = i++;

    key2index['q'] = i++;
    key2index['w'] = i++;
    key2index['e'] = i++;
    key2index['r'] = i++;
    key2index['t'] = i++;
    key2index['y'] = i++;

    key2index['u'] = i++;
    key2index['i'] = i++;
    key2index['o'] = i++;
    key2index['p'] = i++;
    key2index['['] = i++;
    key2index[']'] = i++;

    key2index['a'] = i++;
    key2index['s'] = i++;
    key2index['d'] = i++;
    key2index['f'] = i++;
    key2index['g'] = i++;
    key2index['h'] = i++;

    key2index['j'] = i++;
    key2index['k'] = i++;
    key2index['l'] = i++;
    key2index[';'] = i++;
    key2index['\''] = i++;
    key2index['\n'] = i;
}