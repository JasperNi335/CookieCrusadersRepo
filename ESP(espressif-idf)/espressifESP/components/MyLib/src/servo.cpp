#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo.h"

static const char* TAG = "SERVO";

// Initialize LEDC for servo
void servo_init(int gpio_pin) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_16_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = SERVO_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure      = false // Explicitly initialize the deconfigure member
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = gpio_pin,
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Move servo to specific pulse width in microseconds
void servo_write_us(int us) {
    uint32_t duty = (us * 65535) / (1000000 / SERVO_FREQ); // convert to 16-bit duty
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Sweep servo back and forth
void servo_sweep() {
    const int step = 5;           // small increment in microseconds
    const int delay_ms = 50;      // delay between steps, adjust to make it slower/faster

    // Sweep from left to right
    ESP_LOGI(TAG, "Sweeping Right Slowly");
    for (int us = SERVO_MIN_US; us <= SERVO_MAX_US; us += step) {
        servo_write_us(us);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }

    // Sweep from right to left
    ESP_LOGI(TAG, "Sweeping Left Slowly");
    for (int us = SERVO_MAX_US; us >= SERVO_MIN_US; us -= step) {
        servo_write_us(us);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}


