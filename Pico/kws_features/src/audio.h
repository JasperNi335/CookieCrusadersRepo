#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t seq;         // packet sequence
    uint32_t sample_rate; // sanity
    uint16_t n;           // number of samples
    int16_t  samples[];   // PCM16, little-endian
} __attribute__((packed)) audio_packet_t;

#define AUDIO_HEADER_BYTES (sizeof(uint32_t)*2 + sizeof(uint16_t))

void audio_init(void);                 // ADC+DMA ping-pong init
void audio_start(void);                // start continuous sampling

// Try to get a ready packet (non-blocking). Returns pointer + total byte length, else NULL.
const uint8_t* audio_try_acquire_packet(uint16_t* out_len);

// Recycle a packet after sending
void audio_release_packet(const uint8_t* ptr);

#endif // AUDIO_H