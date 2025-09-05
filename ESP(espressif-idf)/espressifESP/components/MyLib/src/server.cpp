#include "server.h"
#include "detection.h"
#include <math.h>

static const char* TAG = "Server";

#define BOUNDARY_LENGTH 32
#define MJPEG_HEADER_SIZE 64
#define TARGET_FPS 1000
#define FRAME_DELAY 1000 / TARGET_FPS
#define JPEG_QUALITY 10 // uint8 0 - 255

// following MIME boundary allowed characters
const char BOUNDARY_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

char PART_BOUNDARY[BOUNDARY_LENGTH + 1];
char _STREAM_CONTENT_TYPE[128];
char _STREAM_BOUNDARY[128];
char _STREAM_PART[] = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

const char* TEST_BOUNDARY = "frame";

void generateBoundary(char* buffer, size_t length){
    for (int i = 0; i < length - 1; i++){
        buffer[i] = BOUNDARY_CHARS[esp_random() % (sizeof(BOUNDARY_CHARS) - 1)];
    }
    buffer[length - 1] = '\0';
}

void generateTestBoundary(char* buffer){
    strcpy(buffer, TEST_BOUNDARY);
}

void setStreamConst(){
    generateTestBoundary(PART_BOUNDARY); // this line used for testing

    // dynamic boundaries to be implemeted later
    //generateBoundary(PART_BOUNDARY, sizeof(PART_BOUNDARY));

    ESP_LOGE(TAG, "Boundary is: %s", PART_BOUNDARY);

    snprintf(_STREAM_CONTENT_TYPE, sizeof(_STREAM_CONTENT_TYPE), "multipart/x-mixed-replace;boundary=%s", PART_BOUNDARY);
    snprintf(_STREAM_BOUNDARY, sizeof(_STREAM_BOUNDARY), "\r\n--%s\r\n", PART_BOUNDARY);
}

esp_err_t get_stream_handler(httpd_req_t* request) {
    ESP_LOGI(TAG, "Client connected");

    esp_err_t res = httpd_resp_set_type(request, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set content type: %s", esp_err_to_name(res));
        return res;
    }

    // Buffers for JPEG encoding
    static uint8_t *jpg_buffer = NULL;
    size_t jpg_len = 0;

    // Maximum faces we can handle
    int x[MAX_FACES], y[MAX_FACES], w[MAX_FACES], h[MAX_FACES];

    while(true) {
        camera_fb_t* fb = esp_camera_fb_get();
        if(!fb || !fb->buf) {
            ESP_LOGE(TAG, "Camera capture failed");
            continue;
        }

        // --- Detect skin blobs ---
        int num_faces = detect_faces(fb, x, y, w, h);

        // --- Draw rectangles on the frame ---
        draw_faces(fb, x, y, w, h, num_faces);

        // --- JPEG encode ---
        if(!frame2jpg(fb, 80, &jpg_buffer, &jpg_len)) {
            ESP_LOGE(TAG, "JPEG encoding failed");
            esp_camera_fb_return(fb);
            continue;
        }

        // --- Send MJPEG boundary ---
        res = httpd_resp_send_chunk(request, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        if(res != ESP_OK) {
            free(jpg_buffer);
            esp_camera_fb_return(fb);
            break;
        }

        // --- Send MJPEG part header ---
        char header[128];
        int header_len = snprintf(header, sizeof(header), _STREAM_PART, jpg_len);
        res = httpd_resp_send_chunk(request, header, header_len);
        if(res != ESP_OK) {
            free(jpg_buffer);
            esp_camera_fb_return(fb);
            break;
        }

        // --- Send JPEG image ---
        res = httpd_resp_send_chunk(request, (char*)jpg_buffer, jpg_len);
        free(jpg_buffer);
        if(res != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        esp_camera_fb_return(fb);
    }

    return ESP_OK;
}



httpd_uri_t uri_get = {
    .uri      = "/stream",
    .method   = HTTP_GET,
    .handler  = get_stream_handler,
    .user_ctx = NULL
};

httpd_handle_t server = NULL;

httpd_handle_t startServer(){
    setStreamConst();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK){
        ESP_LOGE(TAG, "Failed to start server, Error: %s", esp_err_to_name(err));
    }else{
        httpd_register_uri_handler(server, &uri_get);
    }
    
    return server;
}

bool getServerStatus(){
    if (server) {
        ESP_LOGE(TAG, "Server is online");
        return true;
    }
    ESP_LOGE(TAG, "Server is not online");
    return false;
}

void endServer(){
    if (server) httpd_stop(server);
} 