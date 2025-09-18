#include "keypad_input.h"
#include "Config.h"
#include "ui_lcd.h"
#include <Keypad.h>

// --- Keypad map 4x4 ---
static const byte ROWS = 4, COLS = 4;
static char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// ใช้พินจาก Config.h (Cfg::ROWS / Cfg::COLS)
static Keypad keypad = Keypad(
  makeKeymap(keys),
  Cfg::ROWS, Cfg::COLS,
  ROWS, COLS
);

void KeypadInput::begin() {
  lastKey_   = 0;
  submitted_ = false;
  len_ = 0; 
  buf_[0] = '\0';

  // วาดตามโหมดปัจจุบัน
  const char* modeTxt =
    (mode_==IDLE?"Mode: IDLE":mode_==TOPUP?"Mode: TOPUP":mode_==PAY?"Mode: PAY":"Mode: CHECK");
  ui::showHeader(modeTxt);

  if (mode_ == TOPUP || mode_ == PAY) {
    ui::showValueLabel(mode_==TOPUP ? "Topup value:" : "Pay value:");
    ui::showValue(buf_);
    ui::showFooterActions();
  } else {
    ui::showPressed('-');
    ui::showBuffer(buf_);
    ui::showFooterActions();
  }

  Serial.println("[KEYPAD] init ok.");
}

// !!! มีได้เพียง 'setMode' เดียวเท่านั้นในไฟล์นี้ !!!
void KeypadInput::setMode(Mode m) {
  mode_ = m;  // << เอา if (mode_ == m) return; ออก

  const char* modeTxt =
    (m==IDLE ? "Mode: IDLE" :
     m==TOPUP? "Mode: TOPUP" :
     m==PAY  ? "Mode: PAY"   : "Mode: CHECK");
  ui::showHeader(modeTxt);
  clear();  // ล้างอินพุตเมื่อเปลี่ยนโหมด

  if (m == TOPUP || m == PAY) {
    ui::showValueLabel(m==TOPUP ? "Topup value:" : "Pay value:");
    ui::showValue(buf_);
  } else {
    ui::showPressed('-');
    ui::showBuffer(buf_);
  }
  ui::showFooterActions();

  Serial.print("[MODE] "); Serial.println(modeTxt);
}

void KeypadInput::clear() {        // <- ต้องเป็น KeypadInput (K ใหญ่)
  len_ = 0; buf_[0] = '\0';
  submitted_ = false;

  if (mode_ == TOPUP || mode_ == PAY) {
    ui::showValue(buf_);
  } else {
    ui::showBuffer(buf_);
    ui::showPressed('-');
  }
  Serial.println("[KEYPAD] buffer cleared");
}

void KeypadInput::backspace() {
  if (len_ > 0) {
    len_--;
    buf_[len_] = '\0';
    if (mode_ == TOPUP || mode_ == PAY) ui::showValue(buf_);
    else ui::showBuffer(buf_);
    Serial.println("[KEYPAD] backspace");
  } else {
    Serial.println("[KEYPAD] backspace (empty)");
  }
}

bool KeypadInput::poll() {
  char k = keypad.getKey();
  if (!k) return false;

  lastKey_ = k;

  // ฟังก์ชันคีย์ (รองรับ row4 พัง: ใช้ A/B/C)
  if (k == '*' || k == 'B') {          // Clear
    clear();
    Serial.print("[KEYPAD] key='"); Serial.print(k); Serial.println("' (clear)");
  } else if (k == '#' || k == 'A') {   // OK / Submit
    submitted_ = true;
    ui::showSubmitted(buf_);
    // วาดหัวกลับมาเพื่อใช้ต่อ
    ui::showHeader(
      mode_==IDLE?"Mode: IDLE":mode_==TOPUP?"Mode: TOPUP":mode_==PAY?"Mode: PAY":"Mode: CHECK"
    );
    if (mode_ == TOPUP || mode_ == PAY) {
      ui::showValueLabel(mode_==TOPUP ? "Topup value:" : "Pay value:");
      ui::showValue(buf_);
    } else {
      ui::showPressed('-');
      ui::showBuffer(buf_);
    }
    ui::showFooterActions();
    Serial.print("[SUBMIT] value=\""); Serial.print(buf_); Serial.println("\"");
  } else if (k == 'C') {               // Backspace
    backspace();
  } else {
    // พิมพ์ตัวเลข/จุดทศนิยมเฉพาะใน TOPUP/PAY
    bool isDigit = (k >= '0' && k <= '9');
    bool isDot   = (k == '.');
    if (mode_ == TOPUP || mode_ == PAY) {
      if (isDigit || (isDot && strchr(buf_, '.') == nullptr)) {
        if (len_ < MAXLEN) {
          buf_[len_++] = k; buf_[len_] = '\0';
          ui::showValue(buf_);
          Serial.print("[VAL] "); Serial.println(buf_);
        } else {
          Serial.println("[KEYPAD] value full");
        }
      } else {
        // กดคีย์อื่นในหน้านี้ -> ไม่ทำอะไร
        Serial.print("[IGN] non-numeric key '"); Serial.print(k); Serial.println("' in value page");
      }
    } else {
      // หน้าทั่วไป (IDLE/CHECK) แสดงแบบเดิม
      if (len_ < MAXLEN) {
        buf_[len_++] = k; buf_[len_] = '\0';
        ui::showBuffer(buf_);
        ui::showPressed(k);
        Serial.print("[KEYPAD] key='"); Serial.print(k);
        Serial.print("', buffer=\""); Serial.print(buf_); Serial.println("\"");
      } else {
        Serial.println("[KEYPAD] buffer full (ignored key)");
      }
    }
  }

  return true;
}