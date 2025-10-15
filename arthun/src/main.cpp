// ===== Debug switch =====
#ifndef DBG
#define DBG 1  // ตั้งเป็น 0 ถ้าไม่อยากให้พิมพ์ log
#endif
#define DBGPRINT(...) do { if (DBG) Serial.printf(__VA_ARGS__); } while(0)

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "ui_lcd.h"
#include "ui_oled.h"
#include "keypad_input.h"
#include "rfid_wallet.h"

using Cfg::beepOK;
using Cfg::beepErr;
using Cfg::beepWait;

KeypadInput keypad;
RFIDWallet  rfid;

enum AppState : uint8_t {
  ST_HOME,
  ST_CHECK_WAIT_CARD,
  ST_TOPUP_WAIT_CARD,
  ST_TOPUP_ENTER_AMOUNT,
  ST_TOPUP_SHOW_QR
};
AppState state_ = ST_HOME;
KeypadInput::Mode lastMode_ = KeypadInput::HOME;

uint32_t topupArmUntil = 0;       // รอช่วงกันเด้งก่อนยอมรับการแตะ
String   lastUIDSeen     = "";
uint32_t lastUIDAt       = 0;

uint32_t qrKeysIgnoreUntil = 0;   // กันคีย์ค้างตอนเข้าหน้า QR

bool   oledReady = false;
String currentUID;

static inline void clearOledQR() { if (oledReady) oled::clear(); }

// ---------- TCP TEST (loop-until-pass) ----------
static bool parseHostPortFromUrl(const char* url, String& host, uint16_t& port) {
  host = url;
  int p = host.indexOf("://");                // ตัด schema
  if (p >= 0) host = host.substring(p + 3);
  int slash = host.indexOf('/');              // ตัด path
  if (slash >= 0) host = host.substring(0, slash);
  int colon = host.indexOf(':');              // แยก host:port
  if (colon >= 0) { port = (uint16_t)host.substring(colon + 1).toInt(); host = host.substring(0, colon); }
  else port = 80;
  return host.length() > 0;
}

static bool tcpOnce(const char* host, uint16_t port, uint32_t timeoutMs = 3000) {
  WiFiClient c;
  c.setTimeout(timeoutMs);
  Serial.printf("[TCP] connect %s:%u ... ", host, port);
  if (!c.connect(host, port, timeoutMs)) {
    Serial.println("FAIL(connect)");
    return false;
  }
  Serial.println("OK(connect)");
  // ยิง HTTP ง่ายๆ แล้วเช็คบรรทัดแรก
  c.print("GET / HTTP/1.1\r\nHost: ");
  c.print(host);
  c.print("\r\nConnection: close\r\n\r\n");

  uint32_t t0 = millis();
  while (!c.available() && millis() - t0 < timeoutMs) { delay(10); yield(); }

  if (!c.available()) {
    Serial.println("[TCP] no data (timeout)");
    c.stop();
    return false;
  }
  // อ่านบรรทัดแรกไว้ดูสถานะ
  String line = c.readStringUntil('\n');
  line.trim();
  Serial.print("[TCP] first line: "); Serial.println(line);
  c.stop();
  return true; // มองว่า “ผ่าน” เมื่อเชื่อมได้และมี data กลับ
}

// เรียกอันนี้เพื่อ “วนจนกว่าจะผ่าน”
static void tcpProbeApiBaseUntilOk() {
  String host; uint16_t port;
  if (!parseHostPortFromUrl(Cfg::API_BASE, host, port)) {
    Serial.println("[TCP] parse API_BASE failed");
    return;
  }

  uint32_t backoff = 1000;              // เริ่มรอ 1s
  const uint32_t backoffMax = 10000;    // สูงสุด 10s

  Serial.printf("[TCP] probing %s:%u until pass ...\n", host.c_str(), port);
  for (uint32_t attempt = 1;; ++attempt) {
    // ถ้า Wi-Fi หลุดให้รีคอนเนกต์สั้นๆ
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] reconnecting...");
      WiFi.reconnect();
      uint32_t t0 = millis();
      while (millis() - t0 < 3000 && WiFi.status() != WL_CONNECTED) { delay(100); yield(); }
    }

    bool ok = (WiFi.status() == WL_CONNECTED) && tcpOnce(host.c_str(), port, 3000);
    if (ok) {
      Serial.printf("[TCP] PASS on attempt %u\n", attempt);
      Cfg::beepOK();
      break;
    } else {
      Serial.printf("[TCP] FAIL (attempt %u), retry in %lus\n", attempt, backoff/1000);
      Cfg::beepErr();
      // (ถ้ามี LCD) โชว์สถานะสั้นๆ
      ui::showHeader("TCP FAIL, retry...");
      uint32_t t0 = millis();
      while (millis() - t0 < backoff) { delay(50); yield(); }
      if (backoff < backoffMax) backoff += 1000; // เพิ่มทีละ 1s จนถึง 10s
    }
  }
}

// ---------- Network helpers ----------
static void wifiConnect() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(Cfg::WIFI_SSID, Cfg::WIFI_PASS);
  ui::showHeader("WiFi connecting...");
  for (int i=0; i<60 && WiFi.status()!=WL_CONNECTED; ++i) delay(200);
  if (WiFi.status()==WL_CONNECTED) { ui::showHeader("WiFi OK"); beepOK(); }
  else { ui::showHeader("WiFi FAIL"); beepErr(); }
}

static bool httpGetJson(const String& url, JsonDocument& doc, uint32_t timeoutMs=7000) {
  wifiConnect();
  for (int attempt = 0; attempt < 3; ++attempt) {
    HTTPClient http; 
    WiFiClient client;
    http.setTimeout(timeoutMs);
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    DBGPRINT("[HTTP] URL: %s\n", url.c_str());
    if (!http.begin(client, url)) { 
      DBGPRINT("[HTTP] begin() fail, retry...\n");
      delay(200); 
      continue; 
    }

    int code = http.GET();
    DBGPRINT("[HTTP] attempt=%d code=%d (%s)\n", attempt+1, code, http.errorToString(code).c_str());
    if (code > 0) {
      String body = http.getString();
      DBGPRINT("[HTTP] body_len=%d\n", body.length());
      if (body.length() > 0) {
        // พิมพ์ตัวอย่างเนื้อ JSON ไม่เกิน 200 ตัวอักษร (อ่านง่าย)
        String sample = body.substring(0, 200);
        DBGPRINT("[HTTP] body: %s\n", sample.c_str());
        DeserializationError err = deserializeJson(doc, body);
        if (!err) { 
          http.end(); 
          return true; 
        }
        DBGPRINT("[HTTP] JSON err: %s\n", err.c_str());
      } else {
        DBGPRINT("[HTTP] JSON err: EmptyInput\n");
      }
    }
    http.end();
    delay(300);
  }
  return false;
}

// GET /balance/{uid}
static bool apiGetBalance(const String& uid, float& outBalance) {
  String url = String(Cfg::API_BASE) + "/balance/" + uid;
  DBGPRINT("[CHECK] uid=%s\n", uid.c_str());

  JsonDocument doc;
  if (!httpGetJson(url, doc)) return false;

  JsonVariantConst v = doc["balance"];
  if (v.isNull()) return false;
  outBalance = v.as<float>();
  DBGPRINT("[CHECK] balance=%.2f\n", outBalance);
  return true;
}

// GET /create-qr-matrix/{uid}/{amount}
static bool apiCreateTopupQR(const String& uid, double amount, String& outUrl) {
  String url = String(Cfg::API_BASE) + "/create-qr-matrix/" + uid + "/" + String(amount, 2);
  DBGPRINT("[TOPUP] uid=%s amount=%.2f\n", uid.c_str(), amount);

  JsonDocument doc;
  if (!httpGetJson(url, doc)) return false;

  JsonVariantConst v = doc["topup_url"];
  if (!v.is<const char*>()) return false;
  outUrl = String(v.as<const char*>());
  DBGPRINT("[TOPUP] qr_url=%s\n", outUrl.c_str());
  return true;
}

void setup() {
  Serial.begin(Cfg::SERIAL_BAUD);
  delay(200);

  // Buzzer
  Cfg::initBuzzer();

  ui::begin();                     
  oledReady = oled::begin();
  if (oledReady) { oled::showBoot("ESP32 ready", "OLED OK"); }

  wifiConnect();

  // RFID + Keypad
  rfid.begin();
  keypad.begin();

  tcpProbeApiBaseUntilOk();

  ui::showHeader("Mode: MENU");
  ui::footer_menu();               
  keypad.setMode(KeypadInput::HOME);
}

void loop() {
  // อ่านคีย์แพด
  if (keypad.poll()) {
    if (keypad.lastKey() != '*') beepWait(); // ไม่ดังตอน submit
  }

  // เปลี่ยนโหมดตาม Keypad (A/B/D)
  KeypadInput::Mode now = keypad.mode();
  if (now != lastMode_) {
    if (state_ == ST_TOPUP_SHOW_QR) clearOledQR();
    lastMode_ = now;
    if (now == KeypadInput::HOME)   state_ = ST_HOME;
    if (now == KeypadInput::CHECK)  state_ = ST_CHECK_WAIT_CARD;
    if (now == KeypadInput::TOPUP)  {
      state_ = ST_TOPUP_WAIT_CARD;
      currentUID = "";
      // ui::showHeader("TOPUP: แตะบัตร");  // <-- LCD ไม่มีฟอนต์ไทย
      ui::showHeader("TOPUP: tap card");     // <-- ใช้ภาษาอังกฤษแทน
      ui::clearLine(1);
      ui::clearLine(2);
      topupArmUntil = millis() + 1200;
    }
  }

  // รอแตะบัตรในโหมด CHECK/TOPUP
  String uid;
  if ((state_ == ST_CHECK_WAIT_CARD || state_ == ST_TOPUP_WAIT_CARD) && rfid.pollCard(uid)) {

    if (state_ == ST_TOPUP_WAIT_CARD) {
      // กันเด้ง: เพิ่งเข้า TOPUP ต้องยกบัตรออกก่อน
      if (millis() < topupArmUntil) {
        ui::showHeader("Remove card & re-tap");
        // ไม่รับบัตรในช่วงนี้
      }
      // กันแตะ UID เดิมเร็วเกินไป (ยังวางบัตรค้างอยู่)
      else if (uid == lastUIDSeen && (millis() - lastUIDAt) < 1000) {
        ui::showHeader("Remove card & re-tap");
      }
      else {
        currentUID  = uid;
        lastUIDSeen = uid;
        lastUIDAt   = millis();
        rfid.halt();
        beepOK();
        ui::showCardUID(currentUID);

        state_ = ST_TOPUP_ENTER_AMOUNT;
        keypad.setMode(KeypadInput::TOPUP);
      }
    }

    else { // ST_CHECK_WAIT_CARD
      currentUID  = uid;
      lastUIDSeen = uid;
      lastUIDAt   = millis();
      rfid.halt();
      beepOK();
      ui::showCardUID(currentUID);

      float baht = NAN;
      keypad.showCheckUID(currentUID);
      if (apiGetBalance(currentUID, baht)) {
        keypad.showCheckBalance(baht);
      } else {
        ui::showHeader("Balance FAIL");
        beepErr();
      }
    }
  } 

  // โหมด TOPUP: กรอกจำนวนเงินแล้วกด *
  if (state_ == ST_TOPUP_ENTER_AMOUNT && keypad.takeSubmitted()) {
    double amt = keypad.amount();
    ui::showHeader("Waiting QR...");
    ui::clearLine(1);
    ui::clearLine(2);
    ui::clearLine(3);
    ui::showValueLabel("Amount:");
    ui::showValue(String(amt, 2).c_str());

    String qrUrl;
    if (apiCreateTopupQR(currentUID, amt, qrUrl)) {
      ui::showHeader("Scan QR now");
      ui::showValueLabel("Amount:");
      ui::showValue(String(amt, 2).c_str());
      ui::footer_paywait();
      if (oledReady) {
        oled::drawQR(qrUrl.c_str());
      } else { 
        Serial.print("[QR] "); Serial.println(qrUrl); 
      }
      state_ = ST_TOPUP_SHOW_QR;

      // เพิ่มบล็อกกันเด้งคีย์ ↓↓↓
      keypad.consumeSubmitted();     // ล้าง flag submit
      (void)keypad.takeLastKey();    // กินคีย์ '*' ที่เพิ่งกด
      qrKeysIgnoreUntil = millis() + 250;  // 0.25s แรกไม่อ่านคีย์

      beepOK();
    } else {
      ui::showHeader("QR request FAIL");
      ui::clearLine(1);
      ui::showValueLabel("Amount:");
      ui::showValue(String(amt, 2).c_str());
      ui::footer_payfail();  // หรือ ui::footer_value() ก็ได้ตาม UX ที่ต้องการ
      beepErr();
    }
  }

  // หน้า QR: จัดการปุ่มในโหมดแสดง QR
  if (state_ == ST_TOPUP_SHOW_QR) {
    // อ่านคีย์แบบ one-shot และข้ามถ้ายังอยู่ในช่วง ignore
    char k = (millis() >= qrKeysIgnoreUntil) ? keypad.takeLastKey() : 0;

    if (k == 'C') {
      clearOledQR();
      state_ = ST_TOPUP_ENTER_AMOUNT;
      keypad.setMode(KeypadInput::TOPUP);

      ui::showHeader("Enter amount");
    ui::showValueLabel("Topup value:");
    ui::showValue("");
    ui::footer_value();
    }
    else if (k == '#') {
      if (oledReady) {
        oled::clear();
        oled::showBoot("Topup", "Success");
      }
      ui::showHeader("Topup DONE");
      ui::clearLine(1);
      ui::clearLine(2);
      ui::clearLine(3);
      beepOK();
      
      delay(3000);

      currentUID = "";
      state_ = ST_HOME;
      keypad.setMode(KeypadInput::HOME);
    }
    else if (k == 'D') {
      clearOledQR();
      state_ = ST_HOME;
      keypad.setMode(KeypadInput::HOME);
      currentUID = "";
      ui::showHeader("Mode: MENU");
      ui::footer_menu();
      oled::clear();
      beepOK();
    }
  }

  // ทดสอบวาด QR ผ่าน Serial (พิมพ์: PAYLOAD:<url>)
  oled::handleSerialQR();

  delay(5);
}
