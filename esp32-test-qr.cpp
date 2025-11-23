      #include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SS_PIN 5
#define RST_PIN 0
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

const char* ssid     = "Tonliw";
const char* password = "77777772";

// ใช้ IP จริงของเซิร์ฟเวอร์
String matrixApi = "http://172.20.10.6:8000/create-qr-matrix/";
String payPageApi = "http://172.20.10.6:8000/pay-page/";
float amount = 50.0;  // หน่วยบาท

// ---------- ฟังก์ชันวาด QR บนจอ ----------
void drawQR(JSONVar matrix) {
  if (matrix.length() == 0) return;

  display.clearDisplay();

  int N = matrix.length();   // ขนาด matrix
  int border = 2;            // quiet zone
  int modules = N + border * 2;

  int scaleW = 128 / modules;
  int scaleH = 64 / modules;
  int scale  = (scaleW < scaleH ? scaleW : scaleH);
  if (scale < 1) scale = 1;

  int qrPx = modules * scale;
  int x0   = (128 - qrPx) / 2;  // center horizontally
  int y0   = (64  - qrPx) / 2;  // center vertically

  // ---- ปรับตำแหน่งสำหรับจอ 1.3" ----
  int adjustX = 0;   // ค่าบวก = เลื่อนขวา
  int adjustY = 0;   // ค่าบวก = เลื่อนลง
  x0 += adjustX;
  y0 += adjustY;

  // ---- วาด QR ----
  for (int y = 0; y < N; y++) {
    String row = (const char*) matrix[y];
    for (int x = 0; x < row.length(); x++) {
      if (row[x] == '1') {
        display.fillRect(
          x0 + (x + border) * scale,
          y0 + (y + border) * scale,
          scale, scale,
          SSD1306_WHITE
        );
      }
    }
  }

  display.display();
}

// void drawQR(JSONVar matrix) {
//   if (matrix.length() == 0) return;

//   display.clearDisplay();

//   int N = matrix.length();
//   int border = 2;                 // quiet zone
//   int modules = N + border * 2;

//   // คำนวณ scale พอดีจอ (ใหญ่สุดที่ไม่ล้น)
//   int scaleW = 128 / modules;
//   int scaleH = 64 / modules;
//   int scale  = (scaleW < scaleH ? scaleW : scaleH);
//   if (scale < 1) scale = 1;

//   // ขนาดจริงของ QR
//   int qrPx = modules * scale;

//   // วาด QR
//   for (int y = 0; y < N; y++) {
//     String row = (const char*) matrix[y];
//     for (int x = 0; x < row.length(); x++) {
//       if (row[x] == '1') {
//         display.fillRect(
//           x0 + (x + border) * scale,
//           y0 + (y + border) * scale,
//           scale, scale,
//           SSD1306_WHITE
//         );
//       }
//     }
//   }

//   display.display();
// }







void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init(); // เริ่มต้น RFID reader

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void loop() {
  // ตรวจสอบการ์ดใหม่
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // รวบรวม UID เป็นสตริงฐานสิบหก
  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();

  //แสดง UID และจำนวนเงินบน OLED ชั่วคราว
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("UID: " + uidStr);
  display.println("Amt: " + String(amount, 2));
  display.display();

  // เรียก API /create-qr-matrix/{card_id}/{amount}
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    int amountAsInt = (int)(amount);  // ถ้า backend ใช้บาทตรง ๆ
    String urlMatrix = matrixApi + uidStr + "/" + String(amountAsInt);
    http.begin(urlMatrix);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      JSONVar result = JSON.parse(payload);
      if (JSON.typeof(result) != "undefined") {
        JSONVar matrix = result["matrix"];
        String payUrl = String((const char*)result["pay_url"]);

      // แสดง UID และ Amount ข้างบน
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("UID: " + uidStr);
      display.println("Amt: " + String(amount, 2));
      display.display();
      delay(1000); // โชว์ข้อความก่อน 1 วิ

      // >>> แสดง QR แทนข้อความ <<<
      drawQR(matrix);

      }
    }
    http.end();
  }

  delay(3000);
}
