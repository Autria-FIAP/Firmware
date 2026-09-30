#include "ws_client.h"
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"

bool WsClient::conectar(const char* host, uint16_t porta, const char* path) {
  if (!_client.connect(host, porta)) {
    return false;
  }

  // Chave fixa e simples (nao precisa ser aleatoria de verdade para
  // nosso caso - o servidor so confere se a resposta bate com ela).
  const char* chave = "dGhlIHNhbXBsZSBub25jZQ==";

  String pedido = String("GET ") + path + " HTTP/1.1\r\n" +
                  "Host: " + host + ":" + String(porta) + "\r\n" +
                  "Upgrade: websocket\r\n" +
                  "Connection: Upgrade\r\n" +
                  "Sec-WebSocket-Key: " + chave + "\r\n" +
                  "Sec-WebSocket-Version: 13\r\n" +
                  "\r\n";

  _client.print(pedido);

  unsigned long inicio = millis();
  String resposta = "";
  while (millis() - inicio < 5000) {
    while (_client.available()) {
      char c = _client.read();
      resposta += c;
      if (resposta.endsWith("\r\n\r\n")) {
        goto fim_leitura;
      }
    }
    yield(); // cede tempo para outras tarefas do sistema (camera, wifi, etc.)
    if (!_client.connected()) break;
  }
  fim_leitura:

  if (resposta.indexOf("101") == -1 || resposta.indexOf("101") > 20) {
    // "101" precisa aparecer logo no inicio (status line), nao em outro lugar
    _client.stop();
    return false;
  }

  _conectado = true;
  return true;
}

void WsClient::desconectar() {
  _client.stop();
  _conectado = false;
}

bool WsClient::conectado() {
  if (_conectado && !_client.connected()) {
    _conectado = false;
  }
  return _conectado;
}

void WsClient::enviarFrame(uint8_t opcode, const uint8_t* dados, size_t tamanho) {
  if (!conectado()) return;

  uint8_t mascara[4];
  for (int i = 0; i < 4; i++) mascara[i] = random(0, 256);

  uint8_t cabecalho[10];
  int idx = 0;
  cabecalho[idx++] = 0x80 | opcode; // FIN=1 + opcode

  if (tamanho <= 125) {
    cabecalho[idx++] = 0x80 | tamanho; // MASK=1 + tamanho
  } else if (tamanho <= 65535) {
    cabecalho[idx++] = 0x80 | 126;
    cabecalho[idx++] = (tamanho >> 8) & 0xFF;
    cabecalho[idx++] = tamanho & 0xFF;
  } else {
    // Payloads maiores que 65KB nao suportados nesta versao simplificada -
    // suficiente para as resolucoes de imagem que estamos usando (SVGA/VGA).
    return;
  }

  for (int i = 0; i < 4; i++) cabecalho[idx++] = mascara[i];

  _client.write(cabecalho, idx);

  // Manda o payload mascarado em blocos, sem precisar duplicar o buffer inteiro
  const size_t TAMANHO_BLOCO = 512;
  uint8_t bloco[TAMANHO_BLOCO];
  size_t enviados = 0;
  while (enviados < tamanho) {
    size_t restante = tamanho - enviados;
    size_t nesseBloco = (restante < TAMANHO_BLOCO) ? restante : TAMANHO_BLOCO;
    for (size_t i = 0; i < nesseBloco; i++) {
      bloco[i] = dados[enviados + i] ^ mascara[(enviados + i) % 4];
    }
    _client.write(bloco, nesseBloco);
    enviados += nesseBloco;
  }
}

void WsClient::enviarTexto(const String& texto) {
  enviarFrame(0x1, (const uint8_t*)texto.c_str(), texto.length());
}

void WsClient::enviarBinario(const uint8_t* dados, size_t tamanho) {
  enviarFrame(0x2, dados, tamanho);
}

bool WsClient::receberTexto(String& saida) {
  if (!conectado()) return false;
  if (_client.available() < 2) return false;

  uint8_t b1 = _client.read();
  uint8_t b2 = _client.read();

  uint8_t opcode = b1 & 0x0F;
  bool mascarado = b2 & 0x80;
  uint64_t tamanho = b2 & 0x7F;

  if (tamanho == 126) {
    while (_client.available() < 2) yield();
    uint8_t ext[2];
    _client.readBytes(ext, 2);
    tamanho = (ext[0] << 8) | ext[1];
  } else if (tamanho == 127) {
    while (_client.available() < 8) yield();
    uint8_t ext[8];
    _client.readBytes(ext, 8);
    tamanho = 0;
    for (int i = 0; i < 8; i++) tamanho = (tamanho << 8) | ext[i];
  }

  uint8_t mascara[4] = {0, 0, 0, 0};
  if (mascarado) {
    while (_client.available() < 4) yield();
    _client.readBytes(mascara, 4);
  }

  saida = "";
  uint32_t lidos = 0;
  unsigned long inicioEspera = millis();
  while (lidos < tamanho && millis() - inicioEspera < 3000) {
    if (_client.available()) {
      uint8_t c = _client.read();
      if (mascarado) c ^= mascara[lidos % 4];
      saida += (char)c;
      lidos++;
      inicioEspera = millis();
    } else {
      yield(); // cede tempo para outras tarefas do sistema
    }
  }

  return (opcode == 0x1); // true somente se for um frame de texto
}