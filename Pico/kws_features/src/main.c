#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "audio.h"

static void uart_init_link(void) {
    uart_init(UART_INST, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_INST, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART_INST, true);
}

static void uart_write_all(const uint8_t* data, size_t len) {
    // uart_write_blocking sends all bytes in one call
    uart_write_blocking(UART_INST, data, len);
}

int main() {
    stdio_init_all();
    sleep_ms(1200);
    printf("Pico: DMA ADC -> UART @ %d Hz, pkt=%d samples, baud=%d\n",
           SAMPLE_RATE_HZ, SAMPLES_PER_PKT, UART_BAUD);

    uart_init_link();
    audio_init();
    audio_start();

    while (true) {
        uint16_t pay_len = 0;
        const uint8_t* payload = audio_try_acquire_packet(&pay_len);
        if (!payload) { 
            sleep_ms(1); 
            continue; 
        }

        // Frame: magic (4) + len (2) + payload (pay_len)
        uint8_t hdr[6];
        uint32_t magic = FRAMING_MAGIC;
        uint16_t len   = (uint16_t)pay_len;
        memcpy(&hdr[0], &magic, 4);
        memcpy(&hdr[4], &len,   2);

        uart_write_all(hdr, sizeof(hdr));
        uart_write_all(payload, pay_len);

        audio_release_packet(payload);
    }
    return 0;
}