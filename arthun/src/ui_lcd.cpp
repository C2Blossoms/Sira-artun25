#include "ui_lcd.h"
#include "Config.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(Cfg::LCD_I2C_ADDR, Cfg::LCD_COLS, Cfg::LCD_ROWS);

namespace ui {
  static uint8_t HOME_ICON[8] = {
    B00000, B00100, B01110, B11111, B01010, B01010, B00000, B00000
  };
  static const uint8_t HOME_SLOT = 0;

  static void loadHomeIconOnce() { lcd.createChar(HOME_SLOT, HOME_ICON); }
  static void putHomeIcon(uint8_t col, uint8_t row) {
    lcd.setCursor(col, row);
    lcd.write(HOME_SLOT);
  }
}
namespace ui {

  void begin() {
    Wire.begin(Cfg::I2C_SDA, Cfg::I2C_SCL);
    lcd.init();
    lcd.backlight();
    lcd.noAutoscroll();
    lcd.leftToRight();
    lcd.clear();

    loadHomeIconOnce();

    footer_menu();
  }

  void clearLine(uint8_t row) {
    lcd.setCursor(0, row);
    for (uint8_t i = 0; i < Cfg::LCD_ROWS; i++) {
      lcd.write(' ');
    }
    lcd.setCursor(0, row);
  }
  void clearSelect(uint8_t col1, uint8_t col2, uint8_t row) {
    lcd.setCursor(col1, row);
    for (uint8_t i = col1; i < col2; ++i) {
      lcd.write(' ');
    }
      lcd.setCursor(col1, row);
  }

  void showHeader(const char* modeText) {
    clearLine(0);
    lcd.setCursor(0,0);
    lcd.print(modeText);
  }

  void showPressed(char k) {
    clearLine(1);
    lcd.setCursor(0,1);
    lcd.print(F("Pressed: "));
    lcd.print(k);
  }

  void showBuffer(const char* buf) {
    clearLine(2);
    lcd.setCursor(0,2);
    lcd.print(F("Buffer: "));
    lcd.print(buf);
  }

  void showValueLabel(const char* label) {
    clearLine(1);
    lcd.setCursor(0,1);
    lcd.print(label);
  }

  void showValue(const char* value) {
    clearSelect(13, 19, 1);
    lcd.setCursor(13,1);
    lcd.print(value);
  } 

  void footer_menu() {
    clearLine(3); lcd.setCursor(0,3); lcd.print(F("A=TOPUP B=CHK D="));
    putHomeIcon(Cfg::LCD_COLS - 4, 3);
  }

  void footer_value() {
    clearLine(3); lcd.setCursor(0,3); lcd.print(F("C=CLR #=DEL *=OK D=MN"));
  }

  void footer_paywait() {
    clearLine(3); lcd.setCursor(0,3); lcd.print(F("Scan QR or C=Cancel"));
  }

  void showSubmitted(const char* buf) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(F("Submitted:"));
    lcd.setCursor(0,1); lcd.print(buf);
  }

} // namespace ui
