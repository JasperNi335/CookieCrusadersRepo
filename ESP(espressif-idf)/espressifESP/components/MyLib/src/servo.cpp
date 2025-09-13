#include "servo.h"

static const char* TAG = "SERVO";

/* ------------ Predefined servo objects ------------ */
servo_t SERVO_HEAD = {
    .gpio_pin       = 13,
    .min_us         = 1000,
    .max_us         = 2000,
    .freq_hz        = 50,
    .speed_mode     = LEDC_HIGH_SPEED_MODE,
    .timer          = LEDC_TIMER_0,
    .channel        = LEDC_CHANNEL_0,
    .duty_resolution= LEDC_TIMER_16_BIT
};

servo_t SERVO_LEFT = {
    .gpio_pin       = 14,
    .min_us         = 500,
    .max_us         = 2500,
    .freq_hz        = 50,
    .speed_mode     = LEDC_HIGH_SPEED_MODE,
    .timer          = LEDC_TIMER_0,
    .channel        = LEDC_CHANNEL_1,
    .duty_resolution= LEDC_TIMER_16_BIT
};

servo_t SERVO_RIGHT = {
    .gpio_pin       = 15,
    .min_us         = 500,
    .max_us         = 2500,
    .freq_hz        = 50,
    .speed_mode     = LEDC_HIGH_SPEED_MODE,
    .timer          = LEDC_TIMER_0,
    .channel        = LEDC_CHANNEL_2,
    .duty_resolution= LEDC_TIMER_16_BIT
};

/* Compute duty value for a given pulse width (us) */
static inline uint32_t servo_us_to_duty(const servo_t* s, int us) {
    const uint32_t max_duty = (1u << s->duty_resolution) - 1u; // e.g., 65535 for 16-bit
    const int period_us = 1000000 / s->freq_hz;                // e.g., 20000us @ 50Hz
    if (us < 0) us = 0;
    return (uint32_t)((((uint64_t)us) * max_duty) / period_us);
}

void servo_init(servo_t* s) {
    if (!s) return;

    // Configure (or reconfigure) LEDC timer
    ledc_timer_config_t tcfg = {
        .speed_mode       = s->speed_mode,
        .duty_resolution  = s->duty_resolution,
        .timer_num        = s->timer,
        .freq_hz          = s->freq_hz,
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure = false
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    // Configure LEDC channel
    ledc_channel_config_t ccfg = {
        .gpio_num       = s->gpio_pin,
        .speed_mode     = s->speed_mode,
        .channel        = s->channel,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = s->timer,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
}

void servo_write_us(servo_t* s, int us) {
    if (!s) return;
    if (us < s->min_us) us = s->min_us;
    if (us > s->max_us) us = s->max_us;

    uint32_t duty = servo_us_to_duty(s, us);
    ESP_ERROR_CHECK(ledc_set_duty(s->speed_mode, s->channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(s->speed_mode, s->channel));
}

void servo_set_angle_deg(servo_t* s, int deg) {
    if (!s) return;
    if (deg < 0)   deg = 0;
    if (deg > 180) deg = 180;

    int pulse = s->min_us + (deg * (s->max_us - s->min_us) / 180);
    servo_write_us(s, pulse);
}

void servo_sweep(servo_t* s, int step_us, int delay_ms) {
    if (!s) return;
    if (step_us <= 0) step_us = 5;
    if (delay_ms <= 0) delay_ms = 20;

    ESP_LOGI(TAG, "Sweeping on ch%u (pin %d)", (unsigned)s->channel, s->gpio_pin);

    // Left->Right
    for (int us = s->min_us; us <= s->max_us; us += step_us) {
        servo_write_us(s, us);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    // Right->Left
    for (int us = s->max_us; us >= s->min_us; us -= step_us) {
        servo_write_us(s, us);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

/* FreeRTOS task that moves one servo based on a command source */
void servo_update_task(void *pvParameters) {
    servo_task_args_t *args = (servo_task_args_t*)pvParameters;
    if (!args || !args->servo) {
        ESP_LOGE(TAG, "servo_update_task: invalid args");
        vTaskDelete(NULL);
        return;
    }

    servo_t *s = args->servo;
    int deg = args->start_deg;
    if (deg < 0) deg = 0;
    if (deg > 180) deg = 180;

    servo_set_angle_deg(s, deg);

    const int step_deg = (args->step_deg == 0) ? 1 : args->step_deg;
    const int period_ms = (args->period_ms <= 0) ? 50 : args->period_ms;

    for (;;) {
        char cmd = 'M';
        if (args->get_cmd) {
            cmd = args->get_cmd();   // expected 'L', 'R', or 'M'
        }

        if (cmd == 'L' && deg > 0)       deg -= step_deg;
        else if (cmd == 'R' && deg < 180) deg += step_deg;

        if (deg < 0)   deg = 0;
        if (deg > 180) deg = 180;

        servo_set_angle_deg(s, deg);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}