#pragma once
#include <Arduino.h>

namespace Cfg {
  // RC522
  constexpr uint8_t RCC522_SS   = 27;
  constexpr uint8_t RCC522_RST  = 4;
  constexpr uint8_t RCC522_SCK  = 14;
  constexpr uint8_t RCC522_MOSI = 13;
  constexpr uint8_t RCC522_MISO = 34;

  // Keypad 4x4 wiring (edit to match your keypad)
  static byte ROWS[4] = { 32, 33, 25, 26 };
  static byte COLS[4] = { 19, 18, 17, 16 };

  // I2C LCD 20x4
  constexpr uint8_t LCD_I2C_ADDR = 0x27;     // เปลี่ยนเป็น 0x3F ถ้าไม่ติด
  constexpr uint8_t I2C_SDA      = 21;
  constexpr uint8_t I2C_SCL      = 22;
  constexpr uint8_t LCD_COLS     = 20;
  constexpr uint8_t LCD_ROWS     = 4;

  constexpr uint32_t SERIAL_BAUD = 115200;
  constexpr char NVS_NS[] = "wallet";
}
