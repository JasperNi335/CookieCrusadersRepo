idf_component_register(
    SRCS "main.cpp"
    REQUIRES MyLib nvs_flash esp_wifi esp_https_server esp_camera
)