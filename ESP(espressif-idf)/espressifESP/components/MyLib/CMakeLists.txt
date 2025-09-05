idf_component_register(
    SRCS "src/camera.cpp"
<<<<<<< HEAD
         "src/network.cpp"
         "src/server.cpp"
         "src/detection.cpp"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_wifi esp_https_server esp_camera
=======
         "src/facialDetection.h"
    INCLUDE_DIRS "include"
    REQUIRES esp_camera esp_who
>>>>>>> 98ba9a4be8113d0f9709983649c6a8954a716332
)
