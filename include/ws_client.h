#ifndef WS_CLIENT_H
#define WS_CLIENT_H

#include <Arduino.h>
#include <WiFiClient.h>

// Cliente WebSocket minimo, feito a mao sobre WiFiClient.
// Suporta: handshake, envio de texto/binario mascarado (obrigatorio
// para frames cliente->servidor pelo protocolo), recebimento de texto.
// Nao suporta fragmentacao de mensagens (assume mensagens em 1 frame).

class WsClient {
public:
  bool conectar(const char* host, uint16_t porta, const char* path);
  void desconectar();
  bool conectado();

  void enviarTexto(const String& texto);
  void enviarBinario(const uint8_t* dados, size_t tamanho);

  // Tenta ler uma mensagem de texto disponivel. Retorna true se leu algo.
  bool receberTexto(String& saida);

private:
  WiFiClient _client;
  bool _conectado = false;

  void enviarFrame(uint8_t opcode, const uint8_t* dados, size_t tamanho);
};

#endif