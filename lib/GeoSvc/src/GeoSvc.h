#pragma once
#include <Arduino.h>

namespace GeoSvc {
  void fetchOnce(void (*logFn)(const String&));
  float lat();
  float lng();
  float accuracy();
  bool valid();
  String position();   // "lat,lng"
}
