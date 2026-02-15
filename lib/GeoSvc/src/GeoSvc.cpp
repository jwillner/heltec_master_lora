#include "GeoSvc.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace GeoSvc {

static float s_lat = 0;
static float s_lng = 0;
static bool  s_valid = false;

float lat()      { return s_lat; }
float lng()      { return s_lng; }
float accuracy() { return 0; }
bool  valid()    { return s_valid; }

String position() {
  return String(s_lat, 6) + "," + String(s_lng, 6);
}

void fetchOnce(void (*logFn)(const String&)) {
  if (logFn) logFn("Geo: request...");

  HTTPClient http;
  http.setTimeout(GEO_TIMEOUT_MS);
  http.begin(GEO_API_URL);

  int code = http.GET();
  if (code != 200) {
    if (logFn) logFn("Geo: HTTP " + String(code));
    http.end();
    return;
  }

  String resp = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    if (logFn) logFn("Geo: JSON err");
    return;
  }

  s_lat   = doc["lat"] | 0.0f;
  s_lng   = doc["lon"] | 0.0f;
  s_valid = true;

  if (logFn) logFn("Geo: " + position());
}

}
