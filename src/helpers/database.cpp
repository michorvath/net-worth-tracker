#include "database.h"
#include "configuration.h"
#include "format.h"

bool initDatabase() {
  if (!LittleFS.begin()) {
      Serial.println("LittleFS mount failed!");
      return false;
  }
  Serial.println("LittleFS mounted successfully");
  Serial.printf("Total: %u bytes, Used: %u bytes\n", LittleFS.totalBytes(), LittleFS.usedBytes());
  return true;
}

int getRecordCount() {
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file) {
    return 0;
  }

  int count = file.size() / sizeof(DailyNetWorth);
  file.close();
  return count;
}

// find index of record with matching date, returns -1 if not found
static int findDateIndex(const char* date) {
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file) {
    return -1;
  }
  DailyNetWorth record;
  int index = 0;

  while (file.read((uint8_t*)&record, sizeof(DailyNetWorth)) == sizeof(DailyNetWorth)) {
    if (strncmp(record.date, date, DATE_LEN - 1) == 0) {
      file.close();
      return index;
    }
    index++;
  }
  file.close();
  return -1;
}

bool saveNetWorth(const char* date, int32_t netWorth) {
  DailyNetWorth entry;
  strncpy(entry.date, date, DATE_LEN - 1);
  entry.date[DATE_LEN - 1] = '\0';
  entry.netWorth = netWorth;

  int existingIndex = findDateIndex(date);

  if (existingIndex >= 0) {
    File file = LittleFS.open(DB_FILE, "r+");
    if (!file) {
      Serial.println("Failed to open database for update");
      return false;
    }

    file.seek(existingIndex * sizeof(DailyNetWorth));
    size_t written = file.write((uint8_t*)&entry, sizeof(DailyNetWorth));
    file.close();

    if (written != sizeof(DailyNetWorth)) {
      Serial.println("Failed to update record");
      return false;
    }

    Serial.printf("Updated net worth for %s: $%d\n", date, netWorth);
  } else {
    File file = LittleFS.open(DB_FILE, FILE_APPEND);
    if (!file) {
      Serial.println("Failed to open database for append");
      return false;
    }

    size_t written = file.write((uint8_t*)&entry, sizeof(DailyNetWorth));
    file.close();

    if (written != sizeof(DailyNetWorth)) {
      Serial.println("Failed to append record");
      return false;
    }

    Serial.printf("Saved net worth for %s: $%d\n", date, netWorth);
  }

  return true;
}

bool getLatestNetWorth(DailyNetWorth& result) {
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file || file.size() < sizeof(DailyNetWorth)) {
    if (file) {
      file.close();
    }
    return false;
  }

  file.seek(file.size() - sizeof(DailyNetWorth));
  size_t bytesRead = file.read((uint8_t*)&result, sizeof(DailyNetWorth));
  file.close();

  return bytesRead == sizeof(DailyNetWorth);
}

bool getNetWorthDaysAgo(int daysAgo, DailyNetWorth& result) {
  int recordCount = getRecordCount();
  if (recordCount == 0) {
    return false;
  }

  // clamp to oldest available if not enough history
  int targetIndex = max(0, recordCount - 1 - daysAgo);

  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file) {
    return false;
  }

  file.seek(targetIndex * sizeof(DailyNetWorth));
  size_t bytesRead = file.read((uint8_t*)&result, sizeof(DailyNetWorth));
  file.close();

  return bytesRead == sizeof(DailyNetWorth);
}

int getNetWorthHistory(DailyNetWorth* buffer, int maxDays) {
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file) {
    return 0;
  }

  int totalRecords = file.size() / sizeof(DailyNetWorth);
  int toRead = min(maxDays, totalRecords);
  int startIndex = totalRecords - toRead;

  file.seek(startIndex * sizeof(DailyNetWorth));
  size_t bytesRead = file.read((uint8_t*)buffer, toRead * sizeof(DailyNetWorth));
  file.close();

  return bytesRead / sizeof(DailyNetWorth);
}

float getPercentageChange(int daysAgo) {
  DailyNetWorth latest;
  if (!getLatestNetWorth(latest)) {
    return 0.0f;
  }

  DailyNetWorth past;
  if (!getNetWorthDaysAgo(daysAgo, past)) {
    return 0.0f;
  }

  if (past.netWorth == 0) {
    return 0.0f;
  }

  float change = ((float)(latest.netWorth - past.netWorth) / (float)past.netWorth) * 100.0f;
  return change;
}

String getGoalProjection() {
  int recordCount = getRecordCount();

  // need at least 14 days of data for a "meaningful" projection
  if (recordCount < 14) {
    return "";
  }

  DailyNetWorth currentNW;
  if (!getLatestNetWorth(currentNW)) {
    return "";
  }

  if (currentNW.netWorth >= GOAL) {
    return "Goal Reached!";
  }

  DailyNetWorth startNW;
  if (!getNetWorthDaysAgo(365, startNW)) {
    return "";
  }

  float delta = (float)(currentNW.netWorth - startNW.netWorth);

  // scale up delta to annual velocity if we have less than a year of data
  int daysOfData = min(recordCount, 365);
  float annualVelocity = delta * (365.0f / (float)daysOfData);

  // if recent velocity is negative but we have more history, try all-time average
  if (annualVelocity <= 0 && recordCount > 365) {
    DailyNetWorth oldestNW;
    if (getNetWorthDaysAgo(recordCount - 1, oldestNW)) {
      float allTimeDelta = (float)(currentNW.netWorth - oldestNW.netWorth);
      annualVelocity = allTimeDelta * (365.0f / (float)recordCount);
    }
  }

  String goalStr = formatCurrency(GOAL);

  if (annualVelocity <= 0) {
    return "";
  }

  float gap = (float)(GOAL - currentNW.netWorth);
  float years = gap / annualVelocity;
  float totalMonths = years * 12.0f;

  if (totalMonths < 1.0f) {
    return "Less than a month to " + goalStr;
  }

  if (years < 1.0f) {
    int m = (int)floor(totalMonths);
    if (m < 1) m = 1;
    return String(m) + (m == 1 ? " month to " : " months to ") + goalStr;
  }

  float roundedYears = round(years * 10.0f) / 10.0f;
  if (roundedYears == (int)roundedYears) {
    return String((int)roundedYears) + " year" + ((int)roundedYears == 1 ? "" : "s") + " to " + goalStr;
  }

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.1f", roundedYears);
  return String(buffer) + " years to " + goalStr;
}