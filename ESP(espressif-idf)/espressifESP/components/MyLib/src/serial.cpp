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

/*
void serial_send_task(void *pvParameters) {
    
    uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
    vTaskDelay(pdMS_TO_TICKS(2000)); // send every 2 seconds
}

void send(const char* msg){

}*/