#ifndef SERVER_H
#define SERVER_H

#include <string.h>
#include "esp_https_server.h"
#include "esp_random.h"
#include "camera.h"
#include "network.h"

// starts server with default configs
httpd_handle_t startServer();

// check status of server -- very basic
bool getServerStatus();

// ends server if there is one
void endServer();

#endif // SERVER_H