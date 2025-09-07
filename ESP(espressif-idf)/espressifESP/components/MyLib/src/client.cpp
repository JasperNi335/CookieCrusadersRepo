#include "client.h"

static const char* TAG = "CLIENT";

ssize_t send_all(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const uint8_t *p = (const uint8_t *)buf;
    while (total < len) {
        ssize_t sent = send(sock, p + total, len - total, 0);
        if (sent <= 0) return -1; // error
        total += sent;
    }
    return total;
}

void stream_task(void *pvParameters) {
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while (true) {
        // Create socket
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            ESP_LOGE("STREAM", "Unable to create socket");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // Connect to server
        if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            ESP_LOGE("STREAM", "Socket connection failed, retrying...");
            close(sock);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        ESP_LOGI("STREAM", "Connected to server %s:%d", SERVER_IP, SERVER_PORT);

        // Streaming loop
        while (true) {
            camera_fb_t *fb = cameraCapturePhoto();
            if (!fb) continue;

            uint32_t len = fb->len;
            if (send_all(sock, &len, sizeof(len)) < 0 ||
                send_all(sock, fb->buf, fb->len) < 0) {
                ESP_LOGW("STREAM", "Lost connection to server");
                esp_camera_fb_return(fb);
                close(sock);
                break; // reconnect loop
            }

            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(30)); // ~33 FPS
        }
    }
}
