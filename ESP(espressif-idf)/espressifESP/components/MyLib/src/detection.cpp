#include "detection.h"
#include "esp_log.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define TAG "DETECTION"

// Downscaled frame
#define DS_W 80
#define DS_H 60

// Minimum detection box (full resolution)
#define MIN_FACE_W 5
#define MIN_FACE_H 5

// Skin detection in YCrCb (tuned for avg rgb(122,117,124))
#define MIN_CR 135
#define MAX_CR 150
#define MIN_CB 120
#define MAX_CB 130
#define MIN_Y  60 
#define MAX_Y 110

// Buffers
static uint16_t small_buf[DS_W*DS_H];
static uint8_t skin_mask[DS_W*DS_H];

// Flood-fill stack (heap allocated once)
static int (*ff_stack)[2] = NULL;
static int ff_stack_size = 0;

// Allocate flood-fill stack once
static bool ensure_ff_stack(void) {
    int required = DS_W * DS_H;
    if (ff_stack && ff_stack_size >= required) return true;

    // free old if smaller
    if (ff_stack) {
        free(ff_stack);
        ff_stack = NULL;
        ff_stack_size = 0;
    }

    ff_stack = (int(*)[2])malloc(sizeof(int[DS_W*DS_H][2]));
    if (!ff_stack) {
        ESP_LOGE(TAG, "Failed to allocate flood-fill stack");
        return false;
    }
    ff_stack_size = required;
    return true;
}

// Convert RGB565 -> YCrCb
static inline void rgb565_to_ycrcb(uint16_t px, uint8_t *y, uint8_t *cr, uint8_t *cb){
    uint8_t r = ((px >> 11) & 0x1F) << 3;
    uint8_t g = ((px >> 5) & 0x3F) << 2;
    uint8_t b = (px & 0x1F) << 3;

    *y  = (uint8_t)(0.299*r + 0.587*g + 0.114*b);
    *cr = (uint8_t)(128 + 0.5*r - 0.4187*g - 0.0813*b);
    *cb = (uint8_t)(128 - 0.1687*r - 0.3313*g + 0.5*b);
}

// Check if pixel is skin
static inline int is_skin(uint16_t px){
    uint8_t y, cr, cb;
    rgb565_to_ycrcb(px, &y, &cr, &cb);

    int skin = (y >= MIN_Y && y <= MAX_Y &&
                cr >= MIN_CR && cr <= MAX_CR &&
                cb >= MIN_CB && cb <= MAX_CB);

    if(skin) {
        ESP_LOGI(TAG, "Skin pixel detected: Y=%d Cr=%d Cb=%d", y, cr, cb);
    }

    return skin;
}


// Downscale frame to DS_W*DS_H
static void downscale_frame(camera_fb_t *fb){
    int sx = fb->width / DS_W;
    int sy = fb->height / DS_H;

    for(int y=0; y<DS_H; y++){
        for(int x=0; x<DS_W; x++){
            int src_x = x * sx;
            int src_y = y * sy;
            small_buf[y*DS_W + x] = ((uint16_t*)fb->buf)[src_y*fb->width + src_x];
        }
    }
}

// Flood-fill to find connected skin regions (8-connectivity)
static int flood_fill(int x, int y, int *min_x, int *min_y, int *max_x, int *max_y){
    if (!ff_stack) return 0; // no memory allocated

    int top = 0;
    ff_stack[top][0] = x;
    ff_stack[top][1] = y;
    top++;

    *min_x = x; *min_y = y; *max_x = x; *max_y = y;
    int count = 0;

    while(top > 0){
        top--;
        int cx = ff_stack[top][0];
        int cy = ff_stack[top][1];

        if(cx<0 || cy<0 || cx>=DS_W || cy>=DS_H) continue;
        int idx = cy*DS_W + cx;
        if(!skin_mask[idx]) continue;

        skin_mask[idx] = 0;
        count++;

        if(cx < *min_x) *min_x = cx;
        if(cy < *min_y) *min_y = cy;
        if(cx > *max_x) *max_x = cx;
        if(cy > *max_y) *max_y = cy;

        // push neighbors if space
        if(top + 8 <= ff_stack_size){
            ff_stack[top][0] = cx+1; ff_stack[top][1] = cy; top++;
            ff_stack[top][0] = cx-1; ff_stack[top][1] = cy; top++;
            ff_stack[top][0] = cx;   ff_stack[top][1] = cy+1; top++;
            ff_stack[top][0] = cx;   ff_stack[top][1] = cy-1; top++;
            ff_stack[top][0] = cx+1; ff_stack[top][1] = cy+1; top++;
            ff_stack[top][0] = cx-1; ff_stack[top][1] = cy+1; top++;
            ff_stack[top][0] = cx+1; ff_stack[top][1] = cy-1; top++;
            ff_stack[top][0] = cx-1; ff_stack[top][1] = cy-1; top++;
        }
    }

    return count;
}

// Detect faces (largest blob only)
int detect_faces(camera_fb_t *fb, int *x_arr, int *y_arr, int *w_arr, int *h_arr){
    if(!fb || !x_arr || !y_arr || !w_arr || !h_arr) return 0;
    if(fb->width < DS_W || fb->height < DS_H) return 0;
    if(!ensure_ff_stack()) return 0;

    downscale_frame(fb);

    // Skin detection
    for(int i=0;i<DS_W*DS_H;i++){
        skin_mask[i] = is_skin(small_buf[i]);
    }

    int scale_x = fb->width / DS_W;
    int scale_y = fb->height / DS_H;

    // Best blob so far
    int best_area = 0;
    int best_x=0, best_y=0, best_w=0, best_h=0;

    for(int y=0;y<DS_H;y++){
        for(int x=0;x<DS_W;x++){
            if(!skin_mask[y*DS_W + x]) continue;

            int min_x, min_y, max_x, max_y;
            int count = flood_fill(x, y, &min_x, &min_y, &max_x, &max_y);

            int w_full = (max_x - min_x + 1) * scale_x;
            int h_full = (max_y - min_y + 1) * scale_y;
            int area   = w_full * h_full;

            // Only consider blobs large enough
            if(w_full >= MIN_FACE_W && h_full >= MIN_FACE_H){
                if(area > best_area){
                    best_area = area;
                    best_x = min_x * scale_x;
                    best_y = min_y * scale_y;
                    best_w = w_full;
                    best_h = h_full;
                }
            }
        }
    }

    if(best_area > 0){
        x_arr[0] = best_x;
        y_arr[0] = best_y;
        w_arr[0] = best_w;
        h_arr[0] = best_h;
        return 1; // one face (largest blob)
    }

    return 0; // none
}

// Draw detection box
void draw_faces(camera_fb_t *fb, int *x, int *y, int *w, int *h, int num_faces){
    if(!fb || num_faces <= 0 || !x || !y || !w || !h) return;
    uint16_t *img = (uint16_t*)fb->buf;
    uint16_t color = 0x07E0; // bright green

    int x0 = x[0];
    int y0 = y[0];
    int x1 = x0 + w[0];
    int y1 = y0 + h[0];

    if(x0 < 0) x0 = 0;
    if(y0 < 0) y0 = 0;
    if(x1 >= fb->width)  x1 = fb->width-1;
    if(y1 >= fb->height) y1 = fb->height-1;

    // Top & bottom
    for(int xx=x0; xx<=x1; xx++){
        img[y0*fb->width + xx] = color;
        img[y1*fb->width + xx] = color;
    }

    // Left & right
    for(int yy=y0; yy<=y1; yy++){
        img[yy*fb->width + x0] = color;
        img[yy*fb->width + x1] = color;
    }
}
