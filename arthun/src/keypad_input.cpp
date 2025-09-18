// keypad_input.cpp
#include "keypad_input.h"
#include "Config.h"
#include <Keypad.h>

using namespace Cfg;

static byte rows[4] = { ROWS[0], ROWS[1], ROWS[2], ROWS[3] };
static byte cols[4] = { COLS[0], COLS[1], COLS[2], COLS[3] };

static char keys[4][4] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
static Keypad kp = Keypad(makeKeymap(keys), rows, cols, 4, 4);

void KeypadInput::begin(){ /* no-op */ }
char KeypadInput::getKey_(){ return kp.getKey(); }
bool KeypadInput::poll(){ char k=getKey_(); if(!k) return false; handleKey_(k); return true; }
long KeypadInput::amountTHB() const { return buf_.length()? buf_.toInt():0; }
bool KeypadInput::consumeConfirm(){ bool w=confirmed_; confirmed_=false; return w; }
void KeypadInput::handleKey_(char k){
  if (k >= '0' && k <= '9') {
    if (buf_ == "0") buf_ = "";          // ตัด 0 นำหน้า
    if (buf_.length() < 7) buf_ += k;    // จำกัดความยาว
    return;
  }
  switch(k){
    case 'A': setMode(TOPUP); break;
    case 'B': setMode(PAY);   break;
    case 'C': setMode(CHECK); break;
    case 'D': buf_=""; break;
    case '*': if(buf_.length()) buf_.remove(buf_.length()-1); break;
    case '#': confirmed_=true; break;
  }
}
