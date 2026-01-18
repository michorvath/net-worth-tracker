#ifndef HELPERS_DATABASE_H
#define HELPERS_DATABASE_H

#include <Arduino.h>
#include <LittleFS.h>

#define DB_FILE "/networth.dat"
#define DATE_LEN 11 // "MM-DD-YYYY\0"

struct DailyNetWorth {
    char date[DATE_LEN]; // "MM-DD-YYYY"
    int32_t netWorth; // net worth in whole dollars (rounded)
};

// initialize LittleFS filesystem
bool initDatabase();

// save or update net worth for a specific date (format: "MM-DD-YYYY")
bool saveNetWorth(const char* date, int32_t netWorth);

// get percentage change comparing latest value to value from X days ago
// returns the percentage as a float (e.g., 5.25 for +5.25%) or 0.0 if not enough
float getPercentageChange(int daysAgo);

// get net worth history for last X days
// fills buffer with DailyNetWorth entries, oldest first (may be less than maxDays if not enough data)
int getNetWorthHistory(DailyNetWorth* buffer, int maxDays);

// get total number of records stored
int getRecordCount();

// get the most recent net worth value
bool getLatestNetWorth(DailyNetWorth& result);

#endif
