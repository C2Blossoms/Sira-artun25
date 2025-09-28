#include "keypad_input.h"
#include "Config.h"
#include "ui_lcd.h"
#include <Keypad.h>
#include <Keypad_I2C/Keypad_I2C.h>
#include <cstring>   // strchr
#include <cstdlib>   // strtod

// --- Keypad map 4x4 ---
static const byte K_ROWS = 4, K_COLS = 4;
static char keys[K_ROWS][K_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// ใช้พินจาก Config.h (Cfg::ROWS / Cfg::COLS)
static Keypad_I2C keypad(makeKeymap(keys), Cfg::Keypad_ROWS, Cfg::Keypad_COLS, K_ROWS, K_COLS, Cfg::KEYPAD_I2C_ADDR, PCF8574);

// --- เพิ่ม helper เพื่อลดโค้ดซ้ำ ---
namespace {
  inline const char* modeText(KeypadInput::Mode m) {
    switch (m) {
      case KeypadInput::HOME:  return "Mode: MENU";
      case KeypadInput::TOPUP: return "Mode: TOPUP";
      case KeypadInput::CHECK: return "Mode: CHECK";
    }
    return "Mode: ?"; // กันพลาด
  }

  inline void drawPage_(KeypadInput::Mode m, const char* buf) {
    ui::showHeader(modeText(m));
    if ( m == KeypadInput::TOPUP ) {
      ui::showValueLabel(m == KeypadInput::TOPUP ? "Topup value:" : "Pay value:");
      ui::showValue(buf);
    } else {
      ui::showPressed('-');
      ui::showBuffer(buf);
    }
  }
}

// --- begin() ---
void KeypadInput::begin() {

  keypad.begin();

  lastKey_   = 0;
  submitted_ = false;
  len_ = 0;
  buf_[0] = '\0';
  drawPage_(mode_, buf_);
  if ( mode_ == TOPUP ){
    ui::footer_value();
  } else if (mode_ == PAY_WAIT) {
    ui::footer_paywait();
  } else {
    ui::footer_menu();
  }
  Serial.println(F("[KEYPAD] init ok."));
}

// --- setMode() ---
void KeypadInput::setMode(Mode m) {
  mode_ = m;
  clear(); // จะรีเฟรชหน้าจอด้วยใน clear()
  if ( m == TOPUP ) {
    ui::footer_value();
  } else if (m == PAY_WAIT) {
    ui::footer_paywait();
  } else if (m == CHECK) {
    ui::footer_menu();
  } else {
    ui::footer_menu();
  }
  Serial.print(F("[MODE] ")); Serial.println(modeText(mode_));
}

double KeypadInput::amount() const {
  if (len_ == 0) return NAN;
  if (len_ == 1 && buf_[0] == '.') return NAN;
  char* endp = nullptr;
  double v = strtod(buf_, &endp);
  if (endp == buf_) return NAN;   // parse ไม่ได้
  return v;
}

// --- clear() ---
void KeypadInput::clear() {
  len_ = 0; buf_[0] = '\0';
  submitted_ = false;
  drawPage_(mode_, buf_);
  Serial.println(F("[KEYPAD] buffer cleared"));
}

// --- backspace() ---
void KeypadInput::backspace() {
  if (len_ > 0) {
    buf_[--len_] = '\0';
    drawPage_(mode_, buf_);
    Serial.println(F("[KEYPAD] backspace"));
  } else {
    Serial.println(F("[KEYPAD] backspace (empty)"));
  }
}

bool KeypadInput::takeSubmitted() {
  bool was = submitted_;
  submitted_ = false;
  return was;
}

// --- poll() ---
bool KeypadInput::poll() {
  char k = keypad.getKey();
  if (!k) return false;
  lastKey_ = k;

  // โหมดสลับ
  if (k == 'A') { setMode(TOPUP); return true; }
  if (k == 'B') { setMode(CHECK);   return true; }
  // if (k == 'D') { setMode(MENU);  return true; }

  // คีย์ช่วยเหลือ
  if (k == 'C') { clear(); return true; }      // CLEAR
  if (k == '#') { backspace(); return true; }  // BACKSPACE

  // ยืนยัน
  if (k == '*') {
    if (len_ == 0) {
      Serial.println(F("[SUBMIT] ignored (empty)"));
    } else {
      submitted_ = true;
      ui::showSubmitted(buf_);
      ui::footer_paywait();
      Serial.print(F("[SUBMIT] value=\"")); Serial.print(buf_); Serial.println(F("\""));

      // นโยบายหลัง submit: ล็อกจนกด C เคลียร์ (ตัวอย่าง)
      // ถ้าอยากเคลียร์ทันที ให้เรียก clear(); แล้ว return
      drawPage_(mode_, buf_);
    }
    return true;

  // รับค่า
  if (mode_ == TOPUP ) {
    // *** ตัดทศนิยมออก เพราะ layout ไม่มี '.' ***
    if (k >= '0' && k <= '9') {
      if (len_ < MAXLEN) {
        buf_[len_++] = k; buf_[len_] = '\0';
        ui::showValue(buf_);
        Serial.print(F("[VAL] ")); Serial.println(buf_);
      } else {
        Serial.println(F("[KEYPAD] value full"));
      }
    } else {
      Serial.print(F("[IGN] non-numeric key '")); Serial.print(k); Serial.println(F("'"));
    }
  } else {
    // IDLE/CHECK: เก็บเป็นข้อความทั่วไป
    if (len_ < MAXLEN) {
      buf_[len_++] = k; buf_[len_] = '\0';
      ui::showBuffer(buf_);
      ui::showPressed(k);
      Serial.print(F("[KEYPAD] key='")); Serial.print(k);
      Serial.print(F("', buffer=\"")); Serial.print(buf_); Serial.println(F("\""));
    } else {
      Serial.println(F("[KEYPAD] buffer full (ignored key)"));
    }
  }
  return true;
  }
}