#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESP32_MySQL.h>

#define ESP32_MYSQL_DEBUG_PORT Serial

// Debug Level from 0 to 4
#define _ESP32_MYSQL_LOGLEVEL_ 1

#define SERVO_PIN 12  // MG90S
#define SS_PIN 5      // RFID-RC522
#define RST_PIN 27    // RFID-RC522

#define WLAN_SSID "BSIT22a_2023"
#define WLAN_PASSWORD "NoteredMQTTESP32"

IPAddress server(172, 20, 8, 21);
uint16_t server_port = 3306;
ESP32_MySQL_Connection conn((Client *)&client);
ESP32_MySQL_Query sql_query = ESP32_MySQL_Query(&conn);

String query = String("SELECT rfid FROM schranke.allowedChips;");

Servo servo1;
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial && millis() < 5000)
    ;

  ESP32_MYSQL_DISPLAY1("Connecting to", WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    ESP32_MYSQL_DISPLAY0(".");
  }

  ESP32_MYSQL_DISPLAY1("Connected to network. My IP address is:", WiFi.localIP());

  servo1.attach(SERVO_PIN);
  servo1.write(0);
  SPI.begin();      // init SPI bus
  rfid.PCD_Init();  // init MFRC522
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
  while (1) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {

      // print UID in Serial Monitor in the hex format
      String readRFID = "";
      for (int i = 0; i < rfid.uid.size; i++) {
        readRFID.concat(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        readRFID.concat(String(rfid.uid.uidByte[i], HEX));
      }
      readRFID.toUpperCase();
      Serial.println(readRFID);

      rfid.PICC_HaltA();       // halt PICC
      rfid.PCD_StopCrypto1();  // stop encryption on PCD

      //if (readRFID == allowedRFID) {
      servo1.write(180);
      delay(5000);
      servo1.write(0);
      delay(1000);
      //}
    }
  }
}

void runQuery() {
  ESP32_MySQL_Query query_mem = ESP32_MySQL_Query(&conn);

  // Execute the query
  ESP32_MYSQL_DISPLAY(query);

  if (!query_mem.execute(query.c_str())) {
    ESP32_MYSQL_DISPLAY("Querying error");
    return;
  }

  // Show the result
  // Fetch the columns and print them
  column_names *cols = query_mem.get_columns();
  // Read the rows and print them
  row_values *row = NULL;

  do {
    row = query_mem.get_next_row();

    if (row != NULL) {
      for (int f = 0; f < cols->num_fields; f++) {
        ESP32_MYSQL_DISPLAY0(row->values[f]);
      }
    }
  } while (row != NULL);

  delay(500);
}

void refreshDataset() {
  ESP32_MYSQL_DISPLAY("Connecting...");

  //if (conn.connect(server, server_port, user, password))
  if (conn.connect(server, server_port, "esp32", "Schranke")) {
    delay(500);
    runQuery();
    conn.close();  // close the connection
  } else {
    ESP32_MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
  }

  ESP32_MYSQL_DISPLAY("\nSleeping...");
  ESP32_MYSQL_DISPLAY("================================================");

  delay(10000);
}
