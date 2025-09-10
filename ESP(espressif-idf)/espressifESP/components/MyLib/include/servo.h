#ifndef SERVO_H
#define SERVO_H

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "client.h"

#define SERVO_PIN    13
#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500
#define SERVO_FREQ   50     // 50 Hz

// Initialize LEDC for servo
void servo_init(int gpio_pin);

// Move servo to specific pulse width in microseconds
void servo_write_us(int us);

// Sweep servo back and forth
void servo_sweep();

void servo_move(char command);

void servo_update_task(void *pvParameters);

#endif // SERVO_H