#include "pico/stdlib.h"
#include "sound_sensor.h"

int main() {
    // Initialize the GPIO pin for the sound sensor (GP26)
    sound_sensor_init();

    while (true) {
        // Sample audio data from the sensor
        int audio_data = sound_sensor_sample();
        
        // You can add logic here to process or store the data
        printf("Audio data: %d\n", audio_data);

        // Add a small delay for sampling rate control
        sleep_ms(10); // Adjust as necessary to optimize sample rate
    }

    return 0;
}