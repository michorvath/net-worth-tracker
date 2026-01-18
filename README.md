
# Desktop Net Worth Tracker

This personal net worth tracker parses financial data from the [Lunch Money API](https://lunchmoney.dev/#getting-started) to display on a **Seeed Studio reTerminal E1002** e-ink display.

#### A [Lunch Money](https://lunchmoney.app/) account is required for an API access token.

By default, it wakes from deep sleep 6 times a day to sync via WiFi.

 ---

### To get up and running:

- You'll need to install [PlatformIO](https://platformio.org/) either by cli or with the VSCode extension
- Create a [Lunch Money](https://lunchmoney.app/) account if you don't already have one, add static assets, link all your financial accounts via Plaid, and go to the [developers page](https://my.lunchmoney.app/developers) to "Request New Access Token"
- Copy the credential.h.example file and rename to credentials.h. Add your WiFi SSID and password then paste your personal Lunch Money access token.
- Next copy the configuration.h.example file and rename to configuration.h. Set your name for the header title and feel free to change the default low battery indicator threshold or deep sleep time.
- Connect the device by usb and run `pio run -t uploadfs` to initialize the LittleFS partition
- Now you should be good to build then upload to the device, the serial monitor should start immediately for debugging

> The battery should last for several months with the default 4 hour refresh rate. A more frequent refresh rate is unnecessary as Plaid only syncs so frequently and even if you have 6-8 accounts, the 4 hour window should catch different synchronizations as well as equity fluctuations.

A red low battery indicator pill will display on the top left of the display when you need to charge it.