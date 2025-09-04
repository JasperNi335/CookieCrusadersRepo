#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "esp_log.h"

/*-----------------------------------------
Default Camera methods
-----------------------------------------*/

// Initilase the camera
bool initCamera();

// deInitalise camera
void deInitCamera();

// set camera settings
void setCameraSettings();

// Capture a photo, Warning must release returned pointer
// use esp_camera_fb_return(fb)
camera_fb_t* cameraCapturePhoto();

// converts frame buffer pointer to base64
// user needs to free the buffer
char* fb_to_b64(camera_fb_t* frame_buffer);

/*-----------------------------------------
Serial Camera methods
-----------------------------------------*/

// Serial Communication for camera
bool sendPhotoSerial();

/*-----------------------------------------
Wifi Camera methods
-----------------------------------------*/

#endif // CAMERA_H