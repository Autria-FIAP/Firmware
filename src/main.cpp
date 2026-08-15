#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <ESPAsyncWebServer.h>
#include "camera_utils.h"
#include "stream_server.h"

const char* AP_SSID = "OculosTeste";
const char* AP_SENHA = "teste1234";

AsyncWebServer servidor(80);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!inicializarCamera()) {
    Serial.println("Camera falhou. Parando.");
    while (1) delay(1000);
  }
  Serial.println("Camera inicializada com sucesso!");

  WiFi.softAP(AP_SSID, AP_SENHA);
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.print("SoftAP criado. Conecte-se a rede '");
  Serial.print(AP_SSID);
  Serial.println("'");
  Serial.print("Foto unica: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/capture");
  Serial.print("Video ao vivo: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println(":81/stream");

  servidor.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    camera_fb_t* fb = capturarFotoValida();
    if (!fb) {
      request->send(500, "text/plain", "Falha ao capturar imagem valida");
      return;
    }
    AsyncWebServerResponse *response = request->beginResponse_P(
        200, "image/jpeg", fb->buf, fb->len);
    request->send(response);
    esp_camera_fb_return(fb);
  });
  servidor.begin();

  iniciarServidorStream();
}

void loop() {
  // nada aqui, os dois servidores cuidam das requisicoes sozinhos
}