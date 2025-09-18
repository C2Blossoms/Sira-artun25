// ui_serial.cpp
#include "ui_serial.h"
void UI::banner(){
  Serial.println(F("\n== RFID Wallet + Keypad =="));
  Serial.println(F("Keys: A=TopUp, B=Pay, C=Check, D=Clear, *=Backspace, #=Confirm"));
  Serial.println(F("Enter THB (integer), then #"));
}
void UI::money(int32_t sat){
  long thb = sat/100, st = llabs(sat%100);
  Serial.printf("%ld.%02ld THB", thb, st);
}
void UI::status(const char* mode, const String& amt, const String& uid, int32_t bal){
  Serial.println(F("\n===================="));
  Serial.print(F("MODE   : ")); Serial.println(mode);
  Serial.print(F("Amount : ")); Serial.println(amt.length()? amt : "-");
  Serial.print(F("Card   : ")); Serial.println(uid.length()? uid : F("(no card)"));
  Serial.print(F("Balance: ")); money(bal); Serial.println();
  Serial.println(F("===================="));
}
