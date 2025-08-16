#include "oled.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 21
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 22
#endif

// จอ OLED 128x64 I2C @ 0x3C
static Adafruit_SSD1306 d(128, 64, &Wire, -1);

void oled_begin() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  d.begin(SSD1306_SWITCHCAPVCC, 0x3C);   // ถ้าบอร์ดคุณเป็น 0x3D ให้แก้ตรงนี้
  d.clearDisplay();
  d.display();
}

void oled_msg(const String& l1, const String& l2, const String& l3) {
  d.clearDisplay();
  d.setTextSize(1);
  d.setTextColor(SSD1306_WHITE);
  d.setCursor(0, 0);
  d.println(l1);
  if (l2.length()) d.println(l2);
  if (l3.length()) d.println(l3);
  d.display();
}
