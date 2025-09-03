#include <stdio.h>
#include <string.h>
#include "config.h"
#include "audio.h"
#include "mfcc.h"

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

int main() {
    stdio_init_all();
    sleep_ms(1000); // Give USB time to enumerate
    printf("\nPico MFCC streamer starting...\n");

    // LED for quick feedback
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, true);

    // Init audio sampler
    if (!audio_init(SAMPLE_RATE_HZ)) {
        printf("audio_init failed\n");
        while(1) tight_loop_contents();
    }

    // Init MFCC pipeline
    if (!mfcc_init(SAMPLE_RATE_HZ, FFT_SIZE, FRAME_LEN_SAMPLES, NUM_MEL_BANDS, NUM_MFCC)) {
        printf("mfcc_init failed\n");
        while(1) tight_loop_contents();
    }

    static int16_t frame[FRAME_LEN_SAMPLES];

    // Prime first frame
    audio_get_samples(frame, FRAME_LEN_SAMPLES);

    // Main loop: every hop compute MFCC
    while (true) {
        // Slide window by hop
        memmove(frame, frame + HOP_LEN_SAMPLES,
                (FRAME_LEN_SAMPLES - HOP_LEN_SAMPLES) * sizeof(int16_t));
        audio_get_samples(frame + (FRAME_LEN_SAMPLES - HOP_LEN_SAMPLES),
                          HOP_LEN_SAMPLES);

        float mfcc[NUM_MFCC];
        mfcc_compute_frame(frame, mfcc);

        // Quick-n-dirty activity indicator using C0
        bool led_on = (mfcc[0] > -2.0f); // tune this later
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);

        // Print first few MFCCs for logging
        printf("MFCC:");
        for (int i = 0; i < NUM_MFCC && i < 6; ++i) {
            printf(" %0.3f", mfcc[i]);
        }
        printf("\n");
    }
}