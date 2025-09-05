#include "network.h"

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define MAX_NETWORK_SCAN 20

static const char* TAG = "WiFi";

wifi_config_t wifi_config = {};

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch(event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGE(TAG, "WiFi started, connecting...");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGE(TAG, "Disconnected. Trying to reconnect...");
                esp_wifi_connect();
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGE(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        }
    }
}

void initNVS(){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGE("NVS", "NVS initialized successfully");
}

void initWiFi(){
    // initialise tcp/ip
    ESP_ERROR_CHECK(esp_netif_init());

    // create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // default station interface
    esp_netif_create_default_wifi_sta();

    // order of the esp register is event, id, func, args, event data
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_config));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool scanWiFi(const char* ssid_target){
    wifi_scan_config_t scan = {};
    scan.home_chan_dwell_time = 0; // Explicitly initialize to default value
    scan.show_hidden = true;

    ESP_LOGE(TAG, "Scanning for avaliable WiFi Networks");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan, true));
    ESP_LOGE(TAG, "Scan done");

    // access points stored here
    wifi_ap_record_t ap_info[MAX_NETWORK_SCAN];
    uint16_t ap_num = MAX_NETWORK_SCAN;

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));
    ESP_LOGE(TAG, "Found %d access points:", ap_num);

    bool targetFound = false;
    for (int i = 0; i < ap_num; i++){
        ESP_LOGE(TAG, "SSID = %s | RSSI = %d", ap_info[i].ssid, ap_info[i].rssi);
        if (ssid_target){
            if (strcmp((const char *)ap_info[i].ssid, ssid_target) == 0){
                targetFound = true;
                ESP_LOGE(TAG, "Target SSID: %s found", ssid_target);
            }
        }
    }

    if (ssid_target && !targetFound) ESP_LOGE(TAG, "Target SSID: %s not found", ssid_target);
    return targetFound;
}

void setConfigs(const char* ssid, const char* pass){
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    // copy strings int to arrays
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, pass);

    wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

void setupWiFi(){
    // make this method also do something with previous passwords or remembered networks

    // make sure flash memory is not corrupted etc
    initNVS();

    // initalise wifi
    initWiFi();

    // scan for networks
    if (scanWiFi(WIFI_SSID)){
        // set configs
        setConfigs(WIFI_SSID, WIFI_PASSWORD);
    }else{
        ESP_LOGE(TAG, "SSID NOT FOUND");
        return;
    }

    ESP_LOGE(TAG, "Connecting to SSID: %s", WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_connect());
}

bool isWiFiConnected(){
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
        ESP_LOGE("WiFi", "Connected to SSID: %s", ap_info.ssid);
        return true;
    }else{
        ESP_LOGE("WiFi", "Not connected.");
        return false;
    }
}