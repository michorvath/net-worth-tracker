#include "database.h"

bool initDatabase() {
  if (!LittleFS.begin(true)) {
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

    Serial.printf("Updated net worth for %s: $%.2f\n", date, netWorth / 100.0);
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

    Serial.printf("Saved net worth for %s: $%.2f\n", date, netWorth / 100.0);
  }

  return true;
}

bool getLatestNetWorth(DailyNetWorth& result) {
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file || file.size() < sizeof(DailyNetWorth)) {
    if (file) file.close();
    return false;
  }

  file.seek(file.size() - sizeof(DailyNetWorth));
  size_t bytesRead = file.read((uint8_t*)&result, sizeof(DailyNetWorth));
  file.close();

  return bytesRead == sizeof(DailyNetWorth);
}

int getNetWorthHistory(DailyNetWorth* buffer, int maxDays) {
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file) return 0;

  int totalRecords = file.size() / sizeof(DailyNetWorth);
  int toRead = min(maxDays, totalRecords);
  int startIndex = totalRecords - toRead;

  file.seek(startIndex * sizeof(DailyNetWorth));
  size_t bytesRead = file.read((uint8_t*)buffer, toRead * sizeof(DailyNetWorth));
  file.close();

  return bytesRead / sizeof(DailyNetWorth);
}

float getPercentageChange(int daysAgo) {
  int recordCount = getRecordCount();
  if (recordCount < 2) return 0.0f;

  DailyNetWorth latest;
  if (!getLatestNetWorth(latest)) return 0.0f;

  // calculate how far back to go (limited by available records)
  int targetIndex = max(0, recordCount - 1 - daysAgo);

  // read the target record
  File file = LittleFS.open(DB_FILE, FILE_READ);
  if (!file) {
    return 0.0f;
  }

  file.seek(targetIndex * sizeof(DailyNetWorth));
  DailyNetWorth past;
  size_t bytesRead = file.read((uint8_t*)&past, sizeof(DailyNetWorth));
  file.close();

  if (bytesRead != sizeof(DailyNetWorth)) {
    return 0.0f;
  }

  // calculate percentage change
  if (past.netWorth == 0) {
    return 0.0f;
  }

  float change = ((float)(latest.netWorth - past.netWorth) / (float)past.netWorth) * 100.0f;
  return change;
}
