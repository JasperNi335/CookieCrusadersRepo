#ifndef AUDIO_H
#define AUDIO_H
#include <stdint.h>

typedef struct {
    uint32_t seq;
    uint32_t sample_rate;
    uint16_t n;
    int16_t  samples[];
} __attribute__((packed)) audio_packet_t;

#define AUDIO_HEADER_BYTES (sizeof(uint32_t)*2 + sizeof(uint16_t))

void audio_init(void);
void audio_start(void);
const uint8_t* audio_try_acquire_packet(uint16_t* out_len);
void audio_release_packet(const uint8_t* ptr);

#endif