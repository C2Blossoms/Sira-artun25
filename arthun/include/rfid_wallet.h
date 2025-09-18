#pragma once
#include <Arduino.h>
#include <MFRC522.h>

class RfidWallet {
public:
  void begin();                       // เริ่ม SPI+RC522
  bool pollCard(String& uidHex);      // true ถ้ามีบัตรใหม่ (คืน UID เป็น HEX)
  void halt();                        // จบ transaction

  // เงินหน่วย "สตางค์"
  int32_t get(const String& uid) const;
  void    set(const String& uid, int32_t satang);
  bool    pay(const String& uid, int32_t satang);   // หักสำเร็จ = true
  void    topup(const String& uid, int32_t satang);

private:
  static String uidToHex_(const MFRC522::Uid& u); // forward (จาก MFRC522)
};
