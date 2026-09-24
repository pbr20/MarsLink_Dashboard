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

#define NODE3_ID  0xDD
#define MASTER_ID 0xFF


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("================================");
  Serial.println("            NODE 3");
  Serial.println("================================");


  // ===================================================
  // LED
  // ===================================================

  pinMode(RESPONSE_LED, OUTPUT);

  // LED OFF initially
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
  // LORA
  // ===================================================

  LoRa.setPins(
    LORA_SS,
    LORA_RST,
    LORA_DIO0
  );


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

  Serial.println("Node ID    : 0xDD");
  Serial.println("Frequency  : 433 MHz");
  Serial.println("SF         : 7");
  Serial.println("BW         : 62.5 kHz");
  Serial.println("CR         : 4/5");
  Serial.println("SyncWord   : 0x12");
  Serial.println("CRC        : ON");

  Serial.println();
  Serial.println("DHT11 READY");
  Serial.println("LED PIN    : GPIO 16");
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
  Serial.println("       REQUEST RECEIVED");
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
  // CHECK DESTINATION
  // ===================================================

  if (destination != NODE3_ID) {

    Serial.println("Not for Node 3");

    LoRa.receive();

    return;
  }


  // ===================================================
  // CHECK MASTER
  // ===================================================

  if (sender != MASTER_ID) {

    Serial.println("Unknown sender");

    LoRa.receive();

    return;
  }


  // ===================================================
  // COMMAND 10 = TEMPERATURE
  // ===================================================

  if (command == "10") {

    Serial.println();
    Serial.println("MASTER REQUESTED TEMPERATURE");


    // =================================================
    // READ DHT11
    // =================================================

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


    // =================================================
    // LED ON
    // =================================================

    digitalWrite(RESPONSE_LED, HIGH);

    Serial.println("LED GPIO 16: ON");


    // =================================================
    // SEND RESPONSE
    // =================================================

    LoRa.idle();

    LoRa.beginPacket();

    // Destination = Master
    LoRa.write(MASTER_ID);

    // Sender = Node 3
    LoRa.write(NODE3_ID);

    // Same message ID
    LoRa.write(receivedID);

    // Temperature
    LoRa.print(data);

    LoRa.endPacket();


    // =================================================
    // INFORMATION
    // =================================================

    Serial.println();
    Serial.println("---------- RESPONSE ----------");

    Serial.print("TX Temperature: ");
    Serial.print(data);
    Serial.println(" C");

    Serial.print("Message ID: ");
    Serial.println(receivedID);

    Serial.println("Response sent");


    // =================================================
    // LED OFF
    // =================================================

    delay(300);

    digitalWrite(RESPONSE_LED, LOW);

    Serial.println("LED GPIO 16: OFF");


    // =================================================
    // BACK TO RX
    // =================================================

    LoRa.receive();
  }
}