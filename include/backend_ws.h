#ifndef BACKEND_WS_H
#define BACKEND_WS_H

#include <Arduino.h>
#include "esp_camera.h"

void iniciarConexoesBackend(const char* ip, int porta);
void pararConexoesBackend();
void loopBackendWs();

void enviarFrameParaBackend(camera_fb_t* fb, int frameId);
void enviarDistanciaParaBackend(int frameId, float distanciaCm, bool valida);

bool backendTemNovaAcao();
String consumirUltimaAcao(); 

#endif