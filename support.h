#ifndef _SUPPORT_H_
#define _SUPPORT_H_

/* other function prototypes */
void init_key_index(int *key2index);
void add_key(Buf *pb);
void rm_key(Buf *pb);
double tic(Tone *pt);
double fractional_buffer_read(Tone *pt);
void init_tone(Tone *pt, int this_key, double string_len);

#endif