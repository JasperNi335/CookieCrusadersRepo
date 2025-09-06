#include "camera.h"

#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27
#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

static const char* TAG = "CAMERA";

camera_config_t camera_config = {
    .pin_pwdn       = PWDN_GPIO_NUM,
    .pin_reset      = RESET_GPIO_NUM,
    .pin_xclk       = XCLK_GPIO_NUM,
    .pin_sccb_sda   = SIOD_GPIO_NUM,
    .pin_sccb_scl   = SIOC_GPIO_NUM,
  
    .pin_d7         = Y9_GPIO_NUM,
    .pin_d6         = Y8_GPIO_NUM,
    .pin_d5         = Y7_GPIO_NUM,
    .pin_d4         = Y6_GPIO_NUM,
    .pin_d3         = Y5_GPIO_NUM,
    .pin_d2         = Y4_GPIO_NUM,
    .pin_d1         = Y3_GPIO_NUM,
    .pin_d0         = Y2_GPIO_NUM,
    .pin_vsync      = VSYNC_GPIO_NUM,
    .pin_href       = HREF_GPIO_NUM,
    .pin_pclk       = PCLK_GPIO_NUM,
  
    .xclk_freq_hz   = 20000000,
    .ledc_timer     = LEDC_TIMER_0,
    .ledc_channel   = LEDC_CHANNEL_0,
    .pixel_format   = PIXFORMAT_JPEG,
    .frame_size     = FRAMESIZE_QVGA,
    .jpeg_quality   = 5,
    .fb_count       = 1,

    .fb_location    = CAMERA_FB_IN_PSRAM,
    .grab_mode      = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port  = 0  // usually 0 on ESP32
};

/*-----------------------------------------
Default Camera methods
-----------------------------------------*/

bool initCamera(){
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed\nError code: 0x%x (%s)", err, esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "Camera Init Successful\n");

    // take a few starting photos to fix green tint
    for (int i = 0; i < 10; i++){
        camera_fb_t* fb = cameraCapturePhoto();

        if (fb) esp_camera_fb_return(fb);
    }
    return true;
}

void setCameraSettings(){
    sensor_t * s = esp_camera_sensor_get();
    if (!s){
        ESP_LOGE(TAG, "Camera not Found\n");
        return;
    }

    // Brightness, contrast, saturation
    s->set_brightness(s, 0);    // neutral
    s->set_contrast(s, 0);      // slightly higher contrast to separate skin from background
    s->set_saturation(s, 0);    // slightly saturated to make skin colors pop

    // Disable automatic exposure & gain
    s->set_exposure_ctrl(s, 0); // disable auto exposure
    s->set_aec2(s, 0);          // disable AEC
    s->set_agc_gain(s, 0);      // fixed gain
    s->set_gain_ctrl(s, 0);     // disable auto gain
    s->set_aec_value(s, 200);

    // Optional: enable manual white balance to stabilize colors
    s->set_whitebal(s, 1);      // disable auto white balance

    // Lens correction & mirroring as needed
    s->set_lenc(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);

    ESP_LOGI(TAG, "Camera Settings applied\n");
}

// Helper: get pixel RGB from RGB565
static inline void rgb565_to_rgb(uint16_t px, uint8_t* r, uint8_t* g, uint8_t* b){
    *r = ((px >> 11) & 0x1F) << 3;
    *g = ((px >> 5) & 0x3F) << 2;
    *b = (px & 0x1F) << 3;
}

// Helper: convert RGB back to RGB565
static inline uint16_t rgb_to_rgb565(uint8_t r, uint8_t g, uint8_t b){
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

#define KUWAHARA_WINDOW 5 

void kuwahara_filter(camera_fb_t* fb){
    if(!fb) return;

    int width = fb->width;
    int height = fb->height;
    uint16_t* img = (uint16_t*)fb->buf;

    uint16_t* tmp_buf = (uint16_t*)malloc(width * height * sizeof(uint16_t));
    if(!tmp_buf) return;

    int w = KUWAHARA_WINDOW / 2;

    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            int r_avg[4] = {0}, g_avg[4] = {0}, b_avg[4] = {0};
            int n[4] = {0};

            // 4 quadrants
            for(int q=0;q<4;q++){
                int x_start = (q%2==0) ? x-w : x;
                int x_end   = (q%2==0) ? x : x+w;
                int y_start = (q<2) ? y-w : y;
                int y_end   = (q<2) ? y : y+w;

                for(int yy=y_start; yy<=y_end; yy++){
                    for(int xx=x_start; xx<=x_end; xx++){
                        if(xx<0 || yy<0 || xx>=width || yy>=height) continue;
                        uint8_t r,g,b;
                        rgb565_to_rgb(img[yy*width + xx], &r, &g, &b);
                        r_avg[q] += r;
                        g_avg[q] += g;
                        b_avg[q] += b;
                        n[q]++;
                    }
                }

                if(n[q]>0){
                    r_avg[q] /= n[q];
                    g_avg[q] /= n[q];
                    b_avg[q] /= n[q];
                }
            }

            // choose quadrant with smallest variance (simplified: choose quadrant with lowest sum of RGB variance)
            int best_q = 0;
            int min_var = 0x7FFFFFFF;
            for(int q=0;q<4;q++){
                int var = r_avg[q]*r_avg[q] + g_avg[q]*g_avg[q] + b_avg[q]*b_avg[q];
                if(var < min_var){
                    min_var = var;
                    best_q = q;
                }
            }

            tmp_buf[y*width + x] = rgb_to_rgb565(r_avg[best_q], g_avg[best_q], b_avg[best_q]);
        }
    }

    // copy back
    memcpy(img, tmp_buf, width*height*sizeof(uint16_t));
    free(tmp_buf);
}

void blur_rgb565(camera_fb_t* fb) {
    if(!fb || !fb->buf) return;

    int width  = fb->width;
    int height = fb->height;
    uint16_t* img = (uint16_t*)fb->buf;

    // Temporary buffer to store blurred result
    uint16_t* tmp = (uint16_t*)malloc(width * height * sizeof(uint16_t));
    if(!tmp) return;

    for(int y=0; y<height; y++){
        for(int x=0; x<width; x++){
            int r_sum=0, g_sum=0, b_sum=0, count=0;

            // 3x3 neighborhood
            for(int dy=-1; dy<=1; dy++){
                int ny = y + dy;
                if(ny < 0 || ny >= height) continue;

                for(int dx=-1; dx<=1; dx++){
                    int nx = x + dx;
                    if(nx < 0 || nx >= width) continue;

                    uint16_t px = img[ny*width + nx];
                    uint8_t r = ((px >> 11) & 0x1F) << 3;
                    uint8_t g = ((px >> 5) & 0x3F) << 2;
                    uint8_t b = (px & 0x1F) << 3;

                    r_sum += r;
                    g_sum += g;
                    b_sum += b;
                    count++;
                }
            }

            // Average and convert back to RGB565
            uint8_t r_avg = r_sum / count;
            uint8_t g_avg = g_sum / count;
            uint8_t b_avg = b_sum / count;
            tmp[y*width + x] = ((r_avg>>3)<<11) | ((g_avg>>2)<<5) | (b_avg>>3);
        }
    }

    // Copy back
    memcpy(img, tmp, width*height*sizeof(uint16_t));
    free(tmp);
}

camera_fb_t* cameraCapturePhoto(){
    camera_fb_t* fb = esp_camera_fb_get();
    // if failed to capturn image
    if (!fb){
        ESP_LOGE(TAG, "Camera Capture Failed\n");
        return nullptr;
    }
    return fb;
}

void deInitCamera(){
    esp_camera_deinit();
}

/*-----------------------------------------
Serial Camera methods
-----------------------------------------*/
/* Arduino framework legacy code
bool sendPhotoSerial(){
  // try to capture image
  camera_fb_t* fb = cameraCapturePhoto();

  // if image is sucessfully captured
  if (fb){
      uint32_t size = fb->len;

      Serial.printf("Captured image size: %zu bytes\n", size);

      // sends image in smaller packages
      Serial.println("IMAGE_START");
      Serial.write((uint8_t*)&size, sizeof(size));

      // send in chunks of 64
      size_t sent = 0;
      while(sent < size){
        size_t chunk = 64;
        if (sent + chunk > size) chunk = size - sent;

        Serial.write(fb->buf + sent, chunk);

        sent += chunk;
        delay(1);
      }

      Serial.println("IMAGE_END");

      esp_camera_fb_return(fb);
      return true;
  }
  return false;
}
*/

