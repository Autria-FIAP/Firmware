#include "http_capture.h"
#include "camera_utils.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_http_server.h"

static httpd_handle_t servidor = NULL;
static volatile bool servida = false;
const char* AP_SSID = "OculosInteligente";
const char* AP_SENHA = "oculos1234";

static esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = capturarFotoValida();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_type(req, "image/jpeg");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  if (res == ESP_OK) {
    servida = true;
  }
  return res;
}

void iniciarTransferenciaWifi() {
  servida = false;
  WiFi.softAP(AP_SSID, AP_SENHA, 6);

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32769;

  httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
  };

  if (httpd_start(&servidor, &config) == ESP_OK) {
    httpd_register_uri_handler(servidor, &capture_uri);
  }
}

void pararTransferenciaWifi() {
  if (servidor != NULL) {
    httpd_stop(servidor);
    servidor = NULL;
  }
  WiFi.softAPdisconnect(true);
}

bool imagemFoiServida() {
  return servida;
}