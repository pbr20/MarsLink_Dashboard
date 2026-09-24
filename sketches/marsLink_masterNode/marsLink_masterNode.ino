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
#define NODE3_ID   0xDD


// =====================================================
// TIMING
// =====================================================

// Time before starting a new complete cycle
const unsigned long CYCLE_INTERVAL = 5000;

// How long Master waits for a node response
const unsigned long RESPONSE_TIMEOUT = 2000;

// Time between Node 1 -> Node 2 -> Node 3
// CHANGE THIS VALUE TO TEST DIFFERENT INTERVALS
const unsigned long NODE_INTERVAL = 5000;


// =====================================================
// VARIABLES
// =====================================================

unsigned long lastCycle = 0;

byte messageID = 0;


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("################################");
  Serial.println("#       MARSLINK MASTER        #");
  Serial.println("################################");
  Serial.println();

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
  // LoRa PINS
  // ===================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  // ===================================================
  // START LoRa
  // ===================================================

  Serial.println("Starting LoRa...");

  if (!LoRa.begin(433E6)) {

    Serial.println("ERROR: LoRa initialization failed!");

    while (1) {
      delay(1000);
    }
  }

  Serial.println("LoRa initialized successfully!");
  Serial.println();

  // ===================================================
  // LoRa SETTINGS
  // ===================================================

  LoRa.setSpreadingFactor(7);

  // 62.5 kHz
  LoRa.setSignalBandwidth(62.5E3);

  LoRa.setCodingRate4(5);

  LoRa.setSyncWord(0x12);

  LoRa.enableCrc();

  LoRa.setTxPower(17);

  // ===================================================
  // NETWORK INFORMATION
  // ===================================================

  Serial.println("--------------------------------");
  Serial.println("NETWORK CONFIGURATION");
  Serial.println("--------------------------------");

  Serial.print("Master ID : 0x");
  Serial.println(MASTER_ID, HEX);

  Serial.print("Node 1 ID : 0x");
  Serial.println(NODE1_ID, HEX);

  Serial.print("Node 2 ID : 0x");
  Serial.println(NODE2_ID, HEX);

  Serial.print("Node 3 ID : 0x");
  Serial.println(NODE3_ID, HEX);

  Serial.println();

  Serial.println("--------------------------------");
  Serial.println("TIMING CONFIGURATION");
  Serial.println("--------------------------------");

  Serial.print("Cycle interval : ");
  Serial.print(CYCLE_INTERVAL);
  Serial.println(" ms");

  Serial.print("Node interval  : ");
  Serial.print(NODE_INTERVAL);
  Serial.println(" ms");

  Serial.print("Response timeout: ");
  Serial.print(RESPONSE_TIMEOUT);
  Serial.println(" ms");

  Serial.println();
  Serial.println("Master ready.");
  Serial.println();

  // Start listening
  LoRa.receive();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // ===================================================
  // START NEW CYCLE
  // ===================================================

  if (millis() - lastCycle >= CYCLE_INTERVAL) {

    lastCycle = millis();

    Serial.println();
    Serial.println("================================");
    Serial.println("        NETWORK CYCLE");
    Serial.println("================================");


    // =================================================
    // NODE 1
    // =================================================

    Serial.println("--------------------------------");
    Serial.println("REQUESTING NODE 1");
    Serial.println("--------------------------------");

    bool node1Received = requestNode(
      NODE1_ID,
      "10"
    );


    // =================================================
    // WAIT BEFORE NODE 2
    // =================================================

    Serial.println();
    Serial.println("--------------------------------");
    Serial.print("WAITING ");
    Serial.print(NODE_INTERVAL);
    Serial.println(" ms BEFORE NODE 2");
    Serial.println("--------------------------------");

    delay(NODE_INTERVAL);


    // =================================================
    // NODE 2
    // =================================================

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("REQUESTING NODE 2");
    Serial.println("--------------------------------");

    bool node2Received = requestNode(
      NODE2_ID,
      "10"
    );


    // =================================================
    // WAIT BEFORE NODE 3
    // =================================================

    Serial.println();
    Serial.println("--------------------------------");
    Serial.print("WAITING ");
    Serial.print(NODE_INTERVAL);
    Serial.println(" ms BEFORE NODE 3");
    Serial.println("--------------------------------");

    delay(NODE_INTERVAL);


    // =================================================
    // NODE 3
    // =================================================

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("REQUESTING NODE 3");
    Serial.println("--------------------------------");

    bool node3Received = requestNode(
      NODE3_ID,
      "10"
    );


    // =================================================
    // WAIT BEFORE STARTING NODE 1 AGAIN
    // =================================================

    Serial.println();
    Serial.println("--------------------------------");
    Serial.print("WAITING ");
    Serial.print(NODE_INTERVAL);
    Serial.println(" ms BEFORE NODE 1");
    Serial.println("--------------------------------");

    delay(NODE_INTERVAL);


    // =================================================
    // CYCLE COMPLETE
    // =================================================

    Serial.println();
    Serial.println("================================");
    Serial.println("        CYCLE COMPLETE");
    Serial.println("================================");

    Serial.print("Node 1: ");
    Serial.println(node1Received ? "ONLINE" : "OFFLINE");

    Serial.print("Node 2: ");
    Serial.println(node2Received ? "ONLINE" : "OFFLINE");

    Serial.print("Node 3: ");
    Serial.println(node3Received ? "ONLINE" : "OFFLINE");

    Serial.println("================================");
    Serial.println();


    // Return to receive mode
    LoRa.receive();
  }
}


// =====================================================
// REQUEST NODE
// =====================================================

bool requestNode(byte nodeID, String command) {

  // Increase message ID
  messageID++;

  // Prevent 0 if it overflows
  if (messageID == 0) {
    messageID = 1;
  }


  // ===================================================
  // PRINT REQUEST INFORMATION
  // ===================================================

  Serial.print("NODE ID: 0x");
  Serial.println(nodeID, HEX);

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


  Serial.println("Request sent.");
  Serial.println("Waiting for response...");


  // ===================================================
  // SWITCH TO RECEIVE MODE
  // ===================================================

  LoRa.receive();

  unsigned long startTime = millis();


  // ===================================================
  // WAIT FOR RESPONSE
  // ===================================================

  while (millis() - startTime < RESPONSE_TIMEOUT) {

    int packetSize = LoRa.parsePacket();

    if (packetSize) {

      // ===============================================
      // READ HEADER
      // ===============================================

      byte destination = LoRa.read();
      byte sender      = LoRa.read();
      byte receivedID  = LoRa.read();


      // ===============================================
      // READ PAYLOAD
      // ===============================================

      String payload = "";

      while (LoRa.available()) {
        payload += (char)LoRa.read();
      }


      // ===============================================
      // SIGNAL INFORMATION
      // ===============================================

      int rssi = LoRa.packetRssi();

      float snr = LoRa.packetSnr();


      // ===============================================
      // PRINT RECEIVED PACKET
      // ===============================================

      Serial.println();
      Serial.println("******** RESPONSE RECEIVED ********");

      Serial.print("Destination : 0x");
      Serial.println(destination, HEX);

      Serial.print("Sender      : 0x");
      Serial.println(sender, HEX);

      Serial.print("Message ID  : ");
      Serial.println(receivedID);

      Serial.print("Temperature : ");
      Serial.println(payload);

      Serial.print("RSSI        : ");
      Serial.print(rssi);
      Serial.println(" dBm");

      Serial.print("SNR         : ");
      Serial.print(snr);
      Serial.println(" dB");


      // ===============================================
      // VALIDATE RESPONSE
      // ===============================================

      if (
        destination == MASTER_ID &&
        sender == nodeID &&
        receivedID == messageID
      ) {

        Serial.println("STATUS      : VALID RESPONSE");

        // =============================================
        // DASHBOARD DATA
        // =============================================

        Serial.print("DATA|NODE=");

        if (sender < 16) {
          Serial.print("0");
        }

        Serial.print(sender, HEX);

        Serial.print("|TEMP=");
        Serial.print(payload);

        Serial.print("|RSSI=");
        Serial.print(rssi);

        Serial.print("|SNR=");
        Serial.print(snr);

        Serial.print("|MSG=");
        Serial.println(receivedID);


        Serial.println("************************************");

        return true;
      }

      else {

        Serial.println("STATUS      : INVALID RESPONSE");
        Serial.println("************************************");
      }
    }
  }


  // ===================================================
  // TIMEOUT
  // ===================================================

  Serial.println();
  Serial.println("******** NO RESPONSE ********");

  Serial.print("Node 0x");
  Serial.print(nodeID, HEX);
  Serial.println(" did not respond.");

  Serial.println("STATUS      : OFFLINE");
  Serial.println("*****************************");

  return false;
}