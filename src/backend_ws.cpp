#include "backend_ws.h"
#include "ws_client.h"
#include <ArduinoJson.h>

static WsClient wsCamera;
static WsClient wsDistancia;

static bool temNovaAcao = false;
static String ultimaAcaoCompacta = "";

static void processarRespostaCamera(const String& json) {
  StaticJsonDocument<2048> doc;
  DeserializationError erro = deserializeJson(doc, json);
  if (erro) {
    Serial.print("[backend_ws] Falha ao parsear JSON do backend: ");
    Serial.println(erro.c_str());
    return;
  }

  const char* action = doc["action"] | "UNKNOWN";
  const char* message = doc["message"] | "";
  bool urgent = doc["urgent"] | false;
  bool speak = doc["speak"] | false;

  StaticJsonDocument<256> compacto;
  compacto["action"] = action;
  compacto["message"] = message;
  compacto["urgent"] = urgent;
  compacto["speak"] = speak;

  ultimaAcaoCompacta = "";
  serializeJson(compacto, ultimaAcaoCompacta);
  temNovaAcao = true;
}

void iniciarConexoesBackend(const char* ip, int porta) {
  Serial.println("[backend_ws] Conectando camera WS...");
  if (wsCamera.conectar(ip, porta, "/api/v1/guia/tempo-real")) {
    Serial.println("[backend_ws] Camera WS conectado!");
  } else {
    Serial.println("[backend_ws] Falha ao conectar Camera WS.");
  }

  Serial.println("[backend_ws] Conectando distancia WS...");
  if (wsDistancia.conectar(ip, porta, "/api/v1/distance-sensor")) {
    Serial.println("[backend_ws] Distancia WS conectado!");
  } else {
    Serial.println("[backend_ws] Falha ao conectar Distancia WS.");
  }
}

void pararConexoesBackend() {
  wsCamera.desconectar();
  wsDistancia.desconectar();
}

void loopBackendWs() {
  // Reconecta sozinho se cair
  static unsigned long ultimaTentativa = 0;
  if (!wsCamera.conectado() && millis() - ultimaTentativa > 3000) {
    ultimaTentativa = millis();
    if (wsCamera.conectar("192.168.4.60", 8000, "/api/v1/guia/tempo-real")) {
      Serial.println("[backend_ws] Camera WS reconectado!");
    }
  }
  if (!wsDistancia.conectado() && millis() - ultimaTentativa > 3000) {
    if (wsDistancia.conectar("192.168.4.60", 8000, "/api/v1/distance-sensor")) {
      Serial.println("[backend_ws] Distancia WS reconectado!");
    }
  }

  String resposta;
  if (wsCamera.receberTexto(resposta)) {
    processarRespostaCamera(resposta);
  }
  String respostaDistancia;
  if (wsDistancia.receberTexto(respostaDistancia)) {
    Serial.print("[backend_ws] Resposta do sensor de distancia recebida: ");
    Serial.println(respostaDistancia);
  }
}

void enviarFrameParaBackend(camera_fb_t* fb, int frameId) {
  if (fb == nullptr) return;
  if (!wsCamera.conectado()) {
    Serial.println("[DEBUG] enviarFrameParaBackend chamado, mas wsCamera NAO esta conectado.");
    return;
  }

  String metadata = "{\"frame_id\":" + String(frameId) + "}";
  wsCamera.enviarTexto(metadata);
  wsCamera.enviarBinario(fb->buf, fb->len);
  Serial.printf("[backend_ws] Frame %d enviado (%d bytes)\n", frameId, fb->len);
}

void enviarDistanciaParaBackend(int frameId, float distanciaCm, bool valida) {
  if (!wsDistancia.conectado()) return;

  String json = "{\"frame_id\":" + String(frameId) +
                ",\"distance_cm\":" + String(distanciaCm, 1) +
                ",\"valid\":" + (valida ? "true" : "false") + "}";
  wsDistancia.enviarTexto(json);
  Serial.print("[backend_ws] Distancia enviada: ");
  Serial.println(json);
}

bool backendTemNovaAcao() {
  return temNovaAcao;
}

String consumirUltimaAcao() {
  temNovaAcao = false;
  return ultimaAcaoCompacta;
}