#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>

// ---------------- Config ----------------
const char* ssid     = "TP-Link_Outdoor_1B990E";
const char* password = "29112547";
const char* serverBaseUrl = "http://127.0.0.1:8000/pay"; // base URL ของ FastAPI

// RFID
#define SS_PIN  5
#define RST_PIN 22
MFRC522 mfrc522(SS_PIN, RST_PIN);

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26}; 
byte colPins[COLS] = {27, 14, 12, 13}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Buzzer
const int buzzerPin = 4;

// Timeout
unsigned long lastInputTime = 0;
const int TIMEOUT_MS = 15000;

// Session variables
String amountStr = "";
String vendorId = "VENDOR01";   // vendor_id fix ไว้เลย
String referenceId = "";

enum State { IDLE, INPUT, WAIT_SCAN, PROCESSING };
State state = IDLE;

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.backlight();

  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  lcd.setCursor(0,0); lcd.print("WiFi Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  lcd.clear();
  lcd.print("Ready");
  delay(1000);
  resetState();
}

// ---------------- Loop ----------------
void loop() {
  char key = keypad.getKey();

  if (state == IDLE) {
    if (key >= '0' && key <= '9') {
      amountStr += key;
      lcd.clear(); lcd.print("Amount: " + amountStr);
      lastInputTime = millis();
      state = INPUT;
    }
  }

  else if (state == INPUT) {
    if (key >= '0' && key <= '9') {
      amountStr += key;
      lcd.setCursor(0,1); lcd.print(amountStr);
      lastInputTime = millis();
    }
    if (key == '#') { // OK
      referenceId = generateRefId();
      lcd.clear(); lcd.print("Scan your card");
      lastInputTime = millis();
      state = WAIT_SCAN;
    }
    if (key == '*') { // Cancel
      resetState();
    }
    if (millis() - lastInputTime > TIMEOUT_MS) {
      resetState();
    }
  }

  else if (state == WAIT_SCAN) {
    if (rfidAvailable()) {
      String uid = readUID();
      state = PROCESSING;
      bool ok = sendPayment(uid, amountStr.toInt(), vendorId);
      if (ok) {
        lcd.clear(); lcd.print("สำเร็จ");
        beepSuccess();
      } else {
        lcd.clear(); lcd.print("ล้มเหลว");
        beepFail();
      }
      delay(3000);
      resetState();
    }
    if (millis() - lastInputTime > TIMEOUT_MS) {
      resetState();
    }
  }
}

// ---------------- Helpers ----------------
void resetState() {
  amountStr = "";
  lcd.clear(); lcd.print("กรอกจำนวนเงิน");
  state = IDLE;
}

void beepSuccess() {
  tone(buzzerPin, 2000, 200); // บี๊บยาว
}

void beepFail() {
  for(int i=0;i<3;i++){
    tone(buzzerPin, 1000, 100);
    delay(150);
  }
}

// ---------------- RFID ----------------
bool rfidAvailable() {
  return mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial();
}

String readUID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  mfrc522.PICC_HaltA();
  return uid;
}

// ---------------- Utils ----------------
String generateRefId() {
  return String((unsigned long)millis()) + String(random(1000,9999));
}

// ---------------- HTTP ----------------
bool sendPayment(String cardId, int amount, String vendorId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }

  // ประกอบ URL ตาม path parameter
  String url = String(serverBaseUrl) + "/" + cardId + "/" + String(amount) + "/" + vendorId;

  HTTPClient http;
  http.begin(url);

  // ยิง POST แบบไม่มี body
  int httpResponseCode = http.POST("");

  Serial.print("POST URL: "); Serial.println(url);
  Serial.print("Response code: "); Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    return true;
  } else {
    http.end();
    return false;
  }
}
