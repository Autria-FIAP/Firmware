#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <functional>

void inicializarBLE(std::function<void(const std::string&)> onComando);

void notificarDistancia(int distanciaCm);
void notificarEvento(const std::string& evento);
bool bleClienteConectado();

#endif