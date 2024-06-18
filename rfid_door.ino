#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESP32_MySQL.h>

#define SERVO_PIN 12  // MG90S
#define SS_PIN 5      // RFID-RC522
#define RST_PIN 27    // RFID-RC522

#define WLAN_SSID "BSIT22a_2023"
#define WLAN_PASSWORD "NoteredMQTTESP32"

IPAddress server(172, 20, 8, 21);
uint16_t server_port = 3306;
ESP32_MySQL_Connection conn((Client *)&client);

String query = String("SELECT rfid FROM schranke.allowedChips;");

Servo servo1;
MFRC522 rfid(SS_PIN, RST_PIN);

String allowedRFIDs[1000];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial && millis() < 5000)
    ;

  Serial.print("Connecting to Wi-Fi ");
  Serial.print(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to Wi-Fi. My IP: ");
  Serial.println(WiFi.localIP());

  servo1.attach(SERVO_PIN);
  servo1.write(0);
  SPI.begin();      // init SPI bus
  rfid.PCD_Init();  // init MFRC522
  delay(10);
  xTaskCreatePinnedToCore(
    readRFID,    // Function to implement the task
    "readRFID",  // Name of the task
    1000,        // Stack size in bytes
    NULL,        // Task input parameter
    0,           // Priority of the task
    NULL,        // Task handle.
    1);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) return;
  refreshDataset();
}

void readRFID(void *pvParameters) {
  for (;;) {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) continue;

    // print UID in Serial Monitor in the hex format
    String readRFID = "";
    for (int i = 0; i < rfid.uid.size; i++) {
      readRFID.concat(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      readRFID.concat(String(rfid.uid.uidByte[i], HEX));
    }
    readRFID.toUpperCase();

    rfid.PICC_HaltA();       // halt PICC
    rfid.PCD_StopCrypto1();  // stop encryption on PCD

    if (allowedRFIDs == nullptr) continue;
    for (int f = 0; f < (sizeof(allowedRFIDs) / sizeof(String)); f++) {
      if (allowedRFIDs[f] != readRFID) continue;
      servo1.write(180);
      delay(5000);
      servo1.write(0);
      delay(1000);
    }
  }
}

void runQuery() {
  ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);

  if (!query_mem.execute(query.c_str())) {
    Serial.println("Querying error");
    return;
  }

  // Show the result
  // Fetch the columns and print them
  column_names *cols = query_mem.get_columns();
  // Read the rows and print them
  row_values *row = NULL;
  int rfidCount = 0;

  if (allowedRFIDs != nullptr) {
    for (int f = 0; f < (sizeof(allowedRFIDs) / sizeof(String)); f++) {
      allowedRFIDs[f] = "";
    }
  }

  do {
    row = query_mem.get_next_row();

    if (row != NULL) {
      for (int f = 0; f < cols->num_fields; f++) {
        allowedRFIDs[rfidCount] = row->values[f];
      }

      rfidCount++;
    }
  } while (row != NULL);

  delay(500);
}

void refreshDataset() {
  Serial.println("Connecting to DB...");

  if (conn.connectNonBlocking(server, server_port, "esp32", "Schranke") == RESULT_OK) {
    delay(750);
    runQuery();
    conn.close();
  } else {
    Serial.println("Connect failed. Trying again on next iteration.");
  }

  Serial.println("Refreshed Dataset.");

  delay(30000);
}
