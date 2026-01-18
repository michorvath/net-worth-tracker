#ifndef HELPERS_FORMAT_H
#define HELPERS_FORMAT_H

#include <Arduino.h>

// format an integer value as currency with commas (e.g., 123456 -> "$123,456")
String formatCurrency(int32_t value);

// format a name with possessive suffix
String formatPossessive(const char* name);

// get the current time formatted as "Last updated: H:MM AM/PM"
String getFormattedTime();

// get the current date formatted as "MM-DD-YYYY" for database storage
String getFormattedDate();

// format a percentage value to nearest tenth, omitting .0 if whole number
String formatPercentage(float value);

#endif
