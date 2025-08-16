#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 21
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 22
#endif

void oled_begin();
void oled_msg(const String& l1,
              const String& l2 = "",
              const String& l3 = "");

namespace OLED {
  static Adafruit_SSD1306 disp(128, 64, &Wire, -1);

  inline void begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    disp.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    disp.clearDisplay(); disp.display();
  }

  inline void show(const String& l1, const String& l2 = "", const String& l3 = "") {
    disp.clearDisplay();
    disp.setTextSize(1);
    disp.setTextColor(SSD1306_WHITE);
    disp.setCursor(0, 0);
    disp.println(l1);
    if (l2.length()) disp.println(l2);
    if (l3.length()) disp.println(l3);
    disp.display();
  }
}
