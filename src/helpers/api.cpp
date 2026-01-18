#include "api.h"
#include "format.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../credentials.h"

#define LUNCH_MONEY_ASSETS_URL "https://dev.lunchmoney.app/v1/assets"
#define LUNCH_MONEY_PLAID_URL "https://dev.lunchmoney.app/v1/plaid_accounts"
#define GOLD_API_URL "https://api.gold-api.com/price/XAU"
#define BITCOIN_API_URL "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd"

String fetchLunchMoneyEndpoint(const char* url) {
  HTTPClient http;

  http.begin(url);
  http.addHeader("Authorization", String("Bearer ") + LUNCH_MONEY_ACCESS_TOKEN);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET %s failed, code: %d\n", url, httpCode);
    http.end();
    return "";
  }

  String payload = http.getString();
  http.end();
  return payload;
}

int32_t fetchNetWorth() {
  double totalNetWorth = 0.0;

  // get manual assets
  Serial.println("Getting manual assets from Lunch Money...");
  String assetsPayload = fetchLunchMoneyEndpoint(LUNCH_MONEY_ASSETS_URL);

  if (assetsPayload.length() > 0) {
    JsonDocument assetsDoc;
    DeserializationError error = deserializeJson(assetsDoc, assetsPayload);

    if (error) {
      Serial.printf("Assets JSON parse error: %s\n", error.c_str());
    } else {
      JsonArray assets = assetsDoc["assets"].as<JsonArray>();

      for (JsonObject asset : assets) {
        const char* typeName = asset["type_name"];
        const char* name = asset["name"];
        const char* balanceStr = asset["balance"];

        if (!asset["closed_on"].isNull()) {
          Serial.printf("  Skipping closed asset: %s\n", name);
          continue;
        }

        double balance = atof(balanceStr);

        if (!asset["to_base"].isNull()) {
          balance = asset["to_base"].as<double>();
        }

        bool isLiability = (
          strcmp(typeName, "loan") == 0 ||
          strcmp(typeName, "credit") == 0 ||
          strcmp(typeName, "other liability") == 0
        );

        if (isLiability) {
          totalNetWorth -= abs(balance);
        } else {
          totalNetWorth += balance;
        }
      }
    }
  }

  // get plaid-synced accounts
  Serial.println("Getting Plaid accounts from Lunch Money...");
  String plaidPayload = fetchLunchMoneyEndpoint(LUNCH_MONEY_PLAID_URL);

  if (plaidPayload.length() > 0) {
    JsonDocument plaidDoc;
    DeserializationError error = deserializeJson(plaidDoc, plaidPayload);

    if (error) {
      Serial.printf("Plaid JSON parse error: %s\n", error.c_str());
    } else {
      JsonArray accounts = plaidDoc["plaid_accounts"].as<JsonArray>();

      for (JsonObject account : accounts) {
        const char* type = account["type"];
        const char* name = account["name"];
        const char* balanceStr = account["balance"];

        double balance = atof(balanceStr);

        if (!account["to_base"].isNull()) {
          balance = account["to_base"].as<double>();
        }

        bool isLiability = (strcmp(type, "credit") == 0 || strcmp(type, "loan") == 0);

        if (isLiability) {
          totalNetWorth -= abs(balance);
        } else {
          totalNetWorth += balance;
        }
      }
    }
  }

  return (int32_t)round(totalNetWorth);
}

String fetchGoldPrice() {
  HTTPClient http;
  http.begin(GOLD_API_URL);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Gold API request failed, code: %d\n", httpCode);
    http.end();
    return "N/A";
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.printf("Gold API JSON parse error: %s\n", error.c_str());
    return "N/A";
  }

  double price = doc["price"].as<double>();
  int32_t wholePrice = (int32_t)round(price);

  Serial.printf("Gold price: $%d\n", wholePrice);
  return formatCurrency(wholePrice);
}

String fetchBitcoinPrice() {
  HTTPClient http;
  http.begin(BITCOIN_API_URL);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("Bitcoin API request failed, code: %d\n", httpCode);
    http.end();
    return "N/A";
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.printf("Bitcoin API JSON parse error: %s\n", error.c_str());
    return "N/A";
  }

  double price = doc["bitcoin"]["usd"].as<double>();
  int32_t wholePrice = (int32_t)round(price);

  Serial.printf("Bitcoin price: $%d\n", wholePrice);
  return formatCurrency(wholePrice);
}
