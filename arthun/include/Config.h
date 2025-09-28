#pragma once

namespace Cfg {
  // OLED
  constexpr uint8_t  OLED_ADDR  = 0x3C;
  constexpr uint8_t OLED_W = 128;
  constexpr uint8_t OLED_H = 64;
  constexpr uint8_t OLED_RESET = -1;
  
  // I2C setting
  constexpr uint32_t I2C_HZ  = 400000;
  constexpr uint8_t I2C_SDA  = 21;
  constexpr uint8_t I2C_SCL  = 22;
  

  // RC522
  constexpr uint8_t RCC522_SS   = 27;
  constexpr uint8_t RCC522_SCK  = 14;
  constexpr uint8_t RCC522_RST  = 4;
  constexpr uint8_t RCC522_MOSI = 13;
  constexpr uint8_t RCC522_MISO = 34;

  // Keypad 4x4 wiring (edit to match your keypad)
  constexpr uint8_t KEYPAD_I2C_ADDR = 0x20;
  // Keypad pin in I2C
  static byte Keypad_ROWS[4] = { 0, 1, 2, 3 };
  static byte Keypad_COLS[4] = { 4, 5, 6, 7 };
  extern byte K_ROWS[4];
  extern byte K_COLS[4];

  // I2C LCD 20x4
  constexpr uint8_t LCD_I2C_ADDR = 0x27;     // เปลี่ยนเป็น 0x3F ถ้าไม่ติด
  constexpr uint8_t LCD_COLS     = 20;
  constexpr uint8_t LCD_ROWS     = 4;

  constexpr uint32_t SERIAL_BAUD = 115200;
  constexpr char NVS_NS[] = "wallet";
}
