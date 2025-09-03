#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Arduino-style setup function
void setup() {
    printf("Running setup...\n");
    // Initialize peripherals
}

// Arduino-style loop function
void loop() {
    printf("Running loop...\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // delay 1 second
}

extern "C" void app_main() {
    setup(); // call setup once

    // Run loop in its own FreeRTOS task
    xTaskCreate([](void*) {
        while (true) {
            loop();
        }
    }, "loop_task", 4096, nullptr, 5, nullptr); // Stack size defined here
}
