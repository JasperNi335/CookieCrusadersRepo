#ifndef DETECTION_H
#define DETECTION_H

#include "esp_camera.h"
#include <stdint.h>

#define DS_W 80
#define DS_H 60
#define MIN_FACE_W 20
#define MIN_FACE_H 20

#define MAX_FACES 3

// Detect skin blobs, returns number of detected blobs
// x, y, w, h arrays will be filled with blob coordinates in original frame size
int detect_faces(camera_fb_t *fb, int *x, int *y, int *w, int *h);

// Draw rectangles on frame
void draw_faces(camera_fb_t *fb, int *x, int *y, int *w, int *h, int num_faces);

#endif // DETECTION_H
