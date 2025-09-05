#ifndef NETWORK_H
#define NETWORK_H

#include <String.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "keys.h"

// intialise NVS
void initNVS();

// sets up network, adds it to non-volitle memory
void setupWiFi();

// simple access point check
bool isWiFiConnected();

#endif // NETWORK_H