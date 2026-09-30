#include <Arduino.h>
#include <WiFi.h>
#include "camera_utils.h"
#include "http_capture.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Inicializando camera...");
  if (!inicializarCamera()) {
    Serial.println("Camera falhou.");
    while (1) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Camera inicializada com sucesso!");

  iniciarTransferenciaWifi();
  Serial.print("Foto disponivel em: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/capture");
}

void loop() {
  delay(1000);
}