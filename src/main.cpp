#include <Arduino.h>
#include <WiFi.h>
#include "ble_server.h"
#include "distance_sensor.h"
#include "camera_utils.h"
#include "http_capture.h"
#include "backend_ws.h"

enum class SystemState {
  AGUARDANDO,
  CAPTURANDO_IMAGEM,
  WIFI_ATIVO_TRANSFERINDO,
  NAVEGACAO_ATIVA,
  EMERGENCIA_SOS
};

SystemState estadoAtual = SystemState::AGUARDANDO;
SystemState estadoAntesDoSOS = SystemState::AGUARDANDO;

#define PINO_BOTAO_SOS 2
unsigned long ultimoAcionamentoBotao = 0;
const unsigned long DEBOUNCE_MS = 250;

int limiteDistanciaCm = 100;
unsigned long ultimaLeituraDistancia = 0;
const unsigned long INTERVALO_LEITURA_DISTANCIA_MS = 200;

unsigned long inicioTransferenciaWifi = 0;
const unsigned long TIMEOUT_SEGURANCA_MS = 60000;
const unsigned long GRACA_APOS_ENTREGA_MS = 3000;

const char* BACKEND_IP = "192.168.4.2"; // TODO: confirmar IP real do notebook do backend
const int BACKEND_PORTA = 8000;
int frameIdNavegacao = 0;
unsigned long ultimoEnvioNavegacao = 0;
const unsigned long INTERVALO_ENVIO_NAVEGACAO_MS = 500;

void tratarComandoBLE(const std::string& comandoOriginal) {
  std::string comando = comandoOriginal;
  while (!comando.empty() && std::isspace(comando.back())) {
    comando.pop_back();
  }
  while (!comando.empty() && std::isspace(comando.front())) {
    comando.erase(comando.begin());
  }

  Serial.print("Comando BLE recebido: '");
  Serial.print(comando.c_str());
  Serial.println("'");

  if (comando == "CAPTURE_IMAGE") {
    if (estadoAtual == SystemState::AGUARDANDO) {
      estadoAtual = SystemState::CAPTURANDO_IMAGEM;
    }
  } else if (comando == "GET_DISTANCE") {
    int distancia = lerDistanciaCm();
    notificarDistancia(distancia);
  } else if (comando == "START_NAVIGATION") {
    if (estadoAtual == SystemState::AGUARDANDO) {
      WiFi.softAP("OculosInteligente", "oculos1234", 6);
      iniciarConexoesBackend(BACKEND_IP, BACKEND_PORTA);
      frameIdNavegacao = 0;
      estadoAtual = SystemState::NAVEGACAO_ATIVA;
      notificarEvento("NAVIGATION_STARTED");
    }
  } else if (comando == "STOP_NAVIGATION") {
    if (estadoAtual == SystemState::NAVEGACAO_ATIVA) {
      pararConexoesBackend();
      WiFi.softAPdisconnect(true);
      estadoAtual = SystemState::AGUARDANDO;
      notificarEvento("NAVIGATION_STOPPED");
    }
  }
}

bool estadoAnteriorBotao = HIGH;

void verificarBotaoSOS() {
  bool estadoBotaoAgora = digitalRead(PINO_BOTAO_SOS);
  unsigned long agora = millis();

  if (estadoAnteriorBotao == HIGH && estadoBotaoAgora == LOW) {
    if (agora - ultimoAcionamentoBotao > DEBOUNCE_MS) {
      ultimoAcionamentoBotao = agora;
      if (estadoAtual != SystemState::EMERGENCIA_SOS) {
        estadoAntesDoSOS = estadoAtual;
        estadoAtual = SystemState::EMERGENCIA_SOS;
      }
    }
  }

  estadoAnteriorBotao = estadoBotaoAgora;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(PINO_BOTAO_SOS, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Inicializando sensor de distancia...");
  if (!inicializarSensorDistancia()) {
    Serial.println("AVISO: sensor de distancia falhou ao inicializar.");
  }

  Serial.println("Inicializando camera...");
  if (!inicializarCamera()) {
    Serial.println("AVISO: camera falhou ao inicializar.");
    while (1) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
  }
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("Inicializando BLE...");
  inicializarBLE(tratarComandoBLE);

  Serial.println("Firmware pronto. Estado: AGUARDANDO");
}

void loop() {
  verificarBotaoSOS();

  switch (estadoAtual) {

    case SystemState::AGUARDANDO: {
      static unsigned long ultimoHeartbeat = 0;
      if (millis() - ultimoHeartbeat > 2000) {
        ultimoHeartbeat = millis();
        Serial.println("[HEARTBEAT] loop() rodando, estado: AGUARDANDO");
      }
      unsigned long agora = millis();
      if (agora - ultimaLeituraDistancia > INTERVALO_LEITURA_DISTANCIA_MS) {
        ultimaLeituraDistancia = agora;
        int distancia = lerDistanciaCm();
        if (distancia > 0 && distancia < limiteDistanciaCm) {
          notificarEvento("OBSTACLE_DETECTED:" + std::to_string(distancia));
        }
      }
      break;
    }

    case SystemState::CAPTURANDO_IMAGEM: {
      Serial.println("Capturando imagem...");
      iniciarTransferenciaWifi();
      inicioTransferenciaWifi = millis();
      notificarEvento("IMAGE_READY");
      estadoAtual = SystemState::WIFI_ATIVO_TRANSFERINDO;
      break;
    }

    case SystemState::WIFI_ATIVO_TRANSFERINDO: {
      unsigned long decorrido = millis() - inicioTransferenciaWifi;

      bool deveDesligar = false;
      if (imagemFoiServida() && decorrido > GRACA_APOS_ENTREGA_MS) {
        deveDesligar = true;
      } else if (decorrido > TIMEOUT_SEGURANCA_MS) {
        deveDesligar = true;
      }

      if (deveDesligar) {
        pararTransferenciaWifi();
        estadoAtual = SystemState::AGUARDANDO;
        Serial.println("Transferencia encerrada. Estado: AGUARDANDO");
      }
      break;
    }

    case SystemState::NAVEGACAO_ATIVA: {
      loopBackendWs();

      unsigned long agora = millis();
      if (agora - ultimoEnvioNavegacao > INTERVALO_ENVIO_NAVEGACAO_MS) {
        ultimoEnvioNavegacao = agora;
        frameIdNavegacao++;

        camera_fb_t* fb = capturarFotoValida();
        if (fb) {
          enviarFrameParaBackend(fb, frameIdNavegacao);
          esp_camera_fb_return(fb);
        }

        int distancia = lerDistanciaCm();
        bool valida = (distancia > 0);
        enviarDistanciaParaBackend(frameIdNavegacao, valida ? distancia : 0, valida);
      }

      if (backendTemNovaAcao()) {
        String acao = consumirUltimaAcao();
        notificarEvento(("NAV_ACTION:" + std::string(acao.c_str())).c_str());
      }
      break;
    }

    case SystemState::EMERGENCIA_SOS: {
      Serial.println("SOS ACIONADO!");
      notificarEvento("SOS_BUTTON_PRESSED");
      estadoAtual = estadoAntesDoSOS;
      break;
    }
  }
}