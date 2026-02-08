#include "Board.h"

namespace Board {

void vextOn() {
  pinMode(VEXT_CTRL_PIN, OUTPUT);
  digitalWrite(VEXT_CTRL_PIN, VEXT_ON_LEVEL);
  delay(100);
}

void i2cBeginForOled() {
  Wire.begin(OLED_I2C_SDA, OLED_I2C_SCL);
  Wire.setClock(I2C_FREQUENCY);
}

void oledResetPulse() {
  pinMode(OLED_RST_PIN, OUTPUT);
  digitalWrite(OLED_RST_PIN, LOW);
  delay(50);
  digitalWrite(OLED_RST_PIN, HIGH);
  delay(50);
}

}
