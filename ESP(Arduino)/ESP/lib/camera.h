#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include <Arduino.h>

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

/*-----------------------------------------
Serial Camera methods
-----------------------------------------*/

// Serial Communication for camera
bool sendPhotoSerial();

#endif // CAMERA_H