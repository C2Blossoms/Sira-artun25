// keypad_input.h
#pragma once
#include <Arduino.h>
class KeypadInput{
public:
  enum Mode { IDLE, TOPUP, PAY, CHECK };
  void begin(); bool poll();
  Mode mode() const { return mode_; }
  long amountTHB() const;
  const String& amountStr() const { return buf_; }
  void setMode(Mode m){ mode_=m; buf_=""; }
  bool consumeConfirm();
private:
  Mode mode_=IDLE; String buf_; bool confirmed_=false;
  char getKey_(); void handleKey_(char k);
};
