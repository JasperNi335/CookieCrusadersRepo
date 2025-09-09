#include "client.h"

static const char* TAG = "CLIENT";

void stream_task(void *pvParameters) {
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        esp_http_client_config_t config = {0};
        config.url = SERVER_URL;
        config.method = HTTP_METHOD_POST;

        esp_http_client_handle_t client = esp_http_client_init(&config);

        esp_http_client_set_post_field(client, (const char *)fb->buf, fb->len);
        esp_http_client_set_header(client, "Content-Type", "image/jpeg");

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "POST status = %d", esp_http_client_get_status_code(client));
        } else {
            ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        esp_camera_fb_return(fb);

        vTaskDelay(pdMS_TO_TICKS(100)); // ~10 FPS
    }
}
