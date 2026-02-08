#include "Ui.h"

Ui::Ui(int rstPin)
  : _u8g2(U8G2_R0, rstPin) {}

void Ui::begin() {
  _u8g2.begin();
  _u8g2.setFont(u8g2_font_6x12_tf);
}

void Ui::render(const char* timeStr, const Logger& log) {
  _u8g2.clearBuffer();

  _u8g2.drawStr(0, 12, "Heltec Master");
  _u8g2.setFont(u8g2_font_logisoso18_tf);
  _u8g2.drawStr(0, 36, timeStr);

  _u8g2.setFont(u8g2_font_6x12_tf);
  _u8g2.drawHLine(0, 40, 128);

  int y = 52;
  for (size_t i = 0; i < log.count(); i++) {
    _u8g2.drawStr(0, y, log.getOldestFirst(i).c_str());
    y += 12;
  }

  _u8g2.sendBuffer();
}
