#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
// #include <ArduinoJson.h>
#include "Config.h"
#include "rfid_wallet.h"
#include "keypad_input.h"
#include "ui_lcd.h"
#include "ui_oled.h"


const char* WIFI_SSID = "Tonliw";
const char* WIFI_PASS = "77777772";
const char* SERVER    = "http://172.20.10.6:8000";

KeypadInput keypad;
RFIDWallet  rfid;

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
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  Serial.begin(Cfg::SERIAL_BAUD);
  Serial.setRxBufferSize(2048);
  Serial.setTimeout(2000);

  // I2C (เผื่อ ui_oled ใช้ค่าใน Config เองก็ได้)
  Wire.begin(Cfg::I2C_SDA, Cfg::I2C_SCL);
  Wire.setClock(Cfg::I2C_HZ);

  while (!Serial) { delay(10); }
  Serial.println("[BOOT] Sira-arthun booting...");

  // Modules
  rfid.begin();
  keypad.begin();
  ui::begin();            // LCD 2004
  if (!oled::begin()) {   // OLED SSD1306
    Serial.println("[OLED] init failed");
  } else {
    oled::showWaiting();
  }

  Serial.println("[READY] Mode=IDLE. Press keys...");
}

void loop() {
  // อ่าน keypad
  if (keypad.poll()) {
    if (keypad.submitted()) {
      double amt = keypad.amount();
      if (!isnan(amt)) {
        Serial.print("[AMOUNT] "); Serial.println(amt, 2);
        // TODO: ทำ TOPUP/PAY ต่อไป
      }
      keypad.consumeSubmitted();
    }
  }

  // รับ payload ทาง Serial → วาด QR บน OLED
  oled::handleSerialQR();

  // … ส่วน RFID / workflow อื่น ๆ ใส่เพิ่มได้ …
}
