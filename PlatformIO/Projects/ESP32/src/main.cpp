// #include <Arduino.h>

// // put function declarations here:
// int myFunction(int, int);

// void setup() {
//   // put your setup code here, to run once:
//   int result = myFunction(2, 3);
// }

// void loop() {
//   // put your main code here, to run repeatedly:
// }

// // put function definitions here:
// int myFunction(int x, int y) {
//   return x + y;
// }

#include <Arduino.h>

#define LED1 1
#define LED2 2  // บางบอร์ดใช้ขา 2 เป็น LED ในตัว

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.begin(115200);
  Serial.println("ESP32 Test Started");
}

void loop() {
  digitalWrite(LED1, HIGH);
  delay(500);
  digitalWrite(LED2, HIGH);
  delay(500);
  digitalWrite(LED1, LOW);
  delay(500);
  digitalWrite(LED2, LOW);
  delay(500);
}