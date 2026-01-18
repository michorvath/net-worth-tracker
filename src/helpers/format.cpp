#include "format.h"
#include <time.h>

String formatCurrency(int32_t value) {
  String result = "";
  String numStr = String(abs(value));
  int len = numStr.length();

  for (int i = 0; i < len; i++) {
    if (i > 0 && (len - i) % 3 == 0) {
      result += ',';
    }
    result += numStr[i];
  }

  return (value < 0 ? "-$" : "$") + result;
}

String formatPossessive(const char* name) {
  String nameStr = String(name);
  int len = nameStr.length();
  if (len > 0 && (nameStr[len - 1] == "s" || nameStr[len - 1] == "S")) {
    return nameStr + "'";
  }
  return nameStr + "'s";
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    return "Last updated: N/A";
  }

  char buffer[32];
  int hour = timeinfo.tm_hour;
  const char* ampm = hour >= 12 ? "PM" : "AM";
  if (hour == 0) hour = 12;
  else if (hour > 12) hour -= 12;

  snprintf(buffer, sizeof(buffer), "Last updated: %d:%02d %s", hour, timeinfo.tm_min, ampm);
  return String(buffer);
}

String getFormattedDate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    return "00-00-0000";
  }

  char buffer[11];  // "MM-DD-YYYY\0"
  snprintf(
    buffer,
    sizeof(buffer),
    "%02d-%02d-%04d",
    timeinfo.tm_mon + 1,  // tm_mon is 0-based
    timeinfo.tm_mday,
    timeinfo.tm_year + 1900 // tm_year is years since 1900
  );
  return String(buffer);
}

String formatPercentage(float value) {
  float rounded = round(abs(value) * 10.0f) / 10.0f; // round to nearest tenth

  // check if it's a whole number (no decimal needed)
  if (rounded == (int)rounded) {
    return String((int)rounded) + "%";
  }

  // format with one decimal place
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.1f%%", rounded);
  return String(buffer);
}
