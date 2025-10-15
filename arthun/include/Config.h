#pragma once
namespace Cfg {
  // WiFi
  constexpr const char* WIFI_SSID = "baan pa ya";  // เปลี่ยนเป็น SSID ของคุณ
  constexpr const char* WIFI_PASS = "029523007";    // เปลี่ยนเป็น

  // API base (เปลี่ยนเป็น IP เครื่องคุณ)
  constexpr const char* API_BASE  = "http://192.168.11.116:54264";

  inline void initBuzzer() {
    ledcSetup(0, 2000, 8);
    ledcAttachPin(25, 0);
    ledcWrite(0, 0);
  }

  // Buzzer Tones
  static void beepOK()   { ledcWriteTone(0, 2000); delay(80); ledcWrite(0, 0); }   // สั้นๆ = สำเร็จ
  static void beepErr()  { ledcWriteTone(0, 400); delay(220); ledcWrite(0, 0); }  // ยาว = ผิดพลาด
  static void beepWait() { ledcWriteTone(0, 1200); delay(60); ledcWrite(0, 0); }   // ติ๊ด = กำลังทำงาน

  // I2C setting
  constexpr uint32_t I2C_HZ  = 100000;
  constexpr uint8_t I2C_SDA  = 21;
  constexpr uint8_t I2C_SCL  = 22;

  // OLED 128*64
  constexpr uint8_t  OLED_ADDR  = 0x3C;
  constexpr uint8_t OLED_W = 128;
  constexpr uint8_t OLED_H = 64;  

  // RC522
  constexpr uint8_t RCC522_SS   = 27;
  constexpr uint8_t RCC522_SCK  = 18;
  constexpr uint8_t RCC522_RST  = 4;
  constexpr uint8_t RCC522_MOSI = 23;
  constexpr uint8_t RCC522_MISO = 19;

  // Keypad 4x4
  constexpr uint8_t KEYPAD_I2C_ADDR = 0x20;
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
