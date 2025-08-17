#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "qrcode.h"      // from https://github.com/ricmoo/QRCode

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String lineBuf;

// วาด QR Code จาก promptpay_payload
void drawQR(const char* text) {
  // ใช้เวอร์ชัน 10 + ECC ต่ำเพื่อให้ payload PromptPay แบบ static ใส่ได้และพอดีกับสูง 64px
  const uint8_t version = 10; // modules = 4*V + 17 => 57
  const uint8_t ecc = ECC_LOW;

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ecc, text);

  // คำนวณสเกลให้พอดีจอ 64px พร้อม quiet zone เล็กน้อย
  const int border = 4;                       // ลด quiet zone ให้พอแสดงบนจอเล็ก (สเปคแนะนำ 4)
  const int modules = qrcode.size + border*2; // รวม quiet zone
  int scale = SCREEN_HEIGHT / modules;
  if (scale < 1) scale = 1;
  const int qrPx = modules * scale;
  const int x0 = (SCREEN_WIDTH  - qrPx) / 2;
  const int y0 = (SCREEN_HEIGHT - qrPx) / 2;

  // วาดพื้นหลังสีขาว แล้วจุด QR เป็นสีดำ เพื่อคอนทราสต์สูง
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

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

void setup() {
  Serial.begin(115200);
  Wire.begin();  // SDA=21, SCL=22 (ESP32 default)

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true) { delay(1000); }
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0);
  display.println("Waiting for");
  display.println("PromptPay payload...");
  display.display();

}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      if (lineBuf.startsWith("PAYLOAD:")) {
        String payload = lineBuf.substring(8);
        drawQR(payload.c_str());
      }
      lineBuf = "";
    } else {
      lineBuf += c;
    }
  }
}