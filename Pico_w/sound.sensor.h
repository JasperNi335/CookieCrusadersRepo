#ifndef SOUND_SENSOR_H
#define SOUND_SENSOR_H

#include "pico/stdlib.h"

void sound_sensor_init();
int sound_sensor_sample();

#endif