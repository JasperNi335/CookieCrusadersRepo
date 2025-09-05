#include "detection.h"
#include "esp_log.h"
#include <string.h>
#include <stdint.h>

#define TAG "DETECTION"

// Downscaled frame
#define DS_W 80
#define DS_H 60

// Minimum detection box (full resolution)
#define MIN_FACE_W 40
#define MIN_FACE_H 40

// Skin detection in YCrCb
// Tuned for your face color (avg rgb(122,88,93))
#define MIN_CR 132
#define MAX_CR 140
#define MIN_CB 135
#define MAX_CB 145

// Buffers
static uint16_t small_buf[DS_W*DS_H];
static uint8_t skin_mask[DS_W*DS_H];

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
    return (cr >= MIN_CR && cr <= MAX_CR &&
            cb >= MIN_CB && cb <= MAX_CB);
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
    int stack[DS_W*DS_H][2];
    int top = 0;
    stack[top][0] = x; stack[top][1] = y; top++;

    *min_x = x; *min_y = y; *max_x = x; *max_y = y;
    int count = 0;

    while(top > 0){
        top--;
        int cx = stack[top][0];
        int cy = stack[top][1];

        if(cx<0 || cy<0 || cx>=DS_W || cy>=DS_H) continue;
        if(!skin_mask[cy*DS_W + cx]) continue;

        skin_mask[cy*DS_W + cx] = 0;
        count++;

        if(cx < *min_x) *min_x = cx;
        if(cy < *min_y) *min_y = cy;
        if(cx > *max_x) *max_x = cx;
        if(cy > *max_y) *max_y = cy;

        // 8-connectivity neighbors
        int nx[8] = {cx+1, cx-1, cx, cx, cx+1, cx-1, cx+1, cx-1};
        int ny[8] = {cy, cy, cy+1, cy-1, cy+1, cy+1, cy-1, cy-1};

        for(int i=0;i<8;i++){
            if(top < DS_W*DS_H){
                stack[top][0] = nx[i];
                stack[top][1] = ny[i];
                top++;
            }
        }
    }

    return count;
}

// Detect faces
int detect_faces(camera_fb_t *fb, int *x_arr, int *y_arr, int *w_arr, int *h_arr){
    if(!fb) return 0;

    downscale_frame(fb);

    // Skin detection
    for(int i=0;i<DS_W*DS_H;i++){
        skin_mask[i] = is_skin(small_buf[i]);
    }

    memset(x_arr,0,sizeof(int)*MAX_FACES);
    memset(y_arr,0,sizeof(int)*MAX_FACES);
    memset(w_arr,0,sizeof(int)*MAX_FACES);
    memset(h_arr,0,sizeof(int)*MAX_FACES);

    int faces = 0;
    int scale_x = fb->width / DS_W;
    int scale_y = fb->height / DS_H;

    for(int y=0;y<DS_H;y++){
        for(int x=0;x<DS_W;x++){
            if(!skin_mask[y*DS_W + x]) continue;

            int min_x, min_y, max_x, max_y;
            flood_fill(x, y, &min_x, &min_y, &max_x, &max_y);

            int w_full = (max_x - min_x + 1) * scale_x;
            int h_full = (max_y - min_y + 1) * scale_y;

            // Only consider minimum size faces
            if(w_full >= MIN_FACE_W && h_full >= MIN_FACE_H && faces < MAX_FACES){
                x_arr[faces] = min_x * scale_x;
                y_arr[faces] = min_y * scale_y;
                w_arr[faces] = w_full;
                h_arr[faces] = h_full;
                faces++;
            }
        }
    }

    return faces;
}

// Draw detection boxes
void draw_faces(camera_fb_t *fb, int *x, int *y, int *w, int *h, int num_faces){
    if(!fb) return;
    uint16_t *img = (uint16_t*)fb->buf;
    uint16_t color = 0x07E0; // bright green

    for(int i=0;i<num_faces;i++){
        int x0 = x[i];
        int y0 = y[i];
        int x1 = x0 + w[i];
        int y1 = y0 + h[i];

        // Top & bottom
        for(int xx=x0;xx<=x1 && xx<fb->width;xx++){
            if(y0>=0 && y0<fb->height) img[y0*fb->width + xx] = color;
            if(y1>=0 && y1<fb->height) img[y1*fb->width + xx] = color;
        }

        // Left & right
        for(int yy=y0;yy<=y1 && yy<fb->height;yy++){
            if(x0>=0 && x0<fb->width) img[yy*fb->width + x0] = color;
            if(x1>=0 && x1<fb->width) img[yy*fb->width + x1] = color;
        }
    }
}
