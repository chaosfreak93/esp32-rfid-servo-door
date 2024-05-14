#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

#define SERVO_PIN 12 // MG90S
#define SS_PIN  5 // RFID-RC522
#define RST_PIN 27 // RFID-RC522

Servo servo1;
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  servo1.attach(SERVO_PIN);
  servo1.write(0);
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return; // new tag is not available or NUID hasn't been readed
  // MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  // Serial.print("RFID/NFC Tag Type: ");
  // Serial.println(rfid.PICC_GetTypeName(piccType));

  // print UID in Serial Monitor in the hex format
  String readRFID = "";
  for (int i = 0; i < rfid.uid.size; i++) {
    readRFID.concat(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    readRFID.concat(String(rfid.uid.uidByte[i], HEX));
  }
  readRFID.toUpperCase();
  Serial.println(readRFID);

  rfid.PICC_HaltA(); // halt PICC
  rfid.PCD_StopCrypto1(); // stop encryption on PCD

  if (readRFID == allowedRFID) {
    servo1.write(180);
    delay(5000);
    servo1.write(0);
    delay(1000);
  }
}
