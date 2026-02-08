#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

namespace Board {
  void vextOn();
  void i2cBeginForOled();
  void oledResetPulse();
}
