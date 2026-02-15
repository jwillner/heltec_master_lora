#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// UART Debug
static constexpr int UART_RX_GPIO = 44; // D6
static constexpr int UART_TX_GPIO = 43; // D7
HardwareSerial uartDebug(1);

// Wio-SX1262 (B2B) Pins
static constexpr int LORA_NSS   = 41;
static constexpr int LORA_DIO1  = 39;
static constexpr int LORA_RST   = 42;
static constexpr int LORA_BUSY  = 40;
static constexpr int LORA_ANTSW = 38;

// SPI pins (B2B)
static constexpr int LORA_SCK  = 7;
static constexpr int LORA_MOSI = 9;
static constexpr int LORA_MISO = 8;

SPIClass spiLoRa(FSPI);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, spiLoRa);

// Flag wird im Interrupt gesetzt
volatile bool rxFlag = false;

void setRxFlag(void) {
  rxFlag = true;
}

static void die(int code) {
  uartDebug.print("FATAL: code=");
  uartDebug.println(code);
  while (true) delay(1000);
}

void setup() {
  uartDebug.begin(115200, SERIAL_8N1, UART_RX_GPIO, UART_TX_GPIO);
  delay(300);
  uartDebug.println();
  uartDebug.println("=== SX1262 RECEIVER (CONTINUOUS RX) ===");

  pinMode(LORA_ANTSW, OUTPUT);
  digitalWrite(LORA_ANTSW, LOW); // falls invertiert: HIGH testen

  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  spiLoRa.setFrequency(1000000);

  int state = radio.begin(868.0, 125.0, 7, 5, 0x12, 14, 8);
  if (state != RADIOLIB_ERR_NONE) die(state);

  state = radio.setTCXO(1.8);
  if (state != RADIOLIB_ERR_NONE) die(state);

  // ISR auf DIO1 (RX done)
  radio.setDio1Action(setRxFlag);

  // In continuous receive gehen
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) die(state);

  uartDebug.println("Radio init OK – continuous RX started");
}

void loop() {
  if (!rxFlag) {
    return;
  }
  rxFlag = false;

  String msg;
  int state = radio.readData(msg);

  if (state == RADIOLIB_ERR_NONE) {
    uartDebug.print("RX <- ");
    uartDebug.print(msg);
    uartDebug.print(" | RSSI=");
    uartDebug.print(radio.getRSSI());
    uartDebug.print(" dBm | SNR=");
    uartDebug.print(radio.getSNR());
    uartDebug.println(" dB");
  } else {
    uartDebug.print("readData ERR code=");
    uartDebug.println(state);
  }

  // Direkt wieder in RX (wichtig!)
  state = radio.startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    uartDebug.print("startReceive ERR code=");
    uartDebug.println(state);
  }
}
