#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "audio.h"
#include "config.h"

#ifndef LED_PIN
#define LED_PIN 25
#endif

// ---------- PWM tone helpers ----------
static void pwm_tone_start(uint gpio, float freq_hz) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    uint chan  = pwm_gpio_to_channel(gpio);

    const float sys_clk = 125000000.0f; // 125 MHz
    uint16_t TOP = 999;
    float div = sys_clk / (freq_hz * (TOP + 1));
    if (div < 1.0f) {
        div = 1.0f;
        uint32_t calc_top = (uint32_t)((sys_clk / (div * freq_hz)) - 1);
        TOP = (calc_top > 0xFFFF) ? 0xFFFF : (uint16_t)calc_top;
    }
    if (div > 255.0f) div = 255.0f;

    pwm_set_clkdiv(slice, div);
    pwm_set_wrap(slice, TOP);
    pwm_set_chan_level(slice, chan, TOP / 2);
    pwm_set_enabled(slice, true);
}
static void pwm_tone_stop(uint gpio) {
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_enabled(slice, false);
}
static void play_melody(uint gpio) {
    static const float pat[][3] = {
        {1000,2000,3000},{1500,1000,2500},{2000,1200,3000},{3000,2000,1000}
    };
    int c = rand() % (sizeof(pat)/sizeof(pat[0]));
    for (int i=0;i<3;i++){
        pwm_tone_start(gpio, pat[c][i]);
        sleep_ms(BEEP_TONE_MS);
        pwm_tone_stop(gpio);
        if (i!=2) sleep_ms(BEEP_GAP_MS);
    }
}

// ---------- USB write helper ----------
static inline void usb_write_all(const void* data, size_t len) {
    // stdout is USB CDC because of pico_enable_stdio_usb(...)
    // Disable buffering so this is immediate
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

int main() {
    stdio_init_all();
    // make USB CDC unbuffered, no CRLF translation
    setvbuf(stdout, NULL, _IONBF, 0);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);

    audio_init();
    audio_start();

    absolute_time_t next_allowed = 0;

    // Tiny line buffer for host commands (ASCII lines)
    char cmd[16]; int cmd_len = 0;

    while (true) {
        // 1) STREAM AUDIO FRAMES OVER USB (binary)
        uint16_t pay_len = 0;
        const uint8_t* payload = audio_try_acquire_packet(&pay_len);
        if (payload) {
            uint8_t hdr[6];
            uint32_t magic = FRAMING_MAGIC;
            uint16_t len   = (uint16_t)pay_len;
            memcpy(&hdr[0], &magic, 4);
            memcpy(&hdr[4], &len,   2);
            usb_write_all(hdr, sizeof(hdr));
            usb_write_all(payload, pay_len);
            audio_release_packet(payload);
        }

        // 2) POLL FOR HOST COMMANDS (ASCII, e.g., "BEEP\n")
        int ch = getchar_timeout_us(0);
        if (ch >= 0) {
            if (ch == '\r') continue;
            if (ch == '\n') {
                cmd[cmd_len] = 0;
                if (strcmp(cmd, "BEEP") == 0) {
                    if (to_ms_since_boot(get_absolute_time()) >
                        to_ms_since_boot(next_allowed)) {
                        play_melody(SPEAKER_PIN);
                        next_allowed = delayed_by_ms(get_absolute_time(), COOLDOWN_MS);
                    }
                }
                cmd_len = 0;
            } else if (cmd_len < (int)sizeof(cmd)-1) {
                cmd[cmd_len++] = (char)ch;
            } else {
                cmd_len = 0; // overflow -> reset
            }
        }

        // blink heartbeat very slowly off the hot path
        static uint32_t hb = 0;
        if ((++hb & 0x3FFFF) == 0) gpio_put(LED_PIN, !gpio_get(LED_PIN));
    }
    return 0;
}