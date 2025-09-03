#include <Arduino.h>
#include "../lib/camera.h"

static const char* TAG = "MAIN";

void setup() {
  Serial.begin(115200);

  delay(1000);  // small delay for clear logging

  Serial.printf("[%s] Begin setup\r\n", TAG);

  initCamera();
  setCameraSettings();

  Serial.printf("[%s] Completed setup\r\n", TAG);
}

void loop() {
  delay(5000);
  
  if (!sendPhotoSerial()){
    Serial.printf("[%s] Sending Photo Failed\r\n", TAG);
  }else{
    Serial.printf("[%s] Sending Photo Complete\r\n", TAG);
  }
  
}
