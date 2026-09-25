
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

// =====================================================
// LORA PINS
// =====================================================

#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2


// =====================================================
// DHT11
// =====================================================

#define DHTPIN 26
#define DHTTYPE DHT11

DHT dht(
  DHTPIN,
  DHTTYPE
);


// =====================================================
// LED
// =====================================================

#define RESPONSE_LED 16


// =====================================================
// DEVICE IDs
// =====================================================

#define NODE1_ID   0xBB
#define NODE3_ID   0xDD
#define MASTER_ID  0xFF


// =====================================================
// RELAY SETTINGS
// =====================================================

// How long Node 1 waits for Node 3 response
const unsigned long NODE3_RESPONSE_TIMEOUT = 3000;


// =====================================================
// LED SETTINGS
// =====================================================

// Normal Node 1 response
const unsigned long NODE1_BLINK_TIME = 300;

// Node 3 relay indication
const unsigned long NODE3_BLINK_ON_TIME  = 500;
const unsigned long NODE3_BLINK_OFF_TIME = 350;


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("================================");
  Serial.println("        NODE 1 / RELAY");
  Serial.println("================================");


  // ===================================================
  // LED
  // ===================================================

  pinMode(RESPONSE_LED, OUTPUT);

  digitalWrite(RESPONSE_LED, LOW);


  // ===================================================
  // DHT
  // ===================================================

  dht.begin();


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

  Serial.println("Node ID    : 0xBB");
  Serial.println("Relay Node : 0xDD");
  Serial.println("Frequency  : 433 MHz");
  Serial.println("SF         : 7");
  Serial.println("BW         : 62.5 kHz");
  Serial.println("CR         : 4/5");
  Serial.println("SyncWord   : 0x12");
  Serial.println("CRC        : ON");

  Serial.println();
  Serial.println("DHT11 READY");
  Serial.println("LED PIN    : GPIO 16");

  Serial.println();
  Serial.println("NODE 1 MODE:");
  Serial.println("Master -> Node 1 -> Master");
  Serial.println("Master -> Node 1 -> Node 3 -> Node 1 -> Master");

  Serial.println();
  Serial.println("LED INDICATION:");
  Serial.println("Node 1 response = 1 BLINK");
  Serial.println("Node 3 relay    = 2 BLINKS");

  Serial.println();
  Serial.println("WAITING FOR MASTER REQUEST...");


  // ===================================================
  // RX MODE
  // ===================================================

  LoRa.receive();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  int packetSize = LoRa.parsePacket();


  if (packetSize <= 0) {
    return;
  }


  // ===================================================
  // REQUEST RECEIVED
  // ===================================================

  Serial.println();
  Serial.println("================================");
  Serial.println("       PACKET RECEIVED");
  Serial.println("================================");


  // ===================================================
  // HEADER
  // ===================================================

  byte destination = LoRa.read();

  byte sender = LoRa.read();

  byte receivedID = LoRa.read();


  // ===================================================
  // COMMAND
  // ===================================================

  String command = "";

  while (LoRa.available()) {

    command += (char)LoRa.read();
  }


  // ===================================================
  // RADIO INFORMATION
  // ===================================================

  long rssi = LoRa.packetRssi();

  float snr = LoRa.packetSnr();


  Serial.print("Destination : 0x");
  Serial.println(destination, HEX);

  Serial.print("Sender      : 0x");
  Serial.println(sender, HEX);

  Serial.print("Message ID  : ");
  Serial.println(receivedID);

  Serial.print("Command     : ");
  Serial.println(command);

  Serial.print("RSSI        : ");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("SNR         : ");
  Serial.print(snr);
  Serial.println(" dB");


  // ===================================================
  // ONLY ACCEPT MASTER REQUESTS
  // ===================================================

  if (sender != MASTER_ID) {

    Serial.println("Unknown sender.");

    LoRa.receive();

    return;
  }


  // ===================================================
  // REQUEST FOR NODE 1
  // ===================================================

  if (destination == NODE1_ID) {

    handleNode1Request(
      receivedID,
      command
    );

    return;
  }


  // ===================================================
  // REQUEST FOR NODE 3
  // ===================================================

  if (destination == NODE3_ID) {

    handleNode3Relay(
      receivedID,
      command
    );

    return;
  }


  // ===================================================
  // UNKNOWN DESTINATION
  // ===================================================

  Serial.print("Unknown destination: 0x");
  Serial.println(destination, HEX);

  LoRa.receive();
}


// =====================================================
// NODE 1 NORMAL REQUEST
// =====================================================

void handleNode1Request(
  byte messageID,
  String command
) {

  Serial.println();
  Serial.println("--------------------------------");
  Serial.println("REQUEST IS FOR NODE 1");
  Serial.println("--------------------------------");


  // ===================================================
  // COMMAND 10 = TEMPERATURE
  // ===================================================

  if (command != "10") {

    Serial.println("Unknown command.");

    LoRa.receive();

    return;
  }


  Serial.println("MASTER REQUESTED NODE 1 TEMPERATURE");


  // ===================================================
  // READ DHT11
  // ===================================================

  float temperature = dht.readTemperature();


  if (isnan(temperature)) {

    Serial.println("DHT11 READ FAILED!");

    LoRa.receive();

    return;
  }


  Serial.print("Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" C");


  String data = String(
    temperature,
    1
  );


  // ===================================================
  // SEND RESPONSE TO MASTER
  // ===================================================

  LoRa.idle();

  LoRa.beginPacket();

  // Destination = Master
  LoRa.write(MASTER_ID);

  // Sender = Node 1
  LoRa.write(NODE1_ID);

  // Same message ID
  LoRa.write(messageID);

  // Temperature
  LoRa.print(data);

  LoRa.endPacket();


  // ===================================================
  // NODE 1 LED
  // ===================================================

  Serial.println();
  Serial.println("NODE 1 RESPONSE SUCCESS");
  Serial.println("LED: 1 BLINK");

  blinkNode1();


  // ===================================================
  // INFORMATION
  // ===================================================

  Serial.println();
  Serial.println("---------- NODE 1 RESPONSE ----------");

  Serial.print("Destination: 0x");
  Serial.println(MASTER_ID, HEX);

  Serial.print("Sender: 0x");
  Serial.println(NODE1_ID, HEX);

  Serial.print("Temperature: ");
  Serial.print(data);
  Serial.println(" C");

  Serial.print("Message ID: ");
  Serial.println(messageID);

  Serial.println("Response sent to MASTER");


  // ===================================================
  // BACK TO RX
  // ===================================================

  LoRa.receive();
}


// =====================================================
// NODE 3 RELAY
// =====================================================

void handleNode3Relay(
  byte masterMessageID,
  String command
) {

  Serial.println();
  Serial.println("================================");
  Serial.println("       NODE 3 RELAY MODE");
  Serial.println("================================");

  Serial.println("Master requested NODE 3.");
  Serial.println("Node 1 will relay the request.");


  // ===================================================
  // CHECK COMMAND
  // ===================================================

  if (command != "10") {

    Serial.println("Unknown command.");

    LoRa.receive();

    return;
  }


  // ===================================================
  // SEND REQUEST TO NODE 3
  // ===================================================

  Serial.println();
  Serial.println("Sending request to NODE 3...");


  LoRa.idle();

  LoRa.beginPacket();

  // Destination = Node 3
  LoRa.write(NODE3_ID);

  // Sender = Node 1
  LoRa.write(NODE1_ID);

  // Keep Master's original message ID
  LoRa.write(masterMessageID);

  // Command
  LoRa.print(command);

  LoRa.endPacket();


  Serial.println("Request sent to NODE 3.");

  Serial.print("Destination: 0x");
  Serial.println(NODE3_ID, HEX);

  Serial.print("Sender: 0x");
  Serial.println(NODE1_ID, HEX);

  Serial.print("Message ID: ");
  Serial.println(masterMessageID);


  // ===================================================
  // RECEIVE NODE 3 RESPONSE
  // ===================================================

  Serial.println();
  Serial.println("Waiting for NODE 3 response...");


  LoRa.receive();

  unsigned long startTime = millis();


  while (
    millis() - startTime <
    NODE3_RESPONSE_TIMEOUT
  ) {

    int packetSize = LoRa.parsePacket();


    if (packetSize <= 0) {
      continue;
    }


    // =================================================
    // READ NODE 3 RESPONSE HEADER
    // =================================================

    byte destination = LoRa.read();

    byte sender = LoRa.read();

    byte receivedID = LoRa.read();


    // =================================================
    // READ TEMPERATURE
    // =================================================

    String temperature = "";

    while (LoRa.available()) {

      temperature += (char)LoRa.read();
    }


    // =================================================
    // RADIO INFORMATION
    // =================================================

    long rssi = LoRa.packetRssi();

    float snr = LoRa.packetSnr();


    Serial.println();
    Serial.println("******** NODE 3 RESPONSE ********");

    Serial.print("Destination : 0x");
    Serial.println(destination, HEX);

    Serial.print("Sender      : 0x");
    Serial.println(sender, HEX);

    Serial.print("Message ID  : ");
    Serial.println(receivedID);

    Serial.print("Temperature : ");
    Serial.println(temperature);

    Serial.print("RSSI        : ");
    Serial.print(rssi);
    Serial.println(" dBm");

    Serial.print("SNR         : ");
    Serial.print(snr);
    Serial.println(" dB");


    // =================================================
    // VALIDATE NODE 3 RESPONSE
    // =================================================

    if (
      destination == NODE1_ID &&
      sender == NODE3_ID &&
      receivedID == masterMessageID
    ) {

      Serial.println("STATUS: VALID NODE 3 RESPONSE");


      // ===============================================
      // TWO-BLINK LED
      // ===============================================

      Serial.println();
      Serial.println("NODE 3 RESPONSE RECEIVED");
      Serial.println("LED: 2 BLINKS");

      blinkNode3();


      // ===============================================
      // SEND NODE 3 DATA TO MASTER
      // ===============================================

      Serial.println();
      Serial.println("Forwarding NODE 3 data to MASTER...");


      LoRa.idle();

      LoRa.beginPacket();

      // Destination = Master
      LoRa.write(MASTER_ID);

      // Sender = Node 3
      LoRa.write(NODE3_ID);

      // Same message ID
      LoRa.write(masterMessageID);

      // Temperature
      LoRa.print(temperature);

      LoRa.endPacket();


      // ===============================================
      // INFORMATION
      // ===============================================

      Serial.println();
      Serial.println("---------- RELAY RESPONSE ----------");

      Serial.print("Destination: 0x");
      Serial.println(MASTER_ID, HEX);

      Serial.print("Original Node: 0x");
      Serial.println(NODE3_ID, HEX);

      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" C");

      Serial.print("Message ID: ");
      Serial.println(masterMessageID);

      Serial.println("NODE 3 DATA FORWARDED TO MASTER");


      // ===============================================
      // DASHBOARD DATA
      // ===============================================

      Serial.print("DATA|NODE=");

      if (NODE3_ID < 16) {
        Serial.print("0");
      }

      Serial.print(NODE3_ID, HEX);

      Serial.print("|TEMP=");
      Serial.print(temperature);

      Serial.print("|RSSI=");
      Serial.print(rssi);

      Serial.print("|SNR=");
      Serial.print(snr);

      Serial.print("|MSG=");
      Serial.println(masterMessageID);


      // ===============================================
      // BACK TO RX
      // ===============================================

      LoRa.receive();

      return;
    }


    // =================================================
    // INVALID NODE 3 RESPONSE
    // =================================================

    Serial.println("INVALID NODE 3 RESPONSE");

    LoRa.receive();
  }


  // ===================================================
  // NODE 3 TIMEOUT
  // ===================================================

  Serial.println();
  Serial.println("******** NODE 3 TIMEOUT ********");

  Serial.println("Node 3 did not respond through relay.");

  Serial.println("STATUS: NODE 3 OFFLINE");

  Serial.println("********************************");


  // ===================================================
  // RETURN TO RX
  // ===================================================

  LoRa.receive();
}


// =====================================================
// LED: NODE 1 RESPONSE
// =====================================================
//
// One noticeable blink
//
// ON
// wait
// OFF
//

void blinkNode1() {

  digitalWrite(RESPONSE_LED, HIGH);

  delay(NODE1_BLINK_TIME);

  digitalWrite(RESPONSE_LED, LOW);

  delay(100);
}


// =====================================================
// LED: NODE 3 RELAY
// =====================================================
//
// Two noticeable blinks
//
// ON
// OFF
// ON
// OFF
//

void blinkNode3() {

  // -------------------------
  // BLINK 1
  // -------------------------

  digitalWrite(RESPONSE_LED, HIGH);

  delay(NODE3_BLINK_ON_TIME);

  digitalWrite(RESPONSE_LED, LOW);

  delay(NODE3_BLINK_OFF_TIME);


  // -------------------------
  // BLINK 2
  // -------------------------

  digitalWrite(RESPONSE_LED, HIGH);

  delay(NODE3_BLINK_ON_TIME);

  digitalWrite(RESPONSE_LED, LOW);

  delay(150);
}

