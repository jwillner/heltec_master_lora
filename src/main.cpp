#include <Arduino.h>
#include "config.h"

#include "Board.h"
#include "Logger.h"
#include "TimeSvc.h"
#include "Ui.h"
#include "BleSvc.h"


BleSvc ble;

Logger logger(LOGGER_LINES);
Ui ui(OLED_RST_PIN);

static void logFn(const String& s) { logger.log(s); }

void setup() {
  logger.begin(SERIAL_BAUDRATE);

  Board::vextOn();
  Board::oledResetPulse();
  Board::i2cBeginForOled();

  ui.begin();
#if BLE_ENABLED
  ble.begin(logger);
#endif

  logger.log(PROJECT_NAME " v" PROJECT_VERSION);

#if WIFI_ENABLED
  TimeSvc::syncOnce(WIFI_SSID, WIFI_PASSWORD, TZ_INFO, logFn);
#else
  logger.log("WLAN disabled");
#endif
}

void loop() {
  static uint32_t lastLog = 0;

  char timeBuf[16];
  TimeSvc::formatHHMMSS(timeBuf, sizeof(timeBuf));

  if (millis() - lastLog > LOGGER_TICK_MS) {
    lastLog = millis();
    logger.log(String("Tick ms=") + millis());
  }

  ui.render(timeBuf, logger);
  delay(50);
}
