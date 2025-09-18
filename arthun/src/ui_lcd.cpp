#include "ui_lcd.h"
#include "Config.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(Cfg::LCD_I2C_ADDR, Cfg::LCD_COLS, Cfg::LCD_ROWS);

namespace ui {
  void begin() {
    Wire.begin(Cfg::I2C_SDA, Cfg::I2C_SCL);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Sira-arthun Ready");
    lcd.setCursor(0,3); lcd.print("*=CLR  #=OK");
  }

  void showHeader(const char* modeText) {
    lcd.setCursor(0,0);
    // เคลียร์บรรทัด
    lcd.print("                    ");
    lcd.setCursor(0,0);
    lcd.print(modeText);
  }

  // GENERAL MODE (IDLE/DEBUG)
  void showPressed(char k) {
    lcd.setCursor(0,1);
    lcd.print("Pressed: ");
    lcd.print(k);
    // ลบหางเดิม
    lcd.print("            ");
  }

  void showBuffer(const char* buf) {
    lcd.setCursor(0,2);
    lcd.print("Buffer: ");
    // เคลียร์บรรทัดก่อน
    lcd.print("               ");
    lcd.setCursor(8,2);
    lcd.print(buf);
  }

  // TOPUP/PAY
  void showValueLabel(const char* label) {
    // line 1: Label
    lcd.setCursor(0,1);
    lcd.print("               ");
    lcd.setCursor(0,1);
    lcd.print(label);
  }

  void showValue(const char* value) {
    // line 2: Value
    lcd.setCursor(0,2);
    lcd.print("               ");
    lcd.setCursor(0,2);
    lcd.print(value);
  }

  void showFooterActions() {
    lcd.setCursor(0,3);
    lcd.print("B=CLR  A=OK  C=BS   ");
  }

  void showSubmitted(const char* buf) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Submitted:");
    lcd.setCursor(0,1); lcd.print(buf);
    lcd.setCursor(0,3); lcd.print(buf);
    showFooterActions();
  }
}
