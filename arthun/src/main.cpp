#include <Arduino.h>

// กำหนดขา GPIO ที่มี LED ภายใน ESP32 (มักจะเป็นขา 2)
#define LED_BUILTIN 2

void setup() {
  // เริ่มต้นการสื่อสาร Serial เพื่อดูข้อความใน Serial Monitor
  Serial.begin(115200);

  // กำหนดให้ขา LED_BUILTIN เป็นโหมด OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // เปิด LED
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED is ON");

  // หน่วงเวลา 1 วินาที (1000 มิลลิวินาที)
  delay(1000);

  // ปิด LED
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED is OFF");

  // หน่วงเวลา 1 วินาที
  delay(1000);
}