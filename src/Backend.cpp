#include "backend_ws.h"
#include <WebSocketsClient.h>

static WebSocketsClient wsCamera;
static WebSocketsClient wsDistancia;

static bool temNovaAcao = false;
static String ultimaAcaoJson = "";

static void onEventoCamera(WStype_t tipo, uint8_t* payload, size_t tamanho) {
  switch (tipo) {
    case WStype_CONNECTED:
      Serial.println("[backend_ws] Camera WS conectado");
      break;
    case WStype_DISCONNECTED:
      Serial.println("[backend_ws] Camera WS desconectado");
      break;
    case WStype_TEXT:
      ultimaAcaoJson = String((char*)payload).substring(0, tamanho);
      temNovaAcao = true;
      break;
    case WStype_ERROR:
      Serial.println("[backend_ws] Erro no Camera WS");
      break;
    default:
      break;
  }
}

static void onEventoDistancia(WStype_t tipo, uint8_t* payload, size_t tamanho) {
  switch (tipo) {
    case WStype_CONNECTED:
      Serial.println("[backend_ws] Distancia WS conectado");
      break;
    case WStype_DISCONNECTED:
      Serial.println("[backend_ws] Distancia WS desconectado");
      break;
    case WStype_ERROR:
      Serial.println("[backend_ws] Erro no Distancia WS");
      break;
    default:
      break;
  }
}

void iniciarConexoesBackend(const char* ip, int porta) {
  wsCamera.begin(ip, porta, "/api/v1/guia/tempo-real");
  wsCamera.onEvent(onEventoCamera);
  wsCamera.setReconnectInterval(3000);

  wsDistancia.begin(ip, porta, "/api/v1/distance-sensor");
  wsDistancia.onEvent(onEventoDistancia);
  wsDistancia.setReconnectInterval(3000);
}

void pararConexoesBackend() {
  wsCamera.disconnect();
  wsDistancia.disconnect();
}

void loopBackendWs() {
  wsCamera.loop();
  wsDistancia.loop();
}

void enviarFrameParaBackend(camera_fb_t* fb, int frameId) {
  if (fb == nullptr) return;
  String metadata = "{\"frame_id\":" + String(frameId) + "}";
  wsCamera.sendTXT(metadata);
  wsCamera.sendBIN(fb->buf, fb->len);
}

void enviarDistanciaParaBackend(int frameId, float distanciaCm, bool valida) {
  String json = "{\"frame_id\":" + String(frameId) +
                ",\"distance_cm\":" + String(distanciaCm, 1) +
                ",\"valid\":" + (valida ? "true" : "false") + "}";
  wsDistancia.sendTXT(json);
}

bool backendTemNovaAcao() {
  return temNovaAcao;
}

String consumirUltimaAcao() {
  temNovaAcao = false;
  return ultimaAcaoJson;
}