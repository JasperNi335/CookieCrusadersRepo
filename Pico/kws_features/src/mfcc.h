#ifndef MFCC_H
#define MFCC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool mfcc_init(int sample_rate, int fft_size, int frame_len, int num_mel, int num_mfcc);

// Compute MFCCs for one frame (length = frame_len) of PCM16.
// Writes NUM_MFCC floats into out_mfcc.
void mfcc_compute_frame(const int16_t *pcm, float *out_mfcc);

#endif // MFCC_H