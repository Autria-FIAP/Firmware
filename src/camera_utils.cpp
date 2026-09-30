#include "camera_utils.h"
#include "camera_pins.h"
#include <Arduino.h>
#include "esp_log.h"

bool inicializarCamera() {
  esp_log_level_set("cam_hal", ESP_LOG_VERBOSE);
  esp_log_level_set("camera", ESP_LOG_VERBOSE);
  esp_log_level_set("sccb", ESP_LOG_VERBOSE);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 8000000; // TESTE DIAGNOSTICO: reduzido de 10MHz para 8MHz
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    Serial.println("AVISO: PSRAM nao encontrada!");
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 14;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t resultado = esp_camera_init(&config);
  if (resultado != ESP_OK) {
    Serial.printf("Falha ao inicializar camera: 0x%x\n", resultado);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != nullptr) {
    sensor->set_brightness(sensor, 1);
    sensor->set_contrast(sensor, 1);
    sensor->set_saturation(sensor, 0);
    sensor->set_sharpness(sensor, 1);
    sensor->set_denoise(sensor, 1);
    sensor->set_whitebal(sensor, 1);
    sensor->set_gain_ctrl(sensor, 1);
    sensor->set_exposure_ctrl(sensor, 1);
    sensor->set_vflip(sensor, 1);
  }

  for (int i = 0; i < 3; i++) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
    delay(100);
  }

  return true;
}

bool jpegValido(camera_fb_t *fb) {
  if (fb->len < 4) return false;
  bool inicioOk = (fb->buf[0] == 0xFF && fb->buf[1] == 0xD8);
  bool fimOk = (fb->buf[fb->len - 2] == 0xFF && fb->buf[fb->len - 1] == 0xD9);
  return inicioOk && fimOk;
}

camera_fb_t* capturarFotoValida() {
  for (int tentativa = 0; tentativa < 3; tentativa++) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.printf("[DEBUG-CAM] Tentativa %d: esp_camera_fb_get() retornou NULL (sem buffer disponivel)\n", tentativa + 1);
      continue;
    }
    Serial.printf("[DEBUG-CAM] Tentativa %d: fb obtido, %d bytes. Validando JPEG...\n", tentativa + 1, fb->len);
    if (jpegValido(fb)) {
      Serial.println("[DEBUG-CAM] JPEG valido!");
      return fb;
    }
    Serial.printf("[DEBUG-CAM] JPEG INVALIDO. Primeiros bytes: %02X %02X | Ultimos bytes: %02X %02X\n",
                  fb->buf[0], fb->buf[1], fb->buf[fb->len-2], fb->buf[fb->len-1]);
    esp_camera_fb_return(fb);
  }
  return nullptr;
}