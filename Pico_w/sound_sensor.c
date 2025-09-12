#include "sound_sensor.h"

// Define the GPIO pin where the sound sensor is connected
#define SOUND_SENSOR_PIN 26

void sound_sensor_init() {
    // Initialize GPIO pin for input (GP26)
    gpio_init(SOUND_SENSOR_PIN);
    gpio_set_dir(SOUND_SENSOR_PIN, GPIO_IN);
}

int sound_sensor_sample() {
    // Read the value from the sound sensor connected to GP26
    return gpio_get(SOUND_SENSOR_PIN);
}