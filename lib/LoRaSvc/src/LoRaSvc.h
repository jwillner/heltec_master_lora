#pragma once
#include <Arduino.h>

namespace LoRaSvc {
  bool begin(void (*logFn)(const String&));

  // Sender
  bool send(const uint8_t* data, size_t len);
  bool sendString(const String& msg);

  // Receiver (interrupt-based)
  void startReceive();
  bool available();
  String readString();
  float lastRssi();
  float lastSnr();
}
