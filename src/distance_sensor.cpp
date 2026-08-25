#include "distance_sensor.h"
#include <Wire.h>
#include <VL53L1X.h>

#define I2C_SDA 5
#define I2C_SCL 6

VL53L1X sensor;

bool inicializarSensorDistancia() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  sensor.setTimeout(500);
  if (!sensor.init()) {
    return false;
  }

  sensor.setDistanceMode(VL53L1X::Short);
  sensor.setMeasurementTimingBudget(50000);
  sensor.startContinuous(100);
  return true;
}

int lerDistanciaCm() {
  int distanciaMm = sensor.read();
  if (sensor.timeoutOccurred()) {
    return -1;
  }
  return distanciaMm / 10;
}