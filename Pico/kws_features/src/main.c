#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "audio.h"
#include "config.h"

#ifndef LED_PIN
#define LED_PIN 25
#endif

static inline float dbfs_from_rms(float rms) {
    if (rms <= 1e-9f) return -120.0f;
    return 20.0f * log10f(rms / 32768.0f);
}

// Configure PWM for target frequency with ~50% duty
static void pwm_tone_start(uint gpio, float freq_hz) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint chan  = pwm_gpio_to_channel(gpio);

    const float sys_clk = 125000000.0f; // 125 MHz
    uint16_t TOP = 999;                 // ~10-bit resolution
    float div = sys_clk / (freq_hz * (TOP + 1));
    if (div < 1.0f) {
        div = 1.0f;
        TOP = (uint16_t)((sys_clk / (div * freq_hz)) - 1);
        if (TOP > 0xFFFF) TOP = 0xFFFF;
    }
    if (div > 255.0f) div = 255.0f;

    pwm_set_clkdiv(slice, div);
    pwm_set_wrap(slice, TOP);
    pwm_set_chan_level(slice, chan, TOP / 2); // 50%
    pwm_set_enabled(slice, true);
}

static void pwm_tone_stop(uint gpio) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(slice, false);
}

int main() {
    stdio_init_all();
    sleep_ms(1200);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);

    printf("Pico: Voice detect -> random beep on GPIO%d (PWM)\n", SPEAKER_PIN);

    audio_init();
    audio_start();

    absolute_time_t last_print = get_absolute_time();
    absolute_time_t next_allowed = 0;

    while (true) {
        uint16_t len_bytes = 0;
        const uint8_t* pkt = audio_try_acquire_packet(&len_bytes);
        if (!pkt) {
            sleep_ms(1);
            continue;
        }

        // parse header
        const uint16_t* p16 = (const uint16_t*)(pkt + (sizeof(uint32_t)*2));
        uint16_t n   = p16[0];
        const int16_t* s = (const int16_t*)(pkt + (sizeof(uint32_t)*2 + sizeof(uint16_t)));

        // compute RMS
        long long acc2 = 0;
        for (uint16_t i = 0; i < n; ++i) {
            int16_t v = s[i];
            acc2 += (long long)v * (long long)v;
        }
        float rms  = sqrtf((float)acc2 / (float)n);
        float dbfs = dbfs_from_rms(rms);

        // periodic log
        if (absolute_time_diff_us(last_print, get_absolute_time()) > 200000) {
            last_print = get_absolute_time();
            printf("rms=%.1f dBFS\n", dbfs);
        }

        // detect & beep
        if (dbfs > VOICE_THRESH_DBFS &&
            to_ms_since_boot(get_absolute_time()) > to_ms_since_boot(next_allowed)) {

            int r = rand() % 3;
            float freq = (r == 0) ? 1000.0f : (r == 1) ? 2000.0f : 3000.0f;
            printf("[VOICE] Beep %.0f Hz for %d ms\n", freq, BEEP_MS);

            pwm_tone_start(SPEAKER_PIN, freq);
            sleep_ms(BEEP_MS);
            pwm_tone_stop(SPEAKER_PIN);

            next_allowed = delayed_by_ms(get_absolute_time(), COOLDOWN_MS);
        }
    }
    return 0;
}