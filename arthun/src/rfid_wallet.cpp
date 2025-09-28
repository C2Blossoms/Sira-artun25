#include "rfid_wallet.h"
#include "Config.h"
#include <limits.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>

using namespace Cfg;
static MFRC522     rfid(RCC522_SS, RCC522_RST);
static Preferences prefs;

void RFIDWallet::begin() {
  pinMode(RCC522_SS, OUTPUT);          // <— เพิ่ม
  digitalWrite(RCC522_SS, HIGH);       // <— เพิ่ม (กันชิปจับบัสตอนบูต)

  SPI.begin(RCC522_SCK, RCC522_MISO, RCC522_MOSI, RCC522_SS);
  rfid.PCD_Init();
}

bool RFIDWallet::pollCard(String& uidHex) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;
  uidHex = uidToHex_(rfid.uid);
  return true;
}
void RFIDWallet::halt(){ rfid.PICC_HaltA(); rfid.PCD_StopCrypto1(); }

String RFIDWallet::uidToHex_(const MFRC522::Uid& u){
  String s; s.reserve(u.size*2);
  for (byte i=0;i<u.size;i++){ 
    if (u.uidByte[i]<0x10) s+='0';
      s+=String(u.uidByte[i],HEX); 
  }
  s.toUpperCase(); return s;
}

// ---- เงินใน NVS ----
int32_t RFIDWallet::get(const String& uid) const {
  prefs.begin(NVS_NS, true);
  int32_t v = prefs.getInt(uid.c_str(), 0);
  prefs.end();
  return v;
}
void RFIDWallet::set(const String& uid, int32_t sat) {
  prefs.begin(NVS_NS, false);
  prefs.putInt(uid.c_str(), sat);
  prefs.end();
}
bool RFIDWallet::pay(const String& uid, int32_t sat){
  int32_t cur = get(uid);
  if (cur < sat) return false;
  set(uid, cur - sat); return true;
}
void RFIDWallet::topup(const String& uid, int32_t sat){
  int64_t nb = (int64_t)get(uid) + sat; if (nb > INT32_MAX) nb = INT32_MAX;
  set(uid, (int32_t)nb);
}
