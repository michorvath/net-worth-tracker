#ifndef HELPERS_POWER_H
#define HELPERS_POWER_H

#include <Arduino.h>

#define BATTERY_ADC_PIN 1
#define BATTERY_ENABLE_PIN 21

// initialize battery monitoring pins and ADC
void initBattery();

// read the current battery voltage (returns volts, e.g., 3.7)
float getBatteryVoltage();

// get battery percentage (0-100) based on LiPo discharge curve
int getBatteryPercent();

// check if battery is low (< BATTERY_LOW_THRESHOLD)
bool isBatteryLow();

#endif
