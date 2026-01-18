#include "power.h"
#include "configuration.h"

void initBattery() {
  pinMode(BATTERY_ENABLE_PIN, OUTPUT);
  digitalWrite(BATTERY_ENABLE_PIN, LOW);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
}

float getBatteryVoltage() {
  digitalWrite(BATTERY_ENABLE_PIN, HIGH);
  delay(5);  // allow circuit to stabilize
  int mv = analogReadMilliVolts(BATTERY_ADC_PIN);
  digitalWrite(BATTERY_ENABLE_PIN, LOW);
  return (mv / 1000.0) * 2.0;  // 2x due to voltage divider
}

int getBatteryPercent() {
  float voltage = getBatteryVoltage();
  // LiPo discharge curve: 4.2V = 100%, 3.0V = 0%
  int percent = (int)((voltage - 3.0) / 1.2 * 100);
  return constrain(percent, 0, 100);
}

bool isBatteryLow() {
  return getBatteryPercent() <= BATTERY_LOW_THRESHOLD;
}
