#ifndef HELPERS_API_H
#define HELPERS_API_H

#include <Arduino.h>

// fetches all assets from Lunch Money API and calculates total net worth
// returns the net worth in whole dollars (rounded) or 0 on error
int32_t fetchNetWorth();

// fetches current gold price from gold-api.com
// returns price as "$XXXX" string or "N/A" on error
String fetchGoldPrice();

// fetches current Bitcoin price from CoinGecko
// returns price as "$XXXXX" string or "N/A" on error
String fetchBitcoinPrice();

#endif
