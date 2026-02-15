#include "Ui.h"
#include "config.h"

Ui::Ui(int rstPin)
  : _u8g2(U8G2_R0, rstPin) {}

void Ui::begin() {
  _u8g2.begin();
  _u8g2.setFont(u8g2_font_6x12_tf);
}

void Ui::render(const char* timeStr, const char* posStr,
                float temperature, float humidity,
                const char* loraStatus,
                const char* loraTick) {
  _u8g2.clearBuffer();
  _u8g2.setFont(u8g2_font_6x12_tf);

  // Row 1: Title + time
  _u8g2.drawStr(0, 10, PROJECT_NAME);
  int timeWidth = _u8g2.getStrWidth(timeStr);
  _u8g2.drawStr(128 - timeWidth, 10, timeStr);
  _u8g2.drawHLine(0, 13, 128);

  // Row 2: Position
  _u8g2.drawStr(0, 25, posStr);

  // Row 3: LoRa status
  if (loraStatus) {
    _u8g2.drawStr(0, 37, loraStatus);
  }

  // Row 4: Temp + Humidity combined
  char buf[32];
  snprintf(buf, sizeof(buf), "%.1fC  %.0f%%", temperature, humidity);
  _u8g2.drawStr(0, 49, buf);

  // Row 5: LoRa tick (e.g. "RX: #42")
  if (loraTick) {
    _u8g2.drawStr(0, 61, loraTick);
  }

  _u8g2.sendBuffer();
}
