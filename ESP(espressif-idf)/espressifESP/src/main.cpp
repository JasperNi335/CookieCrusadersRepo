#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "camera.h"
#include "servo.h"
#include "serial.h"
#include "client.h"
#include "network.h"
#include "keys.h"
#include "esp_http_client.h"

static const char* TAG = "MAIN";

void setup() {
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_log_level_set("*", ESP_LOG_INFO);

    ESP_LOGI(TAG, "Setup started");

    ESP_LOGI(TAG, "Initializing camera...");
    if (!initCamera()) {
        ESP_LOGE(TAG, "Failed to initialize camera");
        vTaskDelay(pdMS_TO_TICKS(10000));
    } else {
        ESP_LOGI(TAG, "Camera initialized successfully");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Setting camera parameters");
    setCameraSettings();

    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Setting up WiFi");
    setupWiFi();
    
    // // Servo browns out to be fixed later.
    // servo_init(SERVO_PIN);

    //serial_init();

    ESP_LOGI(TAG, "Setup finished");
}

// ESP-IDF entry point
extern "C" void app_main() {
    setup();  // run setup once

    // Run HTTP streaming task
    xTaskCreate(stream_task, "http_stream_task", 8192, nullptr, 6, nullptr);
    xTaskCreate(audio_task, "http_audio_task", 8192, nullptr, 6, nullptr);
    //xTaskCreate(servo_update_task, "servo_movement_task", 2048, nullptr, 5, nullptr);
    //xTaskCreate([](void*) { servo_sweep(); }, "servo_task", 2048, nullptr, 5, nullptr);
    //xTaskCreate(serial_receive_task, "serial_receive_task", 4096, NULL, 10, NULL);
    //xTaskCreate(serial_send_task, "serial_send_task", 4096, NULL, 10, NULL);
    //xTaskCreate(serial_send_test, "serial_send_test", 4096, NULL, 10, NULL);

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
