#ifndef AUDIO_H
#define AUDIO_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Initialize ADC on MIC_GPIO and start 16 kHz timer sampler.
bool audio_init(uint32_t sample_rate_hz);

// Blocking fetch of N fresh samples (PCM16 mono).
size_t audio_get_samples(int16_t *dst, size_t n);

#endif // AUDIO_H