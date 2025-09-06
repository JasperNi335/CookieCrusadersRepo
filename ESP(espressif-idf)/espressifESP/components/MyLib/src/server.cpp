#include "server.h"

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

esp_err_t get_stream_handler(httpd_req_t* req) {
    ESP_LOGI(TAG, "GET /stream");
    esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    camera_fb_t* fb = NULL;
    size_t jpg_len = 0;
    uint8_t* jpg_buf = NULL;
    char header_buffer[MJPEG_HEADER_SIZE];   // FIXED: correct type

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) { res = ESP_FAIL; break; }

        jpg_buf = NULL;        // reset for each loop
        jpg_len = 0;

        if (fb->format == PIXFORMAT_JPEG) {
            // Use camera-owned JPEG buffer directly
            jpg_buf = fb->buf;
            jpg_len = fb->len;

            // Send boundary + header + image while holding fb
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            if (res == ESP_OK) {
                size_t hlen = snprintf(header_buffer, sizeof(header_buffer), _STREAM_PART, jpg_len);
                res = httpd_resp_send_chunk(req, header_buffer, hlen);
            }
            if (res == ESP_OK) {
                res = httpd_resp_send_chunk(req, (const char*)jpg_buf, jpg_len);
            }

            esp_camera_fb_return(fb);  // return after send
            fb = NULL;

        } else {
            // Convert to JPEG into our own malloc'd buffer
            // quality 80 is a good start; adjust to reduce size if needed
            bool ok = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
            esp_camera_fb_return(fb);  // return ASAP now that we have our own buffer
            fb = NULL;

            if (!ok || !jpg_buf) { res = ESP_FAIL; break; }

            // Send boundary + header + image (we own jpg_buf)
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            if (res == ESP_OK) {
                size_t hlen = snprintf(header_buffer, sizeof(header_buffer), _STREAM_PART, jpg_len);
                res = httpd_resp_send_chunk(req, header_buffer, hlen);
            }
            if (res == ESP_OK) {
                res = httpd_resp_send_chunk(req, (const char*)jpg_buf, jpg_len);
            }

            free(jpg_buf);      // free only our malloc'd buffer
            jpg_buf = NULL;
        }

        if (res != ESP_OK) break;

        // Gentle throttle to avoid FB-OVF on slow networks
        vTaskDelay(pdMS_TO_TICKS(20));  // try 10–50 ms
    }

    // If we bailed out mid-iteration, clean up
    if (fb) esp_camera_fb_return(fb);
    if (jpg_buf && (res != ESP_OK)) free(jpg_buf);

    return res;
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