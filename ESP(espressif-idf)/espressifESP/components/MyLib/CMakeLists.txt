idf_component_register(
    SRCS "src/camera.cpp"
         "src/facialDetection.h"
    INCLUDE_DIRS "include"
    REQUIRES esp_camera esp_who
)
