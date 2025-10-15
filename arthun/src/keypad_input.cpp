#include "keypad_input.h"
#include "Config.h"
#include "ui_lcd.h"
#include <Keypad.h>
#include <Keypad_I2C/Keypad_I2C.h>
#include <cstring>
#include <cstdlib>

extern String currentUID;

// --- Keypad map 4x4 ---
static const byte K_ROWS = 4, K_COLS = 4;
static char keys[K_ROWS][K_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// ใช้พินจาก Config.h (Cfg::Keypad_ROWS/COLS/KEYPAD_I2C_ADDR)
static Keypad_I2C keypad(
  makeKeymap(keys),
  Cfg::Keypad_ROWS, Cfg::Keypad_COLS,
  K_ROWS, K_COLS,
  Cfg::KEYPAD_I2C_ADDR, PCF8574
);

// --- helpers แสดงผล ---
namespace {
  inline const char* modeText(KeypadInput::Mode m) {
    switch (m) {
      case KeypadInput::HOME:  return "Mode: MENU";
      case KeypadInput::TOPUP: return "Mode: TOPUP";
      case KeypadInput::CHECK: return "Mode: CHECK BALANCE";
    }
    return "Mode: ?";
  }

  inline void drawPage_(KeypadInput::Mode m, const char* buf) {
    ui::showHeader(modeText(m));
    if (m == KeypadInput::TOPUP) {
      if (currentUID.isEmpty()) {
        // ยังไม่แตะบัตร -> แสดงหน้ารอแตะบัตร
        ui::showHeader("TOPUP: tap card");
        ui::clearLine(1);
        ui::clearLine(2);
        ui::footer_value();            // ปุ่มช่วย: C=CLR ... (แก้ให้ตรง mapping)
      } else {
        // แตะบัตรแล้ว -> แสดงหน้าป้อนจำนวน
        ui::showHeader("Enter amount");
        ui::showValueLabel("Topup value:");
        ui::showValue(buf);
        ui::footer_value();
      }
      return;                          // กันไม่ให้หน้าถูกวาดทับ
    }

    if (m == KeypadInput::CHECK) {
      ui::showCardUID(currentUID);
      ui::showBalance(NAN);
      ui::footer_menu();
      return;
    }
    
    // HOME: ล้างบรรทัด 1 และ 2 ให้โล่ง
    ui::clearLine(1);
    ui::clearLine(2);
    ui::footer_menu();
  }
}

// --- begin() ---
void KeypadInput::begin() {
  keypad.begin();
  lastKey_ = 0;
  submitted_ = false;
  len_ = 0; buf_[0] = '\0';

  drawPage_(mode_, buf_);
  ui::footer_menu();
}

// --- setMode() ---
void KeypadInput::setMode(Mode m) {
  mode_ = m;
  clear();
  // ui::footer_menu();
  if (m == TOPUP) ui::footer_value();  // C=CLR #=DEL *=OK D=MN
  else ui::footer_menu();
  Serial.print(F("[MODE] ")); Serial.println(modeText(mode_));
}

// --- amount() ---
double KeypadInput::amount() const {
  if (len_ == 0) return NAN;
  if (len_ == 1 && buf_[0] == '.') return NAN;
  char* endp = nullptr;
  double v = strtod(buf_, &endp);
  if (endp == buf_) return NAN;
  return v;
}

// --- showCheckUID() ---
void KeypadInput::showCheckUID(const String& uid) {
  // ให้แน่ใจว่าอยู่หน้า CHECK และเฮดเดอร์ขึ้นถูก
  if (mode_ != CHECK) setMode(CHECK);
  ui::showHeader("Mode: CHECK BALANCE");
  ui::showCardUID(uid.c_str());   // แถว 2 แสดง UID
  // ui::showValue("");             // เคลียร์ตำแหน่งค่าที่แถว 1 (ถ้ามีค่าเก่า)
}

// --- showCheckBalance() ---
void KeypadInput::showCheckBalance(float baht) {
  // แสดงยอดคงเหลือที่แถว 3 (ฟังก์ชันนี้จะพิมพ์ "Balance: ..." ให้เอง)
  ui::showBalance(baht);
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

  // สลับโหมด
  if (k == 'A') { setMode(TOPUP); return true; }   // เข้า TOPUP
  if (k == 'B') { setMode(CHECK); return true; }   // เข้า CHECK
  if (k == 'D') { setMode(HOME);  return true; }   // กลับ HOME

  // คีย์ช่วยเหลือ
  if (k == 'C') { clear(); return true; }          // ล้าง buffer
  if (k == '*') { backspace(); return true; }      // ลบ 1 หลัก

  // ยืนยันค่า
  if (k == '#') {
    submitted_ = true;
    return true;
  }

  // รับเฉพาะ 0-9 และจุดทศนิยม (เฉพาะ TOPUP)
  if (mode_ == TOPUP) {
    if ((k >= '0' && k <= '9') || k == '.') {
      if (len_ < MAXLEN) {
        buf_[len_++] = k;
        buf_[len_]   = '\0';
        drawPage_(mode_, buf_);
      }
      return true;
    }
  }

  // โหมดอื่น ๆ (เช่น CHECK) ไม่ต้องรับตัวเลข
  return false;
}