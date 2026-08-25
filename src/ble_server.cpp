#include "ble_server.h"
#include <NimBLEDevice.h>

#define SERVICE_UUID          "12345678-1234-1234-1234-1234567890ab"
#define CHAR_COMANDO_UUID     "12345678-1234-1234-1234-1234567890ac"
#define CHAR_DISTANCIA_UUID   "12345678-1234-1234-1234-1234567890ad"
#define CHAR_EVENTO_UUID      "12345678-1234-1234-1234-1234567890af"

static NimBLECharacteristic* charDistancia = nullptr;
static NimBLECharacteristic* charEvento = nullptr;
static bool clienteConectado = false;
static std::function<void(const std::string&)> callbackComando = nullptr;

class ComandoCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic) override {
    if (callbackComando != nullptr) {
      callbackComando(characteristic->getValue());
    }
  }
};

class ServidorCallback : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server) override {
    clienteConectado = true;
  }
  void onDisconnect(NimBLEServer* server) override {
    clienteConectado = false;
    NimBLEDevice::startAdvertising();
  }
};

void inicializarBLE(std::function<void(const std::string&)> onComando) {
  callbackComando = onComando;

  NimBLEDevice::init("OculosInteligente");

  NimBLEServer* bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServidorCallback());

  NimBLEService* servico = bleServer->createService(SERVICE_UUID);

  NimBLECharacteristic* charComando = servico->createCharacteristic(
      CHAR_COMANDO_UUID, NIMBLE_PROPERTY::WRITE);
  charComando->setCallbacks(new ComandoCallback());

  charDistancia = servico->createCharacteristic(
      CHAR_DISTANCIA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

  charEvento = servico->createCharacteristic(
      CHAR_EVENTO_UUID, NIMBLE_PROPERTY::NOTIFY);

  servico->start();

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->start();
}

void notificarDistancia(int distanciaCm) {
  if (charDistancia != nullptr && clienteConectado) {
    std::string valor = std::to_string(distanciaCm);
    charDistancia->setValue(valor);
    charDistancia->notify();
  }
}

void notificarEvento(const std::string& evento) {
  if (charEvento != nullptr && clienteConectado) {
    charEvento->setValue(evento);
    charEvento->notify();
  }
}

bool bleClienteConectado() {
  return clienteConectado;
}