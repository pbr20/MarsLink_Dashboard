#include <SPI.h>
#include <LoRa.h>

// =====================================================
// MASTER LoRa PINS
// =====================================================

#define LORA_SCK   42
#define LORA_MOSI  41
#define LORA_MISO  47
#define LORA_SS    14
#define LORA_RST   21
#define LORA_DIO0  1


// =====================================================
// DEVICE IDs
// =====================================================

#define MASTER_ID  0xFF
#define NODE1_ID   0xBB
#define NODE2_ID   0xCC


// =====================================================
// TIMING
// =====================================================

unsigned long lastCycle = 0;

const unsigned long CYCLE_INTERVAL = 5000;
const unsigned long RESPONSE_TIMEOUT = 2000;


// =====================================================
// MESSAGE ID
// =====================================================

byte messageID = 0;


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("================================");
  Serial.println("        MARS LINK MASTER");
  Serial.println("================================");


  // ===================================================
  // SPI
  // ===================================================

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );


  // ===================================================
  // LORA PINS
  // ===================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );


  // ===================================================
  // START LORA
  // ===================================================

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa FAILED!");

    while (1);
  }


  // ===================================================
  // RADIO SETTINGS
  // ===================================================

  LoRa.setSpreadingFactor(7);

  LoRa.setSignalBandwidth(62.5E3);

  LoRa.setCodingRate4(5);

  LoRa.setSyncWord(0x12);

  LoRa.enableCrc();

  LoRa.setTxPower(17);


  // ===================================================
  // INFORMATION
  // ===================================================

  Serial.println("LoRa SUCCESS!");

  Serial.println();
  Serial.println("Frequency : 433 MHz");
  Serial.println("SF        : 7");
  Serial.println("BW        : 125 kHz");
  Serial.println("CR        : 4/5");
  Serial.println("SyncWord  : 0x12");
  Serial.println("CRC       : ON");

  Serial.println();
  Serial.println("MASTER READY");

  Serial.println();
  Serial.println("ROUTING ORDER:");
  Serial.println("MASTER -> NODE 1 (BB)");
  Serial.println("MASTER -> NODE 2 (CC)");

  Serial.println();

  LoRa.receive();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // RUN NETWORK CYCLE
  // ===================================================

  if (millis() - lastCycle >= CYCLE_INTERVAL) {

    lastCycle = millis();

    Serial.println();
    Serial.println();
    Serial.println("################################");
    Serial.println("#       NETWORK CYCLE          #");
    Serial.println("################################");


    // =================================================
    // FIRST NODE 1
    // =================================================

    bool node1Received = requestNode(
      NODE1_ID,
      "10"
    );


    // =================================================
    // THEN NODE 2
    // =================================================

    bool node2Received = requestNode(
      NODE2_ID,
      "10"
    );


    // =================================================
    // CYCLE RESULT
    // =================================================

    Serial.println();
    Serial.println("################################");
    Serial.println("       CYCLE COMPLETE");
    Serial.println("################################");

    Serial.print("Node 1: ");

    if (node1Received) {
      Serial.println("ONLINE");
    }
    else {
      Serial.println("OFFLINE / NO RESPONSE");
    }

    Serial.print("Node 2: ");

    if (node2Received) {
      Serial.println("ONLINE");
    }
    else {
      Serial.println("OFFLINE / NO RESPONSE");
    }

    Serial.println("################################");

    LoRa.receive();
  }
}


// =====================================================
// REQUEST NODE
// =====================================================

bool requestNode(
  byte nodeID,
  String command
) {

  // ===================================================
  // MESSAGE ID
  // ===================================================

  messageID++;


  // ===================================================
  // NODE NAME
  // ===================================================

  Serial.println();
  Serial.println("--------------------------------");

  if (nodeID == NODE1_ID) {

    Serial.println("      REQUESTING NODE 1");
    Serial.println("      NODE ID: 0xBB");
  }

  else if (nodeID == NODE2_ID) {

    Serial.println("      REQUESTING NODE 2");
    Serial.println("      NODE ID: 0xCC");
  }

  Serial.println("--------------------------------");


  Serial.print("Command    : ");
  Serial.println(command);

  Serial.print("Message ID : ");
  Serial.println(messageID);


  // ===================================================
  // SEND REQUEST
  // ===================================================

  LoRa.idle();

  LoRa.beginPacket();

  // Destination
  LoRa.write(nodeID);

  // Sender
  LoRa.write(MASTER_ID);

  // Message ID
  LoRa.write(messageID);

  // Command
  LoRa.print(command);

  LoRa.endPacket();


  Serial.println("REQUEST SENT");


  // ===================================================
  // WAIT FOR RESPONSE
  // ===================================================

  LoRa.receive();

  unsigned long startTime = millis();


  while (millis() - startTime < RESPONSE_TIMEOUT) {

    int packetSize = LoRa.parsePacket();


    if (packetSize > 0) {

      // ===============================================
      // READ HEADER
      // ===============================================

      byte destination = LoRa.read();

      byte sender = LoRa.read();

      byte receivedID = LoRa.read();


      // ===============================================
      // READ PAYLOAD
      // ===============================================

      String payload = "";

      while (LoRa.available()) {

        payload += (char)LoRa.read();
      }


      // ===============================================
      // RADIO DATA
      // ===============================================

      long rssi = LoRa.packetRssi();

      float snr = LoRa.packetSnr();


      // ===============================================
      // CHECK RESPONSE
      // ===============================================

      if (destination != MASTER_ID) {

        continue;
      }

      if (sender != nodeID) {

        continue;
      }

      if (receivedID != messageID) {

        continue;
      }


      // ===============================================
      // NODE 1
      // ===============================================

      if (sender == NODE1_ID) {

        Serial.println();
        Serial.println("================================");
        Serial.println("       NODE 1 RESPONSE");
        Serial.println("================================");

        Serial.println("Node       : NODE 1");
        Serial.println("ID         : 0xBB");

        Serial.print("Temperature: ");
        Serial.print(payload);
        Serial.println(" C");

        Serial.print("RSSI       : ");
        Serial.print(rssi);
        Serial.println(" dBm");

        Serial.print("SNR        : ");
        Serial.print(snr);
        Serial.println(" dB");

        Serial.print("Message ID : ");
        Serial.println(receivedID);

        Serial.println("Status     : ONLINE");

        Serial.println("================================");
      }


      // ===============================================
      // NODE 2
      // ===============================================

      else if (sender == NODE2_ID) {

        Serial.println();
        Serial.println("================================");
        Serial.println("       NODE 2 RESPONSE");
        Serial.println("================================");

        Serial.println("Node       : NODE 2");
        Serial.println("ID         : 0xCC");

        Serial.print("Temperature: ");
        Serial.print(payload);
        Serial.println(" C");

        Serial.print("RSSI       : ");
        Serial.print(rssi);
        Serial.println(" dBm");

        Serial.print("SNR        : ");
        Serial.print(snr);
        Serial.println(" dB");

        Serial.print("Message ID : ");
        Serial.println(receivedID);

        Serial.println("Status     : ONLINE");

        Serial.println("================================");
      }


      // ===============================================
      // SEND DATA TO PYTHON DASHBOARD
      // ===============================================

      Serial.print("DATA|NODE=");
      Serial.print(sender, HEX);

      Serial.print("|TEMP=");
      Serial.print(payload);

      Serial.print("|RSSI=");
      Serial.print(rssi);

      Serial.print("|SNR=");
      Serial.print(snr);

      Serial.print("|MSG=");
      Serial.println(receivedID);


      LoRa.receive();

      return true;
    }
  }


  // ===================================================
  // NO RESPONSE
  // ===================================================

  Serial.println();
  Serial.println("--------------------------------");

  if (nodeID == NODE1_ID) {
    Serial.println("NODE 1 NO RESPONSE");
  }

  else if (nodeID == NODE2_ID) {
    Serial.println("NODE 2 NO RESPONSE");
  }

  Serial.println("Status: OFFLINE / COMMUNICATION FAILURE");

  Serial.println("--------------------------------");


  LoRa.receive();

  return false;
}