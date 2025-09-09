idf_component_register(
    SRCS "src/camera.cpp"
         "src/network.cpp"
         "src/serial.cpp"
         "src/client.cpp"
         "src/servo.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_http_client esp_wifi esp_camera
)
