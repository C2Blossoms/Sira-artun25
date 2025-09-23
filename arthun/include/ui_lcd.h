#pragma once
#include <Arduino.h>

namespace ui {

    // BEGIN
    void begin();

    // Clear
    void clearLine(uint8_t row);
    void clearSelect(uint8_t col1, uint8_t col2, uint8_t row);

    // HEADER
    void showHeader(const char* modeTxt);

    // WORKING
    void showPressed(char k);

    // STATUS
    void showSubmitted(const char* buf);

    // BUFFER
    void showBuffer(const char* buf);

    // TOPUP
    void showValueLabel(const char* label);
    void showValue(const char* value);

    // PAY_WAIT
    void showWaitingRFID(long cents);

    // FOOTER
    void footer_menu();   // A=TOPUP B=CHECK D=IDLE
    void footer_value();   // C=CLEAR #=BKSP *=OK D=HOME
    void footer_paywait(); // C=CANCEL TAP CARD... D=HOME
    void footer_none();

}