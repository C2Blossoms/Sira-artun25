#pragma once
#include <Arduino.h>

namespace oled {
  // ต้องเรียกก่อนใช้งาน ฟังก์ชันนี้จะ init Wire + SSD1306
  bool begin();

  // ข้อความสั้น ๆ ตอนบูต/รอข้อมูล
  void showBoot(const char* line1, const char* line2 = nullptr);
  void showWaiting();   // "Waiting for PromptPay payload..."

  // ลบจอ
  void clear();

  // วาด QR จาก payload (ใช้เวอร์ชันที่เหมาะกับ OLED อัตโนมัติ)
  void drawQR(const char* text);

  // อ่านจาก Serial เป็นบรรทัด: "PAYLOAD:<...>", "PING" → สำหรับทดสอบ
  void handleSerialQR();
}
