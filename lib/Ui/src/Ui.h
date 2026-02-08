#pragma once
#include <U8g2lib.h>
#include "Logger.h"

class Ui {
public:
  Ui(int rstPin);
  void begin();
  void render(const char* timeStr, const Logger& log);

private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2;
};
