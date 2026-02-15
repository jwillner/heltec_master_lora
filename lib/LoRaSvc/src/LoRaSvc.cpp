#include "LoRaSvc.h"
#include "config.h"
#include <SPI.h>
#include <RadioLib.h>

#if defined(ROLE_OBSERVER)
  // Wio-SX1262: eigene SPI-Instanz (B2B-Verkabelung)
  static SPIClass spiLoRa(FSPI);
  static SX1262 radio = new Module(LORA_PIN_NSS, LORA_PIN_DIO1,
                                   LORA_PIN_RST, LORA_PIN_BUSY, spiLoRa);
#else
  static SX1262 radio = new Module(LORA_PIN_NSS, LORA_PIN_DIO1,
                                   LORA_PIN_RST, LORA_PIN_BUSY);
#endif

static volatile bool rxFlag = false;
static float _rssi = 0;
static float _snr  = 0;

static void onReceive() {
  rxFlag = true;
}

bool LoRaSvc::begin(void (*logFn)(const String&)) {
  logFn("[LoRa] init SX1262...");

#if defined(ROLE_OBSERVER)
  // Wio-SX1262: SPI und Antenna-Switch initialisieren
  spiLoRa.begin(LORA_SPI_SCK, LORA_SPI_MISO, LORA_SPI_MOSI, LORA_PIN_NSS);
  spiLoRa.setFrequency(1000000);

  pinMode(LORA_PIN_ANTSW, OUTPUT);
  digitalWrite(LORA_PIN_ANTSW, LOW);   // RX-Pfad aktiv
#endif

  int state = radio.begin(
    LORA_FREQUENCY,
    LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR,
    LORA_CODING_RATE,
    LORA_SYNC_WORD,
    LORA_TX_POWER,
    LORA_PREAMBLE_LEN
  );

  if (state != RADIOLIB_ERR_NONE) {
    logFn("[LoRa] init FAIL: " + String(state));
    return false;
  }

  state = radio.setTCXO(LORA_TCXO_VOLTAGE);
  if (state != RADIOLIB_ERR_NONE) {
    logFn("[LoRa] TCXO FAIL: " + String(state));
    return false;
  }

#if LORA_USE_DIO2_RFSWITCH
  // Heltec V3: DIO2 als RF switch
  state = radio.setDio2AsRfSwitch(true);
  if (state != RADIOLIB_ERR_NONE) {
    logFn("[LoRa] DIO2 switch FAIL: " + String(state));
    return false;
  }
#endif

  logFn("[LoRa] ready");
  return true;
}

bool LoRaSvc::send(const uint8_t* data, size_t len) {
  int state = radio.transmit(data, len);
  return (state == RADIOLIB_ERR_NONE);
}

bool LoRaSvc::sendString(const String& msg) {
  String copy = msg;
  int state = radio.transmit(copy);
  return (state == RADIOLIB_ERR_NONE);
}

void LoRaSvc::startReceive() {
  radio.setPacketReceivedAction(onReceive);
  radio.startReceive();
}

bool LoRaSvc::available() {
  return rxFlag;
}

String LoRaSvc::readString() {
  rxFlag = false;
  String str;
  int state = radio.readData(str);
  if (state == RADIOLIB_ERR_NONE) {
    _rssi = radio.getRSSI();
    _snr  = radio.getSNR();
  }
  radio.startReceive();
  return str;
}

float LoRaSvc::lastRssi() { return _rssi; }
float LoRaSvc::lastSnr()  { return _snr; }
