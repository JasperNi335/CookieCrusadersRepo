#include <Arduino.h>
#include "esp_camera.h"
#include "../lib/camera.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wpa2.h" // Include WPA2 Enterprise API
#include <WebServer.h>

#define CAMERA_MODEL_AI_THINKER

// Wi-Fi credentials
const char* ssid = "MRDANS16";
//const char* username = "jni335";
const char* password = "S5712y/3";


// Brightness threshold for face-like detection
#define BRIGHTNESS_THRESHOLD 100

WebServer server(80);

void drawRect(uint8_t* buf, int width, int height, int x, int y, int w, int h, uint8_t color) {
    // Top & Bottom
    for (int i = x; i < x + w; i++) {
        if (i >= 0 && i < width && y >= 0 && y < height) buf[y*width + i] = color;
        if (i >= 0 && i < width && (y + h) >= 0 && (y + h) < height) buf[(y+h)*width + i] = color;
    }
    // Left & Right
    for (int j = y; j < y + h; j++) {
        if (x >= 0 && x < width && j >= 0 && j < height) buf[j*width + x] = color;
        if ((x + w) >= 0 && (x + w) < width && j >= 0 && j < height) buf[j*width + w] = color;
    }
}

void handleJPGStream() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        server.send(500, "text/plain", "Camera capture failed");
        return;
    }

    /* drawing first convert to diff format rgb565 raw pixle then convert back
    int width = fb->width;
    int height = fb->height;

    // Simple brightness-based detection
    int minX = width, minY = height, maxX = 0, maxY = 0;
    int count = 0;

    for (int y = 0; y < height; y += 2) {
        for (int x = 0; x < width; x += 2) {
            uint8_t pixel = fb->buf[y * width + x];
            if (pixel > BRIGHTNESS_THRESHOLD) {
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
                count++;
            }
        }
    }

    if (count > 50) {
        // Draw a white rectangle around detected area
        drawRect(fb->buf, width, height, minX, minY, maxX - minX, maxY - minY, 255);
    }
    */

    // Stream JPEG
    WiFiClient client = server.client();
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();
    client.write(fb->buf, fb->len);

    esp_camera_fb_return(fb);
}

void startCameraServer() {
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", "<html><body><img src='/stream'></body></html>");
    });

    server.on("/stream", HTTP_GET, handleJPGStream);

    server.begin();
    Serial.println("Camera server started");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Camera init (assume your camera code is correct)
    if (initCamera()) {
        setCameraSettings();
        Serial.println("Camera init Successful");
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true); // Clear old config
    delay(1000);

    Serial.printf("Connecting to Wi-Fi SSID: %s\n", ssid);
    WiFi.begin(ssid, password);

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 60) { // Wait up to ~30 seconds
        delay(500);
        attempt++;
        Serial.print(".");
        int status = WiFi.status();
        switch (status) {
            case WL_NO_SSID_AVAIL:
                Serial.print(" [SSID not found]");
                break;
            case WL_CONNECT_FAILED:
                Serial.print(" [Connection failed]");
                break;
            case WL_IDLE_STATUS:
                Serial.print(" [Idle]");
                break;
            case WL_DISCONNECTED:
                Serial.print(" [Disconnected]");
                break;
            case WL_CONNECTED:
                Serial.print(" [Connected]");
                break;
            case WL_CONNECTION_LOST:
                Serial.print(" [Connection lost]");
                break;
        }
        Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Wi-Fi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Failed to connect to Wi-Fi.");
    }

    startCameraServer();
}


void loop() {
    server.handleClient();
}
