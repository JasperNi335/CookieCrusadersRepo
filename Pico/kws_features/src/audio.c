#include "audio.h"
#include "config.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <string.h>

#ifndef AUDIO_RING_SIZE
#define AUDIO_RING_SIZE  (8192)   // power-of-two makes wrapping easy
#endif

static volatile uint16_t wptr = 0;                // write index (0..AUDIO_RING_SIZE-1)
static int16_t ring[AUDIO_RING_SIZE];             // 16-bit signed samples
static repeating_timer_t rt;                      // repeating timer for sampling
static volatile bool running = false;

// Timer ISR-ish callback: sample ADC at SAMPLE_RATE_HZ
static bool sample_cb(repeating_timer_t *t) {
    (void)t;
    if (!running) return true;

    // Raw 12-bit (0..4095) -> center to 0 and scale to ~int16 range
    uint16_t raw = adc_read();
    int16_t s = (int16_t)((int)raw - 2048) << 4;

    ring[wptr] = s;
    wptr = (wptr + 1) & (AUDIO_RING_SIZE - 1);
    return true;
}

void audio_init(void) {
    adc_init();
    adc_gpio_init(MIC_GPIO);
    adc_select_input(MIC_ADC_INPUT);

    running = true;

    const double us_per_sample = 1e6 / (double)SAMPLE_RATE_HZ;
    const int interval_us = (int)(us_per_sample + 0.5); // ~62 us for 16k
    add_repeating_timer_us(-interval_us, sample_cb, NULL, &rt);
}

void audio_stop(void) {
    running = false;
    cancel_repeating_timer(&rt);
}

void audio_get_recent(int16_t *dst, size_t n) {
    if (n == 0) return;
    if (n > AUDIO_RING_SIZE) n = AUDIO_RING_SIZE;

    uint16_t end   = wptr;                                   // snapshot to avoid race
    uint16_t start = (end - (uint16_t)n) & (AUDIO_RING_SIZE - 1);

    if (start + n <= AUDIO_RING_SIZE) {
        memcpy(dst, &ring[start], n * sizeof(int16_t));
    } else {
        size_t first = AUDIO_RING_SIZE - start;
        memcpy(dst, &ring[start], first * sizeof(int16_t));
        memcpy(dst + first, &ring[0], (n - first) * sizeof(int16_t));
    }
}

size_t audio_read_block(int16_t *dst, size_t n) {
    if (n == 0) return 0;
    if (n > AUDIO_RING_SIZE) n = AUDIO_RING_SIZE;

    uint16_t start_w = wptr;
    while (((uint16_t)(wptr - start_w)) < (uint16_t)n) {
        tight_loop_contents();
    }

    audio_get_recent(dst, n);
    return n;
}

void audio_get_frame(int16_t *dst, size_t n) {
    (void)audio_read_block(dst, n);
}