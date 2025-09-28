#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "rfid_wallet.h"
#include "keypad_input.h"
#include "ui_serial_stub.h"     // <— ใช้สตับแทนจอ
// #include "ui_lcd.h"
// #include "ui_oled.h"

const char* WIFI_SSID = "Tonliw";
const char* WIFI_PASS = "77777772";
const char* SERVER    = "http://172.20.10.6:8000";

KeypadInput keypad;
RFIDWallet  rfid;

// ---- เพิ่ม prototype ให้คอมไลเลอร์เห็นก่อนเรียก ----
static bool waitForCard(String &uid, uint32_t timeoutMs = 10000);

// ---- พิมพ์ยอดลง Serial แทนจอ ----
static void showBahtOnLCD(const char* label, int32_t baht) {
  Serial.printf("[BAHT] %s %ld บาท\n", (label?label:""), (long)baht);
}

// ---- helpers HTTP + API (ใช้ JsonDocument เพื่อลด warning) ----
static bool httpGet(const String& url, String& out) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http; http.begin(url);
  int code = http.GET(); out = http.getString(); http.end();
  return code == 200;
}

static bool httpPost(const String& url, String& out) {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http; http.begin(url);
  int code = http.POST(""); out = http.getString(); http.end();
  return code == 200;
}

static float apiGetBalance(const String& cardId) {
  String body; if (!httpGet(String(SERVER)+"/balance/"+cardId, body)) return -1;
  JsonDocument doc; DeserializationError e = deserializeJson(doc, body);
  if (e) return -1;
  return doc["balance"] | -1;
}

static bool apiTopup(const String& cardId, double amount, float& newBal) {
  String body; String url=String(SERVER)+"/topup/"+cardId+"/"+String(amount,2);
  if (!httpPost(url, body)) return false;
  JsonDocument doc; if (deserializeJson(doc, body)) return false;
  newBal = doc["balance"] | -1; return true;
}

static bool apiPay(const String& cardId, double amount, int vendorId, float& newBal) {
  String body; String url=String(SERVER)+"/pay/"+cardId+"/"+String(amount,2)+"/"+String(vendorId);
  if (!httpPost(url, body)) return false;
  JsonDocument doc; if (deserializeJson(doc, body)) return false;
  newBal = doc["balance"] | -1; return true;
}

// ---- นิยามจริงของ waitForCard ----
// static bool waitForCard(String &uid, uint32_t timeoutMs) {
//   uint32_t t0 = millis();
//   while (millis() - t0 < timeoutMs) {
//     if (rfid.pollCard(uid)) { rfid.halt(); return true; }
//     delay(20);
//   }
//   return false;
// }

static bool waitForCard(String &uid, uint32_t /*timeoutMs*/ ) {
  uid = "TEST1234";   // ใส่อะไรก็ได้ แต่ต้องตรงกับ DB
  return true;
}

void setup() {
  Serial.begin(Cfg::SERIAL_BAUD);
  while (!Serial) { delay(10); }
  Serial.println("[BOOT] Sira-arthun booting...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println("\nWiFi connected");

  Wire.begin(Cfg::I2C_SDA, Cfg::I2C_SCL);
  Wire.setClock(Cfg::I2C_HZ);

  rfid.begin();
  keypad.begin();
  ui::begin();
  oled::showWaiting();

  Serial.println("[READY] Mode=IDLE. Press keys...");
}

void loop() {
  if (keypad.poll()) {
    if (keypad.submitted()) {
      double amt = keypad.amount();
      KeypadInput::Mode m = keypad.mode();
      keypad.consumeSubmitted();

      if (!isnan(amt)) {
        Serial.printf("[AMT] %.2f (%s)\n", amt,
          m==KeypadInput::TOPUP?"TOPUP": m==KeypadInput::PAY?"PAY":"OTHER");

        Serial.println("[INFO] แตะบัตร...");
        String uid;
        if (!waitForCard(uid, 10000)) {
          Serial.println("[ERR] ไม่พบการ์ด");
          return;
        }
        Serial.printf("[CARD] UID=%s\n", uid.c_str());

        float newBal = -1;
        bool ok = false;
        if (m == KeypadInput::TOPUP) {
          ok = apiTopup(uid, amt, newBal);
        } else if (m == KeypadInput::PAY) {
          const int vendorId = 10;
          ok = apiPay(uid, amt, vendorId, newBal);
        } else if (m == KeypadInput::CHECK) {
          newBal = apiGetBalance(uid);
          ok = (newBal >= 0);
        }

        if (ok && newBal >= 0) {
          Serial.printf("[OK] New balance = %.2f\n", newBal);
          showBahtOnLCD("ยอดใหม่", (int32_t)newBal);
        } else {
          Serial.println("[ERR] ทำรายการไม่สำเร็จ");
        }
      }
    }
  }

  // ปิดการวาด QR บน OLED ในโหมด serial-only
  // oled::handleSerialQR();  // (ปิดไว้)
}
