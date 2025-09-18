#pragma once
#include <Arduino.h>

class KeypadInput {
public:
  enum Mode { IDLE, TOPUP, PAY, CHECK };

  void begin();
  // poll() จะคืน true เมื่อมีการอัปเดต (กด/ลบ/ยืนยัน)
  bool poll();

  // state
  Mode mode() const { return mode_; }
  void setMode(Mode m);

  // buffer ปัจจุบัน (ตัวเลข/ตัวที่พิมพ์)
  const char* buffer() const { return buf_; }
  bool submitted() const { return submitted_; }
  char lastKey() const { return lastKey_; }

  // ฟังก์ชันล้าง/รีเซ็ต
  void clear();
  void backspace();
  void consumeSubmitted() { submitted_ = false; } // เรียกหลังอ่านผลแล้ว

private:
  Mode mode_ = IDLE;

  static constexpr uint8_t MAXLEN = 16; // ให้พอดีกับ LCD 20 คอลัมน์
  char buf_[MAXLEN + 1] = {0};
  uint8_t len_ = 0;

  char lastKey_ = 0;
  bool submitted_ = false;
};
