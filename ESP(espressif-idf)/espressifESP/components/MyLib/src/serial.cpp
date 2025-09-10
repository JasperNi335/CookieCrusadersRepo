#include "serial.h"

static const char* TAG = "SERIAL";

void serial_init() {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void serial_receive_task(void *pvParameters) {
    uint8_t data[UART_BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE, pdMS_TO_TICKS(100));
        if (len > 0) {
            uart_write_bytes(UART_PORT_NUM, (const char*)data, len); // Echo back
        }
    }
}

void serial_send_task(void *pvParameters) {
    const char* msg = "Hello from ESP32!\r\n";
    while (1) {
        uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
        vTaskDelay(pdMS_TO_TICKS(2000)); // send every 2 seconds
    }
}

#define CHUNK_ACK       "OK\n"
#define CONFIRM_ACK     "CONFIRMED\n"

static uint8_t* wav_buffer = NULL; // PSRAM buffer
static size_t wav_received = 0;

void receive_wav_task(void* pvParameters) {
    ESP_LOGI(TAG, "UART receiver started");

    // Allocate PSRAM buffer
    wav_buffer = (uint8_t*) heap_caps_malloc(MAX_FILE_SIZE, MALLOC_CAP_SPIRAM);
    if (!wav_buffer) {
        ESP_LOGE(TAG, "Failed to allocate PSRAM buffer");
        vTaskDelete(NULL);
        return;
    }
    wav_received = 0;

    while (1) {
        // Peek how many bytes are available
        size_t avail = 0;
        uart_get_buffered_data_len(UART_PORT_NUM, &avail);

        if (avail == 2) {
            // Maybe EOF marker
            uint8_t eof[2];
            int len = uart_read_bytes(UART_PORT_NUM, eof, 2, pdMS_TO_TICKS(2000));
            if (len == 2 && eof[0] == 0xFF && eof[1] == 0xFF) {
                ESP_LOGI(TAG, "EOF marker received, stopping");
                break;
            } else {
                ESP_LOGW(TAG, "Invalid 2-byte marker received, aborting");
                break;
            }
        }

        // Otherwise expect a full chunk (256 + 2 marker)
        uint8_t chunk[PICO_CHUNK + 2];
        int received = 0;
        int needed = sizeof(chunk);

        while (received < needed) {
            int len = uart_read_bytes(UART_PORT_NUM,
                                      chunk + received,
                                      needed - received,
                                      pdMS_TO_TICKS(2000));
            if (len > 0) {
                received += len;
            } else {
                ESP_LOGW(TAG, "Timeout waiting for chunk");
                goto finish;
            }
        }

        // Validate trailing marker (0xAA 0x55)
        if (chunk[PICO_CHUNK] != 0xAA || chunk[PICO_CHUNK + 1] != 0x55) {
            ESP_LOGW(TAG, "Invalid chunk marker, aborting");
            goto finish;
        }

        // Copy valid audio data into PSRAM
        if (wav_received + PICO_CHUNK <= MAX_FILE_SIZE) {
            memcpy(wav_buffer + wav_received, chunk, PICO_CHUNK);
            wav_received += PICO_CHUNK;
            ESP_LOGI(TAG, "Received chunk, total %d bytes", wav_received);
        } else {
            ESP_LOGE(TAG, "Buffer overflow, aborting");
            goto finish;
        }

        // Send ACK (0xCC)
        uint8_t ack = 0xCC;
        uart_write_bytes(UART_PORT_NUM, (const char*)&ack, 1);

        // Wait for CONFIRM (0xDD)
        uint8_t confirm;
        int c = uart_read_bytes(UART_PORT_NUM, &confirm, 1, pdMS_TO_TICKS(2000));
        if (c == 1 && confirm == 0xDD) {
            ESP_LOGI(TAG, "Sender confirmed ACK");
        } else {
            ESP_LOGW(TAG, "No confirmation received, stopping");
            break;
        }
    }

finish:
    ESP_LOGI(TAG, "Reception finished, WAV size: %d bytes", wav_received);
    vTaskDelete(NULL);
}
