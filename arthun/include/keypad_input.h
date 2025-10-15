// keypad_input.h
#pragma once
#include <Arduino.h>

class KeypadInput {
public:
  enum Mode : uint8_t { HOME, TOPUP, CHECK };   // <<— เหลือ 2 โหมด

  KeypadInput() = default;

  void begin();
  bool poll();
  void clear();
  void backspace();

  double amount() const;
  double asDouble() const { return atof(buf_); }

  Mode mode() const { return mode_; }
  void setMode(Mode m);

  const char* buffer() const { return buf_; }

  bool takeSubmitted();
  bool submitted() const { return submitted_; }
  void consumeSubmitted() { submitted_ = false; }

  char lastKey() const { return lastKey_; }
  char takeLastKey() { char k = lastKey_; lastKey_ = 0; return k; }

  // Check Balance
  void showCheckUID(const String& uid);
  void showCheckBalance(float baht);

private:
  Mode   mode_ = HOME;

  static constexpr size_t MAXLEN = 16;
  char   buf_[MAXLEN + 1] = {0};
  size_t len_  = 0;

  char   lastKey_ = 0;
  bool   submitted_ = false;
};
