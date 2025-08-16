#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "secrets.h"
#include "rfid.h"      // อ่าน UID ด้วย RC522
#include "oled.h"      // แสดงผล
// #include "keypad.h"    // ใส่/อ่านจำนวนเงิน (ภายหลัง)
#include "http_client.h"

#ifndef DEVICE_ROLE
#define DEVICE_ROLE "POS"
#endif
#ifndef DEVICE_ID
#define DEVICE_ID "DEV_XX"
#endif
#ifndef API_BASE_URL
#define API_BASE_URL "http://192.168.1.50:5000"
#endif
#ifndef API_KEY
#define API_KEY ""
#endif
#ifndef OTA_HOST
#define OTA_HOST "rfidpay"
#endif

static void setup_ota(){
  ArduinoOTA.setHostname(OTA_HOST);
  ArduinoOTA.onStart([](){ Serial.println("OTA Start"); });
  ArduinoOTA.onEnd([](){ Serial.println("\nOTA End"); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t){ Serial.printf("OTA %u%%\r", p/(t/100)); });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("OTA Error[%u]\n", e); });
  ArduinoOTA.begin();
}

void setup(){
  Serial.begin(115200);
  wifi_connect();          // เชื่อม Wi-Fi (ใช้ค่าใน secrets.h)
  setup_ota();             // ✅ เปิด OTA
  rfid_begin();            // SPI/RC522
  oled_begin();            // I2C/OLED
  // keypad_begin();       // เปิดใช้เมื่อพร้อม
  oled_msg(String("ID: ")+DEVICE_ID, String("ROLE: ")+DEVICE_ROLE);
}

void loop(){
  ArduinoOTA.handle();     // ✅ ให้ OTA ทำงานตลอด

  // 1) เตรียมจำนวนเงิน
  long amount = (String(DEVICE_ROLE)=="TOPUP") ? 10000 : 2500;   // เริ่มด้วยฮาร์ดโค้ด

  // 2) รอแตะบัตร → อ่าน UID
  String uid = rfid_read_uid();
    if(uid != ""){
        Serial.println("Card UID: " + uid);
    }

  // 3) เรียก API ตามบทบาท
  bool ok=false; long bal=-1; String reason;
  if(String(DEVICE_ROLE)=="TOPUP"){
    ok = api_topup(uid, amount, bal);
  }else{
    ok = api_payment(uid, amount, bal, reason);
  }

  // 4) แสดงผล
  if(ok){ oled_msg("OK", "Bal: " + String(bal)); }
  else  { oled_msg("Declined", reason); }
  delay(800);
}
