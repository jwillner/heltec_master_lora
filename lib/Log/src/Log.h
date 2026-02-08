#pragma once
#include <Arduino.h>
#include <vector>

class LogRing {
public:
  LogRing(size_t lines, size_t maxChars)
  : _lines(lines), _maxChars(maxChars), _buf(lines) {}

  void push(const String& s) {
    String x = s;
    if (x.length() > _maxChars) x = x.substring(0, _maxChars);

    _buf[_head] = x;
    _head = (_head + 1) % _lines;
    if (_count < _lines) _count++;
  }

  size_t count() const { return _count; }
  size_t capacity() const { return _lines; }

  // i=0 => älteste Zeile, i=count-1 => neueste
  String getOldestFirst(size_t i) const {
    if (i >= _count) return "";
    size_t start = (_count == _lines) ? _head : 0;
    size_t idx = (start + i) % _lines;
    return _buf[idx];
  }

private:
  size_t _lines;
  size_t _maxChars;
  std::vector<String> _buf;
  size_t _head = 0;
  size_t _count = 0;
};
 

