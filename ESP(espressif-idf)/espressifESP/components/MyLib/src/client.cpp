#include "client.h"

static const char* TAG = "CLIENT";

char ServoCommandChar = 'M';
char VoiceChar = '0';
char DurationChar = '0';

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    char url[128];

    if (esp_http_client_get_url(evt->client, url, sizeof(url)) == ESP_OK) {
        switch (evt->event_id) {
            case HTTP_EVENT_ON_DATA:
                if (evt->data && evt->data_len > 0) {
                    const char *data = (const char*) evt->data;

                    if (strstr(url, "/ingest") != NULL) {
                        ServoCommandChar = data[0];
                        ESP_LOGI(TAG, "SERVO Received char: %c", ServoCommandChar);
                    }
                    else if (strstr(url, "/audio") != NULL) {
                        if (evt->data_len >= 2) {
                            VoiceChar    = data[0];
                            DurationChar = data[1];
                            ESP_LOGI(TAG, "AUDIO Received Voice: %c, Duration: %c",
                                     VoiceChar, DurationChar);
                        } else {
                            ESP_LOGW(TAG, "AUDIO data too short (len=%d)", evt->data_len);
                        }
                    }
                    else {
                        ESP_LOGI(TAG, "Unknown endpoint (URL: %s)", url);
                    }
                }
                break;

            default:
                // Other events ignored
                break;
        }
    } else {
        ESP_LOGE(TAG, "Failed to get URL from client");
    }

    return ESP_OK;
}

char ServoCommand(){
    return ServoCommandChar;
}

char VoiceCommand(){
    return VoiceChar;
}

char DurationCommand(){
    return DurationChar;
}

void stream_task(void *pvParameters) {
    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        esp_http_client_config_t config = {};
        config.url = SERVER_IMAGE_URL;
        config.method = HTTP_METHOD_POST;
        config.event_handler = _http_event_handler;

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

void audio_task(void *pvParameters) {
    while (true) {
        esp_http_client_config_t config = {
            .url = SERVER_AUDIO_URL,
            .method = HTTP_METHOD_GET,
            .event_handler = _http_event_handler,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "GET status = %d", status);
        } else {
            ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);

        int duration = DurationCommand() - '0';
        vTaskDelay(pdMS_TO_TICKS((duration + 1) * 1000));
    }
}
