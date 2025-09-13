#ifndef SERVO_H
#define SERVO_H

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "client.h"

/* ------------ Servo instance type ------------ */
typedef struct {
    int               gpio_pin;
    int               min_us;
    int               max_us;
    uint32_t          freq_hz;

    ledc_mode_t       speed_mode;
    ledc_timer_t      timer;
    ledc_channel_t    channel;
    ledc_timer_bit_t  duty_resolution;
} servo_t;

/* ------------ Predefined servo objects ------------ */
extern servo_t SERVO_HEAD;
extern servo_t SERVO_LEFT;
extern servo_t SERVO_RIGHT;

/* ------------ Task args type ------------ */
typedef char (*servo_cmd_fn_t)(void);

typedef struct {
    servo_t*       servo;
    int            start_deg;
    int            step_deg;
    int            period_ms;
    servo_cmd_fn_t get_cmd;
} servo_task_args_t;

/* ------------ API ------------ */
void servo_init(servo_t* s);
void servo_write_us(servo_t* s, int us);
void servo_set_angle_deg(servo_t* s, int deg);
void servo_sweep(servo_t* s, int step_us, int delay_ms);
void servo_update_task(void *pvParameters);

#endif // SERVO_H