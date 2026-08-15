#ifndef CAMERA_UTILS_H
#define CAMERA_UTILS_H

#include "esp_camera.h"

bool inicializarCamera();
bool jpegValido(camera_fb_t *fb);
camera_fb_t* capturarFotoValida();

#endif