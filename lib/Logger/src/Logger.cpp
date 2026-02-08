#include "Logger.h"

Logger::Logger(size_t lines) : _lines(lines) {
  _buf = new String[_lines];
}

void Logger::begin(uint32_t baud) {
  Serial.begin(baud);
  delay(800);
}

void Logger::log(const String& s) {
  Serial.println(s);
  _buf[_head] = s;
  _head = (_head + 1) % _lines;
  if (_count < _lines) _count++;
}

size_t Logger::count() const {
  return _count;
}

String Logger::getOldestFirst(size_t i) const {
  if (i >= _count) return "";
  size_t start = (_count == _lines) ? _head : 0;
  return _buf[(start + i) % _lines];
}
