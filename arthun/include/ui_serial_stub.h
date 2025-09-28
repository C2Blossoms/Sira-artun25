#pragma once
#include <Arduino.h>

namespace ui {
  inline void begin() {}
  inline void showHeader(const char* s){ Serial.print("[HDR] "); Serial.println(s); }
  inline void showValueLabel(const char* s){ Serial.print("[LBL] "); Serial.println(s); }
  inline void showValue(const String& s){ Serial.print("[VAL] "); Serial.println(s); }
  inline void showBuffer(const char* s){ Serial.print("[BUF] "); Serial.println(s); }
  inline void showPressed(char c){ Serial.print("[KEY] "); Serial.println(c); }
  inline void showSubmitted(const char* s){ Serial.print("[SUBMIT] "); Serial.println(s); }
  inline void footer_value() {}
  inline void footer_menu() {}
  inline void footer_paywait() {}
}

namespace oled {
  inline bool begin(){ return false; }
  inline bool isReady(){ return false; }
  inline void showWaiting() { Serial.println("[OLED] waiting..."); }
  inline void handleSerialQR() {} // ปิดการวาด QR
}
