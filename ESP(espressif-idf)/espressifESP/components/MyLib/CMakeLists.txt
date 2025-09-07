idf_component_register(
    SRCS "src/camera.cpp"
         "src/network.cpp"
         "src/server.cpp"
         "src/detection.cpp"
         "src/client.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_http_client esp_wifi esp_https_server esp_camera
)
