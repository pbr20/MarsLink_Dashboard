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
// DEVICE IDs
// =====================================================

#define MASTER_ID  0xFF
#define NODE1_ID   0xBB
#define NODE3_ID   0xDD


// =====================================================
// DHT11
// =====================================================

#define DHTPIN 26
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


// =====================================================
// RESPONSE LED
// =====================================================

#define RESPONSE_LED 16


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);
  delay(2000);

  // ---------------------------------------------------
  // LED
  // ---------------------------------------------------

  pinMode(RESPONSE_LED, OUTPUT);
  digitalWrite(RESPONSE_LED, LOW);

  // ---------------------------------------------------
  // DHT
  // ---------------------------------------------------

  dht.begin();

  // ---------------------------------------------------
  // LoRa
  // ---------------------------------------------------

  SPI.begin(
    LORA_SCK,
    LORA_MISO,
    LORA_MOSI,
    LORA_SS
  );

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );

  Serial.println();
  Serial.println("================================");
  Serial.println("       MARS LINK NODE 3");
  Serial.println("================================");

  if (!LoRa.begin(433E6)) {

    Serial.println("LoRa initialization FAILED!");

    while (1);
  }

  // ---------------------------------------------------
  // LoRa configuration
  // ---------------------------------------------------

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  LoRa.setTxPower(17);

  Serial.println("LoRa initialized successfully.");
  Serial.println();
  Serial.println("NODE 3 MODE:");
  Serial.println("Only accepts requests from NODE 1");
  Serial.println("Master requests are IGNORED");
  Serial.println();

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
  // READ PACKET HEADER
  // ===================================================

  byte destination = LoRa.read();
  byte sender      = LoRa.read();
  byte receivedID  = LoRa.read();


  // ===================================================
  // READ COMMAND
  // ===================================================

  String command = "";

  while (LoRa.available()) {

    command += (char)LoRa.read();
  }


  // ===================================================
  // SIGNAL INFORMATION
  // ===================================================

  int rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();


  // ===================================================
  // PRINT REQUEST
  // ===================================================

  Serial.println();
  Serial.println("================================");
  Serial.println("       REQUEST RECEIVED");
  Serial.println("================================");

  Serial.print("Destination : 0x");

  if (destination < 16)
    Serial.print("0");

  Serial.println(destination, HEX);


  Serial.print("Sender      : 0x");

  if (sender < 16)
    Serial.print("0");

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
  // CHECK DESTINATION
  // ===================================================

  if (destination != NODE3_ID) {

    Serial.println();
    Serial.println("Ignored: Not for Node 3");

    LoRa.receive();
    return;
  }


  // ===================================================
  // IMPORTANT:
  // NODE 3 ONLY ACCEPTS NODE 1
  // ===================================================

  if (sender != NODE1_ID) {

    Serial.println();
    Serial.println("Ignored: Request is NOT from Node 1");

    if (sender == MASTER_ID) {

      Serial.println("Master request detected.");
      Serial.println("Node 3 does NOT communicate directly");
      Serial.println("with Master.");
      Serial.println("Waiting for Node 1 relay request.");

    }

    LoRa.receive();
    return;
  }


  // ===================================================
  // NODE 1 REQUEST ACCEPTED
  // ===================================================

  Serial.println();
  Serial.println("REQUEST FROM NODE 1 ACCEPTED");


  // ===================================================
  // CHECK COMMAND
  // ===================================================

  if (command != "10") {

    Serial.println("Unknown command.");
    LoRa.receive();
    return;
  }


  // ===================================================
  // READ TEMPERATURE
  // ===================================================

  Serial.println();
  Serial.println("NODE 1 REQUESTED TEMPERATURE");

  float temperature = dht.readTemperature();


  if (isnan(temperature)) {

    Serial.println("ERROR: DHT11 reading failed.");

    LoRa.receive();
    return;
  }


  String data = String(temperature, 1);


  Serial.print("Temperature: ");
  Serial.print(data);
  Serial.println(" C");


  // ===================================================
  // LED ON
  // ===================================================

  digitalWrite(RESPONSE_LED, HIGH);

  Serial.println("LED GPIO 16: ON");


  // ===================================================
  // SEND RESPONSE TO NODE 1
  // ===================================================

  Serial.println();
  Serial.println("---------- RESPONSE ----------");

  Serial.print("Destination: 0x");

  if (NODE1_ID < 16)
    Serial.print("0");

  Serial.println(NODE1_ID, HEX);


  Serial.print("Sender: 0x");

  if (NODE3_ID < 16)
    Serial.print("0");

  Serial.println(NODE3_ID, HEX);


  Serial.print("TX Temperature: ");
  Serial.print(data);
  Serial.println(" C");


  Serial.print("Message ID: ");
  Serial.println(receivedID);


  // ---------------------------------------------------
  // Send packet
  // ---------------------------------------------------

  LoRa.idle();

  LoRa.beginPacket();

  // Destination = NODE 1
  LoRa.write(NODE1_ID);

  // Sender = NODE 3
  LoRa.write(NODE3_ID);

  // Keep original Master message ID
  LoRa.write(receivedID);

  // Temperature
  LoRa.print(data);

  LoRa.endPacket();


  Serial.println("Response sent");


  // ===================================================
  // LED OFF
  // ===================================================

  delay(300);

  digitalWrite(RESPONSE_LED, LOW);

  Serial.println("LED GPIO 16: OFF");


  // ===================================================
  // RETURN TO RECEIVE MODE
  // ===================================================

  LoRa.receive();
}