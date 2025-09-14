#ifndef AUDIO_H
#define AUDIO_H

#include <stddef.h>
#include <stdint.h>

void   audio_init(void);                      // starts background ADC sampling
void   audio_stop(void);

// Copy the last N samples (oldest..newest) into dst.
// If N > ring size, it will be clamped.
void   audio_get_recent(int16_t *dst, size_t n);

// Block until N new samples have arrived, then copy the last N into dst.
size_t audio_read_block(int16_t *dst, size_t n);

// Back-compat thin wrapper around audio_read_block()
void   audio_get_frame(int16_t *dst, size_t n);

#endif // AUDIO_H