#pragma once
#include <Arduino.h>
#include "Logger.h"

// BLE Service: Nordic UART Service (Advertising only, Step 2.5)
class BleSvc {
public:
  // Initialisiert BLE und startet Advertising
  // Rückgabe: true = ok, false = Fehler
  bool begin(Logger& log);
};
