// ui_serial.h
#pragma once
#include <Arduino.h>
namespace UI {
  void banner();
  void money(int32_t sat);
  void status(const char* modeName, const String& amountStr,
              const String& uidHex, int32_t balanceSatang);
}
