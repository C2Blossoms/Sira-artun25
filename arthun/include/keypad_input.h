#pragma once
#include <Arduino.h>

class KeypadInput {
public:
  enum Mode : uint8_t { HOME, TOPUP, CHECK, PAY, PAY_WAIT };

  KeypadInput() = default;

  void begin();
  bool poll();
  void clear();
  void backspace();
  void consumeSubmitted() { submitted_ = false; }

  double amount() const;

  // accessors
  Mode mode() const { return mode_; }
  void setMode(Mode m);

  
  const char* buffer() const { return buf_; }
  bool takeSubmitted();
  bool submitted() const { return submitted_; }
  char lastKey() const { return lastKey_; }

private:
  Mode   mode_ = HOME;

  static constexpr size_t MAXLEN = 16;
  char   buf_[MAXLEN + 1] = {0};
  size_t len_  = 0;

  char   lastKey_ = 0;
  bool   submitted_ = false;

  long pending_pay_cents_ = -1;
};
