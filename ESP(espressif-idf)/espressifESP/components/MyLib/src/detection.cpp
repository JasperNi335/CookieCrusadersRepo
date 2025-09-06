#include "detection.h"
#include "esp_log.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define TAG "DETECTION"

// -------------------- Config --------------------
#define DS_W 80
#define DS_H 60

// Minimum detection box in full-res pixels
#define MIN_BOX_W_FULL  12
#define MIN_BOX_H_FULL  12

// Also require a minimum blob area (full resolution)
#define MIN_BLOB_AREA_FULL  (MIN_BOX_W_FULL * MIN_BOX_H_FULL)

// Orange detection in HSV (degrees for H, 0–255 fixed-point for S & V)
#define HUE_MIN  10   // inclusive
#define HUE_MAX  40   // inclusive
#define MIN_S    100  // 0..255  (≈0.39)
#define MIN_V     80  // 0..255  (≈0.31)

// Extra vibrancy gate: average orange score (0..255) required for a valid detection.
// Raise this to be stricter; lower to be more permissive.
#define VIBRANT_SCORE  50

// -------------------- Buffers --------------------
static uint16_t small_buf[DS_W*DS_H];
static uint8_t  mask_buf[DS_W*DS_H];     // binary mask
static uint8_t  score_buf[DS_W*DS_H];    // 0..255 orange score

// Flood-fill stack (heap allocated once)
static int (*ff_stack)[2] = NULL;
static int ff_stack_size = 0;

static bool ensure_ff_stack(void) {
    int required = DS_W * DS_H;
    if (ff_stack && ff_stack_size >= required) return true;
    if (ff_stack) { free(ff_stack); ff_stack = NULL; ff_stack_size = 0; }
    ff_stack = (int(*)[2])malloc(sizeof(int[DS_W*DS_H][2]));
    if (!ff_stack) { ESP_LOGE(TAG, "Failed to allocate flood-fill stack"); return false; }
    ff_stack_size = required;
    return true;
}

// -------------------- Color utils --------------------
// RGB565 -> 8-bit RGB
static inline void rgb565_to_rgb(uint16_t px, uint8_t *r, uint8_t *g, uint8_t *b){
    *r = ((px >> 11) & 0x1F) << 3;
    *g = ((px >> 5)  & 0x3F) << 2;
    *b = ( px        & 0x1F) << 3;
}

// Fast-ish RGB->HSV producing H in degrees (0..359), S,V in 0..255
static inline void rgb_to_hsv_u8(uint8_t r, uint8_t g, uint8_t b, uint16_t *H, uint8_t *S, uint8_t *V){
    uint8_t max = r > g ? (r > b ? r : b) : (g > b ? g : b);
    uint8_t min = r < g ? (r < b ? r : b) : (g < b ? g : b);
    uint16_t delta = (uint16_t)max - (uint16_t)min;

    *V = max;

    if (max == 0) { *S = 0; *H = 0; return; }
    *S = (uint8_t)((delta * 255 + (max>>1)) / max); // round

    if (delta == 0) { *H = 0; return; }

    int16_t h;
    if (max == r)       h = 60 * ( (int16_t)(g - b) ) / (int16_t)delta;
    else if (max == g)  h = 60 * ( (int16_t)(b - r) ) / (int16_t)delta + 120;
    else                h = 60 * ( (int16_t)(r - g) ) / (int16_t)delta + 240;

    if (h < 0) h += 360;
    *H = (uint16_t)h;
}

// “Orangeness” score 0..255 based on hue window + S and V.
// Peaks near 25°; fades to 0 at window edges; multiplied by S and V.
static inline uint8_t orange_score_rgb565(uint16_t px){
    uint8_t r,g,b; rgb565_to_rgb(px, &r, &g, &b);
    uint16_t H; uint8_t S,V;
    rgb_to_hsv_u8(r,g,b,&H,&S,&V);

    if (S < MIN_S || V < MIN_V) return 0;
    if (H < HUE_MIN || H > HUE_MAX) return 0;

    // Hue proximity shaping (triangular weight; center around ~25°)
    const int center = 25;
    const int half_range = (HUE_MAX - HUE_MIN) / 2; // e.g., 15
    int d = H - center;
    if (d < 0) d = -d;
    if (d > half_range) d = half_range;
    // Map d=0 -> 255, d=half_range -> ~85 (keeps some tolerance)
    int hue_weight = 255 - (d * 170) / (half_range ? half_range : 1);

    // Combine with saturation and value (both 0..255) → 0..255
    // score ≈ hue_weight * (S/255) * (V/255)
    uint32_t score = (uint32_t)hue_weight * (uint32_t)S;
    score = (score + 127) / 255;
    score = (score * (uint32_t)V + 127) / 255;
    if (score > 255) score = 255;
    return (uint8_t)score;
}

// -------------------- Downscale --------------------
static void downscale_frame(camera_fb_t *fb){
    int sx = fb->width  / DS_W;
    int sy = fb->height / DS_H;
    uint16_t *src = (uint16_t*)fb->buf;

    for(int y=0; y<DS_H; y++){
        int src_y = y * sy;
        for(int x=0; x<DS_W; x++){
            int src_x = x * sx;
            small_buf[y*DS_W + x] = src[src_y*fb->width + src_x];
        }
    }
}

// -------------------- Flood fill --------------------
// Flood-fill returns pixel count; also accumulates sum_score for that blob.
static int flood_fill(int x, int y, int *min_x, int *min_y, int *max_x, int *max_y, uint32_t *sum_score){
    if (!ff_stack) return 0;

    int top = 0;
    ff_stack[top][0] = x;
    ff_stack[top][1] = y;
    top++;

    *min_x = x; *min_y = y; *max_x = x; *max_y = y;
    int count = 0;
    *sum_score = 0;

    while (top > 0){
        top--;
        int cx = ff_stack[top][0];
        int cy = ff_stack[top][1];

        if (cx<0 || cy<0 || cx>=DS_W || cy>=DS_H) continue;
        int idx = cy*DS_W + cx;
        if (!mask_buf[idx]) continue;

        // consume this pixel
        mask_buf[idx] = 0;
        count++;
        *sum_score += score_buf[idx];

        if (cx < *min_x) *min_x = cx;
        if (cy < *min_y) *min_y = cy;
        if (cx > *max_x) *max_x = cx;
        if (cy > *max_y) *max_y = cy;

        if (top + 8 <= ff_stack_size){
            ff_stack[top][0]=cx+1; ff_stack[top][1]=cy;   top++;
            ff_stack[top][0]=cx-1; ff_stack[top][1]=cy;   top++;
            ff_stack[top][0]=cx;   ff_stack[top][1]=cy+1; top++;
            ff_stack[top][0]=cx;   ff_stack[top][1]=cy-1; top++;
            ff_stack[top][0]=cx+1; ff_stack[top][1]=cy+1; top++;
            ff_stack[top][0]=cx-1; ff_stack[top][1]=cy+1; top++;
            ff_stack[top][0]=cx+1; ff_stack[top][1]=cy-1; top++;
            ff_stack[top][0]=cx-1; ff_stack[top][1]=cy-1; top++;
        }
    }
    return count;
}

// -------------------- Main detection --------------------
// Finds the largest orange blob. Returns 1 if a vibrant orange detection is found; else 0.
int detect_faces(camera_fb_t *fb, int *x_arr, int *y_arr, int *w_arr, int *h_arr){
    if(!fb || !x_arr || !y_arr || !w_arr || !h_arr) return 0;
    if(fb->width < DS_W || fb->height < DS_H) return 0;
    if(!ensure_ff_stack()) return 0;

    downscale_frame(fb);

    // Fill score + binary mask
    int total_orange_pixels = 0;
    for (int i=0;i<DS_W*DS_H;i++){
        uint8_t s = orange_score_rgb565(small_buf[i]);
        score_buf[i] = s;
        uint8_t on = (s > 0);
        mask_buf[i] = on;
        total_orange_pixels += on;
    }

    int scale_x = fb->width  / DS_W;
    int scale_y = fb->height / DS_H;

    int best_area_full = 0;
    int best_x=0, best_y=0, best_w=0, best_h=0;
    uint32_t best_sum_score = 0;
    int best_count = 0;

    for (int y=0;y<DS_H;y++){
        for (int x=0;x<DS_W;x++){
            int idx = y*DS_W + x;
            if (!mask_buf[idx]) continue;

            int min_x, min_y, max_x, max_y;
            uint32_t sum_score = 0;
            int count = flood_fill(x, y, &min_x, &min_y, &max_x, &max_y, &sum_score);

            int w_full = (max_x - min_x + 1) * scale_x;
            int h_full = (max_y - min_y + 1) * scale_y;
            int area_full = w_full * h_full;

            if (w_full >= MIN_BOX_W_FULL && h_full >= MIN_BOX_H_FULL && area_full >= MIN_BLOB_AREA_FULL){
                if (area_full > best_area_full){
                    best_area_full = area_full;
                    best_x = min_x * scale_x;
                    best_y = min_y * scale_y;
                    best_w = w_full;
                    best_h = h_full;
                    best_sum_score = sum_score;
                    best_count = count;
                }
            }
        }
    }

    if (best_area_full > 0 && best_count > 0){
        // Average orange score of the best blob (0..255)
        uint8_t avg_score = (uint8_t)((best_sum_score + (best_count>>1)) / best_count);
        ESP_LOGI(TAG, "Best orange blob: area=%d avg_score=%u", best_area_full, avg_score);

        // Final vibrancy gate: only accept if sufficiently vibrant
        if (avg_score >= VIBRANT_SCORE){
            x_arr[0] = best_x;
            y_arr[0] = best_y;
            w_arr[0] = best_w;
            h_arr[0] = best_h;
            return 1;
        } else {
            ESP_LOGI(TAG, "Rejected blob (not vibrant enough). Increase scene orange or lower VIBRANT_SCORE.");
        }
    }

    return 0;
}

// -------------------- Drawing (unchanged) --------------------
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

    for(int xx=x0; xx<=x1; xx++){
        img[y0*fb->width + xx] = color;
        img[y1*fb->width + xx] = color;
    }
    for(int yy=y0; yy<=y1; yy++){
        img[yy*fb->width + x0] = color;
        img[yy*fb->width + x1] = color;
    }
}
