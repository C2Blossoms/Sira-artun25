// main.cpp
#include <Arduino.h>
#include "Config.h"
#include "ui_lcd.h"
#include "keypad_input.h"

KeypadInput keypadIn;

void setup() {
  Serial.begin(Cfg::SERIAL_BAUD);
  while (!Serial) {
    delay(10);
  }
  Serial.println("[BOOT] Sira-arthun booting...");

  // เริ่มจอ LCD ก่อน (มีข้อความบูต)
  ui::begin();

  keypadIn.begin();
  
  // ตั้งโหมดที่ต้องการเริ่ม (เปลี่ยนได้ตามงานจริง)
  keypadIn.setMode(KeypadInput::IDLE);

  Serial.println("[READY] Mode=IDLE. Press keys...");
}

void loop() {
  // อ่านคีย์ทุกลูป
  if (keypadIn.poll()) {
    if (keypadIn.submitted()) {
      // ไปขั้นถัดไปของ TOPUP ได้ตรงนี้ (เช่น ส่งค่าไปประมวลผล)
      // double amount = atof(keypadIn.buffer());
      // doTopup(amount);
      keypadIn.consumeSubmitted();
      keypadIn.clear(); // พร้อมรับค่าใหม่
    }

    // ปุ่มลัดเปลี่ยนโหมด (เลือกปุ่มที่ใช้งานได้ตอนนี้)
    char k = keypadIn.lastKey();
    if      (k == '1') keypadIn.setMode(KeypadInput::IDLE);
    else if (k == '2') keypadIn.setMode(KeypadInput::TOPUP);
    else if (k == '4') keypadIn.setMode(KeypadInput::PAY);
    else if (k == '5') keypadIn.setMode(KeypadInput::CHECK);
  }
}