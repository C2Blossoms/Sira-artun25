#include <Arduino.h>
#include "Config.h"
#include "rfid_wallet.h"
#include "keypad_input.h"
#include "ui_serial.h"

using namespace Cfg;

static RfidWallet wallet;
static KeypadInput input;
static String currentUID;

static const char* modeName(KeypadInput::Mode m){
  switch(m){
    case KeypadInput::TOPUP: return "TOP-UP (A)";
    case KeypadInput::PAY:   return "PAY (B)";
    case KeypadInput::CHECK: return "CHECK (C)";
    default: return "IDLE";
  }
}

void setup(){
  Serial.begin(SERIAL_BAUD); delay(600);
  UI::banner();
  input.begin();
  input.setMode(KeypadInput::IDLE);
  wallet.begin();
  UI::status(modeName(input.mode()), input.amountStr(), currentUID, 0);
}

void loop(){
  if(input.poll()){
    UI::status(modeName(input.mode()), input.amountStr(), currentUID,
               currentUID.isEmpty()?0:wallet.get(currentUID));
  }
  String uid;
  if(wallet.pollCard(uid)){
    currentUID = uid;
    Serial.print(F("Card tapped: ")); Serial.println(currentUID);
    Serial.print(F("Current balance: ")); UI::money(wallet.get(currentUID)); Serial.println();
    wallet.halt();
  }
  if(input.consumeConfirm()){
    if(currentUID.isEmpty()){
      Serial.println(F(">> No card. Tap a card first."));
    }else{
      int32_t bal = wallet.get(currentUID);
      int32_t amt = (int32_t)(input.amountTHB()*100L);
      if(input.mode()==KeypadInput::TOPUP){
        wallet.topup(currentUID, amt);
        Serial.print(F(">> TOP-UP +")); UI::money(amt);
        Serial.print(F(" -> New: ")); UI::money(wallet.get(currentUID)); Serial.println();
      }else if(input.mode()==KeypadInput::PAY){
        if(!wallet.pay(currentUID, amt)){
          Serial.print(F(">> PAY -")); UI::money(amt);
          Serial.print(F(" -> DENIED (insufficient). Balance: ")); UI::money(bal); Serial.println();
        }else{
          Serial.print(F(">> PAY -")); UI::money(amt);
          Serial.print(F(" -> New: ")); UI::money(wallet.get(currentUID)); Serial.println();
        }
      }else if(input.mode()==KeypadInput::CHECK){
        Serial.print(F(">> CHECK: ")); UI::money(bal); Serial.println();
      }
      input.setMode(input.mode());
      UI::status(modeName(input.mode()), input.amountStr(), currentUID, wallet.get(currentUID));
    }
  }
}
