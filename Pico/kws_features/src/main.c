#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>  // <-- add this

#include "audio.h"
#include "config.h"

#ifndef LED_PIN
#define LED_PIN 25  // on-board LED on Pico (not W)
#endif

static inline float dbfs_from_rms(float rms) {
    if (rms <= 1e-9f) return -120.0f;
    return 20.0f * log10f(rms / 32768.0f);
}

int main() {
    stdio_init_all();
    sleep_ms(1500);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);

    printf("Pico Audio Self-Test: DMA ADC @ %d Hz, pkt=%d samples\n",
           SAMPLE_RATE_HZ, SAMPLES_PER_PKT);

    audio_init();
    audio_start();

    enum { CAP_SECONDS = 3 };
    const uint32_t CAP_SAMPLES = SAMPLE_RATE_HZ * CAP_SECONDS;

    // 32 KB fallback buffer
    static int16_t cap_buf[16 * 1024];

    int16_t* rec = NULL;
    uint32_t rec_len = 0;
    uint32_t rec_written = 0;
    bool recording = false;

    if (CAP_SAMPLES <= (sizeof(cap_buf) / sizeof(cap_buf[0]))) {
        rec = cap_buf;
        rec_len = CAP_SAMPLES;
    } else {
        // ~96 KB for 3 s @ 16 kHz; fits in RP2040 SRAM
        rec = (int16_t*)malloc(CAP_SAMPLES * sizeof(int16_t));
        rec_len = rec ? CAP_SAMPLES : (sizeof(cap_buf)/sizeof(cap_buf[0]));
        if (!rec) rec = cap_buf;
    }

    absolute_time_t last_print = get_absolute_time();

    while (true) {
        int ch = getchar_timeout_us(0);
        if (ch == 'r' || ch == 'R') {
            recording = true;
            rec_written = 0;
            printf("\n[REC] started: target %lu samples (~%d s)\n",
                   (unsigned long)rec_len, CAP_SECONDS);
        }

        uint16_t len_bytes = 0;
        const uint8_t* pkt = audio_try_acquire_packet(&len_bytes);
        if (!pkt) {
            static uint32_t hb = 0;
            if (++hb % 50 == 0) gpio_put(LED_PIN, !gpio_get(LED_PIN));
            sleep_ms(1);
            continue;
        }

        const uint32_t* p32 = (const uint32_t*)pkt;
        const uint16_t* p16 = (const uint16_t*)(pkt + (sizeof(uint32_t)*2));
        uint32_t seq = p32[0];
        uint32_t sr  = p32[1];
        uint16_t n   = p16[0];
        const int16_t* s = (const int16_t*)(pkt + (sizeof(uint32_t)*2 + sizeof(uint16_t)));

        long long acc2 = 0, acc1 = 0;
        int16_t peak = 0; int clip = 0;
        for (uint16_t i = 0; i < n; ++i) {
            int16_t v = s[i];
            acc1 += v;
            acc2 += (long long)v * (long long)v;
            int16_t a = v >= 0 ? v : (int16_t)-v;
            if (a > peak) peak = a;
            if (a >= 32760) clip++;
        }
        float mean = (float)acc1 / (float)n;
        float rms  = sqrtf((float)acc2 / (float)n);
        float dbfs = dbfs_from_rms(rms);

        if (absolute_time_diff_us(last_print, get_absolute_time()) > 200000) {
            last_print = get_absolute_time();
            printf("seq=%lu n=%u sr=%lu  rms=%.1f dBFS  peak=%d  dc=%.1f  clips=%d\n",
                   (unsigned long)seq, n, (unsigned long)sr, dbfs, peak, mean, clip);
        }

        if (recording && rec_written < rec_len) {
            uint32_t to_copy = n;
            if (rec_written + to_copy > rec_len) to_copy = rec_len - rec_written;
            memcpy(rec + rec_written, s, to_copy * sizeof(int16_t));
            rec_written += to_copy;

            if (rec_written >= rec_len) {
                recording = false;
                printf("[REC] complete: %lu samples captured. Dumping CSV...\n",
                       (unsigned long)rec_written);
                printf("# CSV int16 little-endian sample stream, %lu Hz, %lu samples\n",
                       (unsigned long)sr, (unsigned long)rec_written);
                for (uint32_t i = 0; i < rec_written; ++i) printf("%d\n", (int)rec[i]);
                printf("# END CSV\n");
                printf("[REC] CSV dump finished. You can copy the block into a file.\n");
            }
        }
    }
    return 0;
}