#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "Sha";
const char58* password = "22222222";

String serverName = "http://192.168.0.130:8000/balance/1";

 display(128, 64, &Wire, -1);

void setup() {
  Serial.begin(115200);

  // WiFi connect
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("OLED Ready!");
  display.display();
  delay(2000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);

    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(payload);

      // Parse JSON
      StaticJsonDocument<200> doc;
      deserializeJson(doc, payload);

      // ดึงค่าจาก JSON ที่ส่งมาจาก API
      int card_id = doc["card_id"];
      float balance = doc["balance"];

      // แสดงผลบน OLED
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("card id: ");
      display.println(card_id);
      display.println("balance: ");
      display.println(balance);
      display.display();
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  }
  delay(5000); // refresh ทุก 5 วิ
}
