#ifndef SERIAL_H
#define SERIAL_H

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

#define UART_PORT_NUM      UART_NUM_0  // UART0 for ESP32-CAM
#define UART_BAUD_RATE     115200
#define UART_BUF_SIZE      1024
#define MAX_FILE_SIZE      (64 * 1024)
#define PICO_CHUNK         256

#include "serial.h"

void serial_init();

// Task to receive data and echo it back
void serial_receive_task(void *pvParameters);

// Optional task to periodically send a message
void serial_send_task(void *pvParameters);

#endif // SERIAL_H