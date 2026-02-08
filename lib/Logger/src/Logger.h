#pragma once
#include <Arduino.h>

class Logger {
public:
  Logger(size_t lines);
  void begin(uint32_t baud);
  void log(const String& s);
  size_t count() const;
  String getOldestFirst(size_t i) const;

private:
  size_t _lines;
  String* _buf;
  size_t _head = 0;
  size_t _count = 0;
};
