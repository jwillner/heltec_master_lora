#include "TimeSvc.h"
#include "config.h"
#include <WiFi.h>
#include <time.h>

namespace TimeSvc {

bool valid() {
  return time(nullptr) > TIME_VALID_EPOCH;
}

void formatHHMMSS(char* out, size_t len) {
  if (!valid()) {
    strncpy(out, "--:--:--", len);
    return;
  }
  time_t now = time(nullptr);
  struct tm t {};
  localtime_r(&now, &t);
  strftime(out, len, "%H:%M:%S", &t);
}

bool wifiConnect(const char* ssid, const char* pass, void (*logFn)(const String&)) {
  if (logFn) logFn("WLAN verbinden...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000)
    delay(250);

  if (WiFi.status() != WL_CONNECTED) {
    if (logFn) logFn("WLAN FAIL");
    WiFi.mode(WIFI_OFF);
    return false;
  }
  return true;
}

void wifiDisconnect(void (*logFn)(const String&)) {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  if (logFn) logFn("WLAN aus");
}

void ntpSync(const char* tzInfo, void (*logFn)(const String&)) {
  if (logFn) logFn("NTP sync...");
  setenv("TZ", tzInfo, 1);
  tzset();

  configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

  uint32_t start = millis();
  while (!valid() && millis() - start < 12000)
    delay(200);

  if (logFn) logFn(valid() ? "Zeit OK" : "NTP Timeout");
}

void syncOnce(const char* ssid,
              const char* pass,
              const char* tzInfo,
              void (*logFn)(const String&)) {
  if (!wifiConnect(ssid, pass, logFn)) return;
  ntpSync(tzInfo, logFn);
  wifiDisconnect(logFn);
}

}
