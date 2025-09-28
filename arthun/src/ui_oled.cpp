#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "qrcode.h"      // https://github.com/ricmoo/QRCode
#include "Config.h"
#include "ui_oled.h"

namespace {
  Adafruit_SSD1306 display(Cfg::OLED_W, Cfg::OLED_H, &Wire, Cfg::OLED_RESET);

  // วาดข้อความสองบรรทัดง่าย ๆ
  void drawTwoLines(const char* l1, const char* l2) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0, 0);
    if (l1) display.println(l1);
    if (l2) display.println(l2);
    display.display();
  }
}

bool oled::begin() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, Cfg::OLED_ADDR)) {
    return false;
  }
  display.clearDisplay();
  display.display();
  return true;
}

void oled::showBoot(const char* line1, const char* line2) {
  drawTwoLines(line1, line2);
}

void oled::showWaiting() {
  drawTwoLines("Waiting for", "PromptPay payload...");
}

void oled::clear() {
  display.clearDisplay();
  display.display();
}

void oled::drawQR(const char* text) {
  if (!text || !*text) return;

  // เลือก version ให้พอดีกับจออัตโนมัติ
  // สำหรับ 128x64 ใช้ version 5–7 ได้ดี ขอล็อกที่ 5 เพื่อลงตัวกับ quiet zone
  const uint8_t version = 5;           // modules = 4*v + 17 = 37
  const uint8_t ecc     = ECC_LOW;

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ecc, text);

  // คำนวณสเกล (รวม quiet zone)
  const int border  = 2; // quiet zone เล็กลงให้พอดีจอ
  const int modules = qrcode.size + border * 2;

  // fit ทั้งกว้างและสูง
  int scaleW = Cfg::OLED_W / modules;
  int scaleH = Cfg::OLED_H / modules;
  int scale  = (scaleW < scaleH ? scaleW : scaleH);
  if (scale < 1) scale = 1;

  const int qrPx = modules * scale;
  const int x0   = (Cfg::OLED_W  - qrPx) / 2;
  const int y0   = (Cfg::OLED_H  - qrPx) / 2;

  display.clearDisplay();
  display.fillRect(0, 0, Cfg::OLED_W, Cfg::OLED_H, SSD1306_WHITE);

  for (int y = 0; y < qrcode.size; y++) {
    for (int x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        display.fillRect(x0 + (x + border) * scale,
                         y0 + (y + border) * scale,
                         scale, scale, SSD1306_BLACK);
      }
    }
  }
  display.display();
}

void oled::handleSerialQR() {
  static String line;
  static const size_t MAXL = 1024;

  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) break;

    if (c == '\n') {
      String s = line; s.trim();
      line = "";

      if (!s.length()) return;

      if (s == "PING") {
        Serial.println("PONG");
        return;
      }
      if (s.startsWith("PAYLOAD:")) {
        String payload = s.substring(8);
        Serial.print("[ESP32] recv payload len=");
        Serial.println(payload.length());
        oled::drawQR(payload.c_str());
        Serial.println("OK: QR drawn");
      } else {
        Serial.print("IGNORED: "); Serial.println(s);
      }
    } else if (c != '\r') {
      if (line.length() < MAXL) line += (char)c;
      else { line = ""; Serial.println("[ESP32] WARN: line overflow -> drop"); }
    }
  }
}
