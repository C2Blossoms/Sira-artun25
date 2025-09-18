#pragma once
#include <Arduino.h>

namespace Cfg {
  // RC522
  constexpr uint8_t RCC522_SS   = 27;
  constexpr uint8_t RCC522_RST  = 23;
  constexpr uint8_t RCC522_SCK  = 14;
  constexpr uint8_t RCC522_MOSI = 13;
  constexpr uint8_t RCC522_MISO = 34;

  // Keypad 4x4 wiring (edit to match your keypad)
  static byte COLS[4] = { 26, 25, 33, 32 };
  static byte ROWS[4] = { 16, 17, 18, 19 };

  constexpr uint32_t SERIAL_BAUD = 115200;
  constexpr char NVS_NS[] = "wallet";
}
