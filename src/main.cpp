#include <Arduino.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>
#include "fonts/FreeSansBold48pt7b.h"
#include <SPI.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <time.h>
#include <epd7c/GxEPD2_730c_GDEP073E01.h>
#include "helpers/display.h"
#include "helpers/format.h"
#include "helpers/power.h"
#include "helpers/api.h"
#include "helpers/database.h"
#include "credentials.h"
#include "configuration.h"

/*
  To do:
  - add no wifi icon to display if failed to connect to WiFi
  - add icon or indicator text to display if using cached values
*/

#define EPD_SCK   7
#define EPD_MOSI  9
#define EPD_CS    15
#define EPD_DC    11
#define EPD_RST   12
#define EPD_BUSY  13

#define SLEEP_DURATION_US (SLEEP_DURATION * 60 * 1000000ULL)

// NTP configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC (-5 * 3600) // (EST)
#define DAYLIGHT_OFFSET_SEC 3600 // 1 hour DST offset

Display display(GxEPD2_730c_GDEP073E01(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
SPIClass* spi;

RTC_DATA_ATTR int32_t netWorth = 0;
RTC_DATA_ATTR bool initialized = false;
RTC_DATA_ATTR char goldPrice[16] = "N/A";
RTC_DATA_ATTR char bitcoinPrice[16] = "N/A";
RTC_DATA_ATTR float percentChange = 0.0f;

bool connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected!");
    Serial.println("IP address: " + WiFi.localIP().toString());
    return true;
  } else {
    Serial.println(" Failed!");
    return false;
  }
}

void syncTime() {
  Serial.print("Syncing time with NTP...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // wait for time to sync (up to 10 seconds)
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo, 1000) && attempts < 10) {
    Serial.print(".");
    attempts++;
  }

  if (getLocalTime(&timeinfo, 100)) {
    Serial.println(" Time synced!");
    Serial.printf("Current time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    Serial.println(" Time sync failed!");
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected");
}

void updateScreen() {
  Serial.println("Refreshing screen...");

  display.setRotation(0);

  // calculate header banner dimensions
  String headerText = formatPossessive(OWNER_NAME) + " Net Worth";
  const int headerPadding = 20;
  display.setFont(&FreeSansBold24pt7b);
  int16_t x1, y1;
  uint16_t textW, textH;
  display.getTextBounds(headerText.c_str(), 0, 0, &x1, &y1, &textW, &textH);
  int bannerHeight = textH + (headerPadding * 2);
  bool lowBattery = isBatteryLow();

  // get sparkline historical data
  DailyNetWorth historyBuffer[SPARKLINE_DAYS];
  int historyCount = getNetWorthHistory(historyBuffer, SPARKLINE_DAYS);
  int32_t sparklineValues[SPARKLINE_DAYS];
  for (int i = 0; i < historyCount; i++) {
    sparklineValues[i] = historyBuffer[i].netWorth;
  }

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    display.fillRect(0, 0, 800, bannerHeight, GxEPD_BLACK); // black header banner
    display.fillRect(0, bannerHeight + 3, 800, 3, GxEPD_BLACK); // accent line beneath header

    // header title
    display.setFont(&FreeSansBold24pt7b);
    display.setTextColor(GxEPD_WHITE);
    drawText(display, headerText.c_str(), 400, bannerHeight / 2, HAlign::Center, VAlign::Center);

    // market prices - top right below header
    display.setFont(&FreeSansOblique9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int priceMarginRight = 15;
    int priceStartY = bannerHeight + 6 + 18;  // Below header + accent line + padding
    String goldText = String("Gold: ") + goldPrice;
    String btcText = String("Bitcoin: ") + bitcoinPrice;
    drawText(display, goldText.c_str(), 800 - priceMarginRight, priceStartY, HAlign::Right, VAlign::Top);
    drawText(display, btcText.c_str(), 800 - priceMarginRight, priceStartY + 22, HAlign::Right, VAlign::Top);

    // low battery warning pill - top left below header
    if (lowBattery) {
      const char* lowBattText = "LOW BATTERY";
      const int pillPaddingX = 12;
      const int pillPaddingY = 6;
      const int pillMargin = 10;
      const int pillRadius = 12;

      display.setFont(&FreeSansOblique9pt7b);
      int16_t bx1, by1;
      uint16_t btw, bth;
      display.getTextBounds(lowBattText, 0, 0, &bx1, &by1, &btw, &bth);

      int pillW = btw + (pillPaddingX * 2);
      int pillH = bth + (pillPaddingY * 2);
      int pillX = pillMargin;
      int pillY = bannerHeight + 6 + pillMargin;

      display.fillRoundRect(pillX, pillY, pillW, pillH, pillRadius, GxEPD_RED);
      display.setTextColor(GxEPD_WHITE);
      drawText(display, lowBattText, pillX + pillW / 2, pillY + pillH / 2, HAlign::Center, VAlign::Center);
    }

    // net worth value - center of screen
    display.setFont(&FreeSansBold48pt7b);
    display.setTextColor(GxEPD_BLACK);
    String netWorthStr = "N/A";
    if (netWorth > 0) {
      netWorthStr = formatCurrency(netWorth);
    }
    drawText(display, netWorthStr.c_str(), 400, 240, HAlign::Center, VAlign::Center);

    // percentage change indicator - centered below net worth
    display.setFont(&FreeSans12pt7b);
    bool isPositive = percentChange >= 0;
    uint16_t changeColor = isPositive ? GxEPD_GREEN : GxEPD_RED;

    String percentText = formatPercentage(percentChange) + " last 24 hours";
    int16_t px1, py1;
    uint16_t ptw, pth;
    display.getTextBounds(percentText.c_str(), 0, 0, &px1, &py1, &ptw, &pth);

    int changeY = 305;
    int triangleSize = 14;
    int triangleTextGap = 8;
    int totalWidth = triangleSize + triangleTextGap + ptw;
    int startX = 400 - (totalWidth / 2);

    // draw triangle indicator
    int triangleCenterX = startX + (triangleSize / 2);
    drawTriangle(display, triangleCenterX, changeY, triangleSize, isPositive, changeColor);

    // draw percentage text
    display.setTextColor(changeColor);
    int textX = startX + triangleSize + triangleTextGap;
    drawText(display, percentText.c_str(), textX, changeY, HAlign::Left, VAlign::Center);

    // goal projection - centered below percentage text
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);
    String goalProjection = getGoalProjection();
    if (goalProjection != "") {
      drawText(display, goalProjection.c_str(), 400, 345, HAlign::Center, VAlign::Center);
    }

    // sparkline - bottom left corner (historical trend)
    if (historyCount >= 7) {
      drawSparkLine(display, 15, 480 - 10 - 80, 240, 80, sparklineValues, historyCount);
    }

    // last updated time - bottom right
    display.setFont(&FreeSansOblique9pt7b);
    display.setTextColor(GxEPD_BLACK);
    String timeStr = getFormattedTime();
    drawText(display, timeStr.c_str(), 800 - 15, 480 - 10, HAlign::Right, VAlign::Bottom);
  } while (display.nextPage());

  Serial.println("Refresh Complete!");
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Waking up...");

  initBattery();
  Serial.printf("Battery: %.2fV (%d%%)\n", getBatteryVoltage(), getBatteryPercent());

  initDatabase();
  DailyNetWorth lastStored;
  if (!initialized && getLatestNetWorth(lastStored)) {
    netWorth = lastStored.netWorth;
    initialized = true;
    Serial.printf("Loaded stored net worth from %s: $%d\n", lastStored.date, netWorth);
  }

  if (connectWiFi()) {
    syncTime();

    int32_t fetchedNetWorth = fetchNetWorth();
    if (fetchedNetWorth != 0) {
      netWorth = fetchedNetWorth;
      initialized = true;

      saveNetWorth(getFormattedDate().c_str(), netWorth);

      // get percentage change (current day vs previous day)
      percentChange = getPercentageChange(1);
      Serial.printf("24h change: %.1f%%\n", percentChange);
    } else if (!initialized) {
      // API failed and first boot with no stored data, show 0
      netWorth = 0;
      Serial.println("API fetch failed on first boot, showing $0");
    } else {
      // API failed but we have a cached value, keep it
      Serial.printf("API fetch failed, using cached value: $%d\n", netWorth);
    }

    String fetchedGold = fetchGoldPrice();
    if (fetchedGold != "N/A") {
      strncpy(goldPrice, fetchedGold.c_str(), sizeof(goldPrice) - 1);
      goldPrice[sizeof(goldPrice) - 1] = '\0';
    }
    String fetchedBtc = fetchBitcoinPrice();
    if (fetchedBtc != "N/A") {
      strncpy(bitcoinPrice, fetchedBtc.c_str(), sizeof(bitcoinPrice) - 1);
      bitcoinPrice[sizeof(bitcoinPrice) - 1] = '\0';
    }
  } else if (!initialized) {
    // no WiFi and first boot with no stored data
    netWorth = 0;
    Serial.println("No WiFi on first boot, showing N/A");
  } else {
    Serial.printf("No WiFi, using cached value: $%d\n", netWorth);
  }

  pinMode(EPD_BUSY, INPUT);

  // initialize SPI - explicitly use SPI2 (FSPI) on ESP32-S3
  spi = new SPIClass(FSPI);
  spi->begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);

  Serial.println("Initializing display...");
  display.epd2.selectSPI(*spi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  display.init(115200, true, 2, false);

  updateScreen();

  // disconnect wifi before sleep to save power
  disconnectWiFi();
}

void loop() {
  Serial.println("Entering deep sleep for " + String(SLEEP_DURATION) + " minutes...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_deep_sleep_start();
}
