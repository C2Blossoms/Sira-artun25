#include <arduino.h>
#include "rfid.h"
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 33

static MFRC522 mfrc522(SS_PIN, RST_PIN);

void rfid_begin()
{
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("RFID ready.");
}

String rfid_read_uid()
{
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        return "";
    }

    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        if (mfrc522.uid.uidByte[i] < 0x10)
            uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return uid;
}