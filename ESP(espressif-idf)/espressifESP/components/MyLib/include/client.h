#ifndef CLIENT_H
#define CLIENT_H

#include "esp_http_client.h"
#include "camera.h"
#include "network.h"
#include "keys.h"
#include "lwip/sockets.h"

// check if there is a connection
bool pingServer(const char* url);

void stream_task(void *pvParameters);

void audio_task(void *pvParameters);

char ServoCommand();

char VoiceCommand();

char DurationCommand();

#endif // CLIENT_H