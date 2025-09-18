#pragma once
#include <Arduino.h>

namespace ui {
  // init LCD
  void begin();

  // แสดงหัวข้อ/โหมด
  void showHeader(const char* modeText);

  // แสดงปุ่มที่เพิ่งกด
  void showPressed(char k);

  // แสดง buffer ปัจจุบัน
  void showBuffer(const char* buf);

  // แสดงผลเมื่อกด #
  void showSubmitted(const char* buf);

  // TOPUP/PAY DISPLAY
  void showValueLabel(const char* label);
  void showValue(const char* value);
  void showFooterActions();

}
