# MarsLink Local Dashboard

This dashboard reads telemetry from the MarsLink MASTER ESP32 through USB Serial
and displays the LoRa nodes in a browser.

## 1. Install Python packages

```bash
py -m pip install -r requirements.txt
```

or:

```bash
python -m pip install -r requirements.txt
```

## 2. Find the MASTER COM port

Windows Device Manager -> Ports (COM & LPT)

Then edit `app.py`:

```python
SERIAL_PORT = "COM5"
```

Replace COM5 with your actual port.

## 3. Important: make the MASTER print machine-readable telemetry

When the master receives a response, add this line after RSSI/SNR are known:

```cpp
Serial.print("DATA|NODE=");
Serial.print(sender, HEX);
Serial.print("|TEMP=");
Serial.print(payload);
Serial.print("|RSSI=");
Serial.print(LoRa.packetRssi());
Serial.print("|SNR=");
Serial.print(LoRa.packetSnr());
Serial.print("|MSG=");
Serial.println(receivedID);
```

For example, Node 1 should produce:

```text
DATA|NODE=BB|TEMP=25.0|RSSI=-55|SNR=8.2|MSG=3
```

The dashboard automatically parses this line.

## 4. Run

```bash
python app.py
```

Open:

http://127.0.0.1:5000

## Current features

- Master/gateway status
- Node 1 and Node 2 cards
- Online/offline detection
- Temperature
- RSSI
- SNR
- Message ID
- Last-seen time
- Temperature history chart
- Serial connection status
- Network event log

## Adding dust later

Send:

```text
DATA|NODE=BB|TEMP=25.0|DUST=14.2|RSSI=-55|SNR=8.2|MSG=3
```

The backend can be extended to parse DUST too.
