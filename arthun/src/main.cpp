#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "qrcode.h"      // https://github.com/ricmoo/QRCode

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- QR ----------
static void drawQR(const char* text) {
  // สำหรับ PromptPay static ด้วยเบอร์มือถือ version=10 + ECC_LOW มักพอ
  const uint8_t version = 4; // modules = 4*V + 17 => 57
  const uint8_t ecc = ECC_LOW;

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(version)];
  qrcode_initText(&qrcode, qrcodeData, version, ecc, text);

  const int border = 4;                            // quiet-zone ย่อเล็กลงให้พอดีจอ
  const int modules = qrcode.size + border * 2;    // รวม quiet-zone
  // int scale = SCREEN_HEIGHT / modules;
  int scale = 1;
  if (scale < 1) scale = 1;

  const int qrPx = modules * scale;
  const int x0 = (SCREEN_WIDTH  - qrPx) / 2;
  const int y0 = (SCREEN_HEIGHT - qrPx + 15) / 2;

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

// ---------- Serial line reader (ทนกับข้อมูลยาว/มาเป็นชิ้น ๆ) ----------
static String rxLine;
static const size_t RX_MAX = 1024;  // กันบัฟเฟอร์ล้น

static void processLine(const String& line) {
  String s = line; s.trim();
  if (!s.length()) return;

  Serial.print("[ESP32] RX len="); Serial.println(s.length());

  if (s == "PING") {
    Serial.println("PONG");
    return;
  }

  if (s.startsWith("PAYLOAD:")) {
    String payload = s.substring(8);
    Serial.print("[ESP32] Payload prefix ok, size=");
    Serial.println(payload.length());
    drawQR(payload.c_str());
    Serial.println("OK: QR drawn");
  } else {
    Serial.print("IGNORED: ");
    Serial.println(s);
  }
}

void setup() {
  // เพิ่มขนาด RX buffer (ESP32 Arduino รองรับ)
  Serial.setRxBufferSize(2048);
  Serial.begin(115200);
  Serial.setTimeout(2000);   // เผื่อช้าหน่อยสำหรับ payload ยาว

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    while (true) { delay(1000); }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0, 0);
  display.println("Waiting for");
  display.println("Promptpay payload...");
  display.display();

  // แจ้งสถานะพร้อมให้ฝั่ง Python รอ string นี้ได้
  Serial.println("[ESP32] Ready: Waiting for Promptpay payload");
}

void loop() {
  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) break;

    if (c == '\n') {
      processLine(rxLine);
      rxLine = "";
    } else if (c != '\r') {
      if (rxLine.length() < RX_MAX) {
        rxLine += (char)c;
      } else {
        // ล้นแล้วทิ้งบรรทัดนี้ไป
        rxLine = "";
        Serial.println("[ESP32] WARN: line overflow -> drop");
      }
    }
  }
}
