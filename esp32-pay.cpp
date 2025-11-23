#include <Arduino.h>
#include <Wire.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ---------------- Config ----------------
const char* ssid     = "Tonliw";      
const char* password = "77777772";
const char* serverBaseUrl = "http://172.20.10.6:8000/pay"; 

// ---------------- RFID ----------------
#define SS_PIN   5
#define RST_PIN  15
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ---------------- Keypad I2C ----------------
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
#define KEYPAD_ADDR 0x20
byte rowPins[ROWS] = {0,1,2,3};
byte colPins[COLS] = {4,5,6,7};
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, KEYPAD_ADDR, 1, &Wire);

// ---------------- LCD I2C ----------------
#define LCD_ADDR 0x27
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);  // LCD 20x4

// ---------------- Buzzer ----------------
const int buzzerPin = 4;

// 🎵 Notes
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392

// ✅ Success melody (3 เสียง)
int successMelody[] = {NOTE_C4, NOTE_E4, NOTE_G4};
int successDurations[] = {4, 4, 2};

// ❌ Fail melody (ติ๊ด ติ๊ด ติ๊ดดดด)
int failMelody[] = {NOTE_G4, NOTE_G4, NOTE_G4};
int failDurations[] = {8, 8, 2};

// ---------------- Melody functions ----------------
void playMelody(int melody[], int durations[], int length) {
  for (int i = 0; i < length; i++) {
    int noteDuration = 1000 / durations[i];
    tone(buzzerPin, melody[i], noteDuration);
    delay(noteDuration * 1.3);
    noTone(buzzerPin);
  }
}

void beepClick() {
  tone(buzzerPin, 800, 70); // ติ๊ดสั้นทุกปุ่ม
}

void beepScan() {
  tone(buzzerPin, 1000, 250); // ติ๊ดยาวตอนสแกนบัตร
}

void beepCancel() {
  tone(buzzerPin, 300, 400); // ติ๊ดยาวต่ำๆ ตอนยกเลิก
  noTone(buzzerPin);
}

// ---------------- State Machine ----------------
String amountStr = "";
String vendorId = "1";
unsigned long lastInputTime = 0;
const int TIMEOUT_MS = 15000;

enum DeviceState { STATE_IDLE, STATE_INPUT, STATE_WAIT_SCAN, STATE_PROCESSING };
DeviceState state = STATE_IDLE;

// ---------------- Helpers ----------------
void centerText(int row, String text) {
  int col = (20 - text.length()) / 2;
  if (col < 0) col = 0;
  lcd.setCursor(col, row);
  lcd.print(text);
}

void resetState() {
  amountStr = "";
  state = STATE_IDLE;
  lcd.clear();
  centerText(1, "Enter amount");
  centerText(2, "Press # to confirm");
  Serial.println("[STATE] Reset → Ready for new input");
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
  Serial.println("[RFID] UID: " + uid);
  return uid;
}

// ---------------- HTTP ----------------
bool sendPayment(String cardId, int amount, String vendorId) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] WiFi not connected");
    return false;
  }

  String url = String(serverBaseUrl) + "/" + cardId + "/" + String(amount) + "/" + vendorId;
  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.POST("");

  Serial.print("[HTTP] POST URL: "); Serial.println(url);
  Serial.print("[HTTP] Response code: "); Serial.println(httpResponseCode);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("[HTTP] Response body: " + response);
    http.end();
    return true;
  } else {
    Serial.println("[HTTP] Failed, code: " + String(httpResponseCode));
    http.end();
    return false;
  }
}


// ---------------- Loading Bar ----------------
byte barEmpty[8]  = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; // ว่าง
byte barFull[8]   = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}; // เต็ม

void initProgressBar() {
  lcd.createChar(0, barEmpty);
  lcd.createChar(1, barFull);
}

// ฟังก์ชันโชว์ progress bar
void showProgressBar(int row, int durationMs) {
  int barLength = 20; // ยาวเต็มบรรทัด
  int steps = barLength;
  int delayStep = durationMs / steps;

  lcd.clear();
  lcd.setCursor(6, row-1);
  lcd.print("Loading...");

  for (int i = 0; i <= steps; i++) {
    lcd.setCursor(0, row);
    for (int j = 0; j < barLength; j++) {
      if (j < i) lcd.write(byte(1)); // เติมเต็ม
      else lcd.write(byte(0));       // ช่องว่าง
    }
    delay(delayStep);
  }
}

// ---------------- Loading Animation ----------------
// void showLoading(int row, int durationMs) {
//   unsigned long start = millis();
//   int dotCount = 0;

//   while (millis() - start < durationMs) {
//     lcd.clear();
//     String txt = "Processing";
//     for (int i = 0; i < dotCount; i++) txt += ".";
//     centerText(row, txt);  // ใช้ centerText แทน
//     dotCount = (dotCount + 1) % 4;
//     delay(500);
//   }
// }


// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);

  // LCD
  lcd.init();
  lcd.backlight();
  centerText(1, "Connecting WiFi...");

  // Keypad
  keypad.begin();

  // RFID
  SPI.begin(18,19,23,SS_PIN);
  mfrc522.PCD_Init();

  // Buzzer
  pinMode(buzzerPin, OUTPUT);
  noTone(buzzerPin);

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n[WIFI] Connected: " + WiFi.localIP().toString());

  lcd.clear();
  centerText(1, "System Ready");
  centerText(2, "Enter amount...");
  delay(1000);

  resetState();
}

// ---------------- Loop ----------------
void loop() {
  char key = keypad.getKey();

  if (key && state != STATE_WAIT_SCAN) beepClick(); // ปุ่มทั่วไป (ยกเว้นรอการ์ด)

  if (state == STATE_IDLE) {
    if (key >= '0' && key <= '9') {
      amountStr += key;
      lcd.clear();
      String txt = "Amount: " + amountStr;
      centerText(1, txt);
      lastInputTime = millis();
      state = STATE_INPUT;
    }
  }

  else if (state == STATE_INPUT) {
    if (key >= '0' && key <= '9') {
      amountStr += key;
      lcd.clear();
      String txt = "Amount: " + amountStr;
      centerText(1, txt);
      lastInputTime = millis();
    }
    if (key == '#') {
      lcd.clear();
      centerText(1, "Place your card...");
      Serial.println("[STATE] Waiting for card...");
      lastInputTime = millis();
      state = STATE_WAIT_SCAN;
    }
    if (key == '*') {
      resetState();
    }
    if (millis() - lastInputTime > TIMEOUT_MS) {
      Serial.println("[TIMEOUT] Input expired.");
      resetState();
    }
  }

  else if (state == STATE_WAIT_SCAN) {
    if (key == '*') { 
      Serial.println("[STATE] Cancel by user.");
      beepCancel();
      resetState();
    }
    else if (rfidAvailable()) {
      String uid = readUID();
      beepScan(); // ติ๊ดยาวตอนสแกน
      state = STATE_PROCESSING;

      // Loading
      lcd.init();
      lcd.backlight();
      initProgressBar();

      lcd.clear();
      showProgressBar(2, 2000);

      bool ok = sendPayment(uid, amountStr.toInt(), vendorId);
      lcd.clear();
      if (ok) {
        centerText(1, "Payment SUCCESS");
        playMelody(successMelody, successDurations, 3);
      } else {
        centerText(1, "Payment FAIL");
        playMelody(failMelody, failDurations, 3);
      }
      delay(3000);
      resetState();
    }
    if (millis() - lastInputTime > TIMEOUT_MS) {
      Serial.println("[TIMEOUT] Waiting for card expired.");
      resetState();
    }
  }
}
