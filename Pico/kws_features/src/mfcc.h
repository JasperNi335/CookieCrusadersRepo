#ifndef MFCC_H
#define MFCC_H

#include <stdbool.h>

void mfcc_init(int sample_rate, int num_fbank, int num_coeffs);

// Compute MFCCs for one frame of `N` samples (mono, float in [-1,1] ideally).
// `mfcc_out` must have room for `num_coeffs`.
// If `include_c0` is true, coefficient 0 (energy) is included as the first value.
void mfcc_compute(const float* frame, int N, float* mfcc_out,
                  int num_coeffs, bool include_c0);

#endif // MFCC_H