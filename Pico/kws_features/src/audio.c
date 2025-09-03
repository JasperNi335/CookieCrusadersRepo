#include "audio.h"
#include "config.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/time.h"

#define AUDIO_RING_CAP  8192   // must be power-of-two

static volatile int16_t ring[AUDIO_RING_CAP];
static volatile uint32_t widx = 0;  // write index
static volatile uint32_t ridx = 0;  // read index
static repeating_timer_t sample_timer;

static inline uint32_t ring_count(void) {
    return (widx - ridx) & (AUDIO_RING_CAP - 1);
}

static inline void ring_push(int16_t s) {
    ring[widx & (AUDIO_RING_CAP - 1)] = s;
    widx++;
}

static bool sample_timer_cb(repeating_timer_t *t) {
    (void)t;
    // Single ADC read (approx 12-bit). Center to 0 and scale to 16-bit.
    uint16_t u = adc_read();              // 0..4095
    int32_t centered = (int32_t)u - 2048; // center around 0
    int16_t s = (int16_t)(centered << 4); // scale to ~int16 range
    ring_push(s);
    return true;
}

bool audio_init(uint32_t sample_rate_hz) {
    // Configure ADC on MIC_GPIO -> ADC input channel
    adc_init();
    adc_gpio_init(MIC_GPIO);
    adc_select_input(MIC_ADC_INPUT);

    // Optional: tune ADC clock (default is fine for 16 kHz)
    // adc_set_clkdiv(...);

    // Warm-up read
    (void)adc_read();

    // Start a repeating timer to sample at desired rate (negative = periodic)
    int64_t period_us = (int64_t)1000000 / (int64_t)sample_rate_hz;
    bool ok = add_repeating_timer_us(-period_us, sample_timer_cb, NULL, &sample_timer);
    return ok;
}

size_t audio_get_samples(int16_t *dst, size_t n) {
    // Block until enough samples are available
    while (ring_count() < n) {
        tight_loop_contents();
    }
    for (size_t i = 0; i < n; ++i) {
        dst[i] = ring[ridx & (AUDIO_RING_CAP - 1)];
        ridx++;
    }
    return n;
}