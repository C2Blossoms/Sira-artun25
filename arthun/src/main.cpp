// main.cpp
#include <Arduino.h>
#include <Wire.h>
#include "Config.h"
#include "rfid_wallet.h"
#include "ui_lcd.h"
#include "keypad_input.h"

KeypadInput keypad;
RFIDWallet rfid;

static bool waitForCard(String &uid, uint32_t timeoutMs = 10000) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (rfid.pollCard(uid)) { rfid.halt(); return true; }
    delay(20);
  }
  return false;
}

static void showBahtOnLCD(const char* label, int32_t baht) {
  char line[24];
  snprintf(line, sizeof(line), "%ld บาท", (long)baht);
  ui::showValueLabel(label);
  ui::showValue(line);
}

void setup() {
  Serial.begin(Cfg::SERIAL_BAUD);
 
  Wire.begin(Cfg::I2C_SDA, Cfg::I2C_SCL);
  Wire.setClock(Cfg::I2C_HZ);

  while (!Serial) {
    delay(10);
  }
  Serial.println("[BOOT] Sira-arthun booting...");

  // IoT begin
  rfid.begin();
  keypad.begin();
  ui::begin();
  
  // ตั้งโหมดที่ต้องการเริ่ม (เปลี่ยนได้ตามงานจริง)
  // keypad.setMode(KeypadInput::MENU);

  Serial.println("[READY] Mode=IDLE. Press keys...");
}

void loop() {
  if (keypad.poll()) {
    if (keypad.submitted()) {
      double amt = keypad.amount();
      if (!isnan(amt)) {
        Serial.print("[AMOUNT] ");
        Serial.println(amt, 2);
        // TODO: ดำเนินการ TOPUP/PAY ต่อ
      }
      keypad.consumeSubmitted();
    }
  }
  
}