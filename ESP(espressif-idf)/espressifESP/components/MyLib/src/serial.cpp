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

uint8_t* wav_buffer = NULL;
size_t wav_size = 0;

void receive_wav_task(void* pvParameters) {
    wav_buffer = (uint8_t*)malloc(MAX_FILE_SIZE);
    if (!wav_buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "Start receiving WAV in chunks...");

    wav_size = 0;
    uint8_t chunk[PICO_CHUNK];

    while (1) {
        int received = 0;
        // Read full chunk
        while (received < PICO_CHUNK) {
            int len = uart_read_bytes(UART_PORT_NUM, chunk + received, PICO_CHUNK - received, pdMS_TO_TICKS(500));
            if (len > 0) received += len;
        }

        // Copy to buffer
        if (wav_size + PICO_CHUNK > MAX_FILE_SIZE) {
            ESP_LOGW(TAG, "Buffer full, stopping reception");
            break;
        }
        memcpy(wav_buffer + wav_size, chunk, PICO_CHUNK);
        wav_size += PICO_CHUNK;

        ESP_LOGI(TAG, "Received chunk, total %d bytes", wav_size);

        // Send ACK
        uart_write_bytes(UART_PORT_NUM, "OK", 2);

        // Optional: stop when you reach the known file size
        // if (wav_size >= FILE_SIZE) break;
    }

    ESP_LOGI(TAG, "Reception finished, WAV size: %d bytes", wav_size);

    // TODO: send wav_buffer to server

    free(wav_buffer);
    wav_buffer = NULL;
    vTaskDelete(NULL);
}