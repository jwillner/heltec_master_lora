#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// ===== UART Debug (funktioniert bei dir) =====
static constexpr int UART_RX_GPIO = 44; // D6
static constexpr int UART_TX_GPIO = 43; // D7
HardwareSerial uartDebug(1);

// ===== Wio-SX1262 (B2B) Pins =====
static constexpr int LORA_NSS   = 41;
static constexpr int LORA_DIO1  = 39;
static constexpr int LORA_RST   = 42;
static constexpr int LORA_BUSY  = 40;
static constexpr int LORA_ANTSW = 38;   // Antenna switch

// SPI pins on XIAO ESP32S3 for this kit (B2B wiring)
static constexpr int LORA_SCK  = 7;
static constexpr int LORA_MOSI = 9;
static constexpr int LORA_MISO = 8;

// Use explicit SPI instance
SPIClass spiLoRa(FSPI);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, spiLoRa);

static void die(int code) {
  uartDebug.print("FATAL: code=");
  uartDebug.println(code);
  while (true) delay(1000);
}

uint32_t counter = 0;

void setup() {
  uartDebug.begin(115200, SERIAL_8N1, UART_RX_GPIO, UART_TX_GPIO);
  delay(300);
  uartDebug.println();
  uartDebug.println("=== SX1262 SENDER (Wio/XIAO B2B) ===");

  pinMode(LORA_ANTSW, OUTPUT);
  digitalWrite(LORA_ANTSW, HIGH); // Sender: meist TX-Pfad aktiv (falls invertiert: LOW testen)

  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  spiLoRa.setFrequency(1000000); // 1 MHz zum Start

  int state = radio.begin(868.0, 125.0, 7, 5, 0x12, 14, 8);
  if (state != RADIOLIB_ERR_NONE) die(state);

  // Wio-SX1262 nutzt TCXO; 1.8V ist bei diesem Board relevant
  state = radio.setTCXO(1.8);
  if (state != RADIOLIB_ERR_NONE) die(state);

  uartDebug.println("Radio init OK");
}

void loop() {
  String msg = "Hello SX1262 #" + String(counter);

  // TX-Pfad (falls Ant-Switch nötig)
  digitalWrite(LORA_ANTSW, HIGH);

  uartDebug.print("TX -> ");
  uartDebug.println(msg);

  int state = radio.transmit(msg);
  if (state == RADIOLIB_ERR_NONE) {
    uartDebug.println("TX OK");
  } else {
    uartDebug.print("TX FAIL code=");
    uartDebug.println(state);
  }

  counter++;
  delay(2000);
}
