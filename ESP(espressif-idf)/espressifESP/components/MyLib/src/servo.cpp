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
void servo_write_us(int us){
    const uint32_t max_duty = 65535;
    const int period_us = 1000000 / SERVO_FREQ; // 20ms
    uint32_t duty = ((uint64_t)us * max_duty) / period_us;
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Sweep servo back and forth
void servo_sweep() {
    const int step = 5;           // small increment in microseconds
    const int delay_ms = 50;      // delay between steps, adjust to make it slower/faster

    while(1){
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
}


static int servo_dir = 1;     // 1 = right, -1 = left, 0 = stopped

void servo_move(char command){
    switch(command){
        case 'M':
            servo_dir = 0; // stop
            break;
        case 'L':
            servo_dir = -1; // move left
            break;
        case 'R':
            servo_dir = 1;  // move right
            break;
        default:
            servo_dir = 0;
            break;
    }
}

void servo_update_task(void *pvParameters){
    char c = 'M';
    int servo_pos = 90;
    while(1){
        char c = ServoCommand(); // 'L', 'R', 'M'
        ESP_LOGI(TAG, "current command %c", c);
        if (c == 'L' && servo_pos > 0) servo_pos -= 1;
        else if (c == 'R' && servo_pos < 180) servo_pos += 1;
        // 'M' does nothing (hold)

        int pulse = SERVO_MIN_US + (servo_pos * (SERVO_MAX_US - SERVO_MIN_US) / 180);
        servo_write_us(pulse);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

