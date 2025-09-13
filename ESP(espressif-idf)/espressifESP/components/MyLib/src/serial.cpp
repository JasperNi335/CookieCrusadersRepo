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

void serial_send_test(void *pvParameters) {
    const char *test_msg = "Hello UART!\n";
    uint8_t rx_buffer[128];

    while (1) {
        // Send message
        uart_write_bytes(UART_PORT_NUM, test_msg, strlen(test_msg));
        ESP_LOGI("UART_TEST", "Sent: %s", test_msg);

        // Try to read back response
        int len = uart_read_bytes(UART_PORT_NUM, rx_buffer, sizeof(rx_buffer) - 1, pdMS_TO_TICKS(1000));
        if (len > 0) {
            rx_buffer[len] = '\0';  // Null-terminate
            ESP_LOGI("UART_TEST", "Received: %s", (char*)rx_buffer);
        } else {
            ESP_LOGI("UART_TEST", "No data received");
        }

        // Repeat every 2 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void serial_send_task(void *pvParameters) {
    while (1) {
        char msg = VoiceCommand();        // e.g. 'A'
        char durationChar = DurationCommand(); // e.g. '2'

        // Convert char '2' → int 2
        int duration = durationChar - '0';
        if (duration < 0 || duration > 9) {
            duration = 0; // fallback if not a valid digit
        }

        // Send message
        uart_write_bytes(UART_PORT_NUM, &msg, 1);

        // Delay = (duration + 1) seconds
        vTaskDelay(pdMS_TO_TICKS((duration + 1) * 1000));
    }
}
