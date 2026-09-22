import os
import time
import json
import serial
import requests


# =========================================================
# CONFIGURATION
# =========================================================

SERIAL_PORT = "COM9"
BAUD_RATE = 115200

# Put your Render URL here after deployment
SERVER_URL = os.getenv(
    "MARSLINK_SERVER",
    "MARSLINK_SERVER=https://marslink-dashboard.onrender.com"
)

# Must match Render's INGEST_TOKEN
INGEST_TOKEN = os.getenv(
    "MARSLINK_TOKEN",
    "marslink-secret-2026"
)


# =========================================================
# SERIAL
# =========================================================

def parse_line(line):

    if not line.startswith("DATA|"):
        return None

    parts = line.strip().split("|")

    data = {}

    for part in parts[1:]:

        if "=" in part:

            key, value = part.split(
                "=",
                1
            )

            data[key.upper()] = value

    node_id = data.get("NODE", "")

    node_id = (
        node_id
        .upper()
        .replace("0X", "")
    )

    if node_id not in ["BB", "CC"]:
        return None

    def number(key):

        try:
            return float(data[key])
        except (
            KeyError,
            ValueError
        ):
            return None

    return {
        "node_id": node_id,

        "temperature": number("TEMP"),

        "dust": number("DUST"),

        "rssi": number("RSSI"),

        "snr": number("SNR"),

        "message_id": data.get("MSG")
    }


# =========================================================
# SEND TO CLOUD
# =========================================================

def send_to_server(data):

    url = SERVER_URL.rstrip("/") + "/api/ingest"

    headers = {
        "Content-Type": "application/json"
    }

    if INGEST_TOKEN:
        headers["X-API-Key"] = INGEST_TOKEN

    try:

        response = requests.post(
            url,
            headers=headers,
            json=data,
            timeout=10
        )

        if response.ok:

            print(
                "Uploaded:",
                json.dumps(data)
            )

            return True

        print(
            "Server error:",
            response.status_code,
            response.text
        )

    except requests.RequestException as e:

        print(
            "Upload failed:",
            e
        )

    return False


# =========================================================
# MAIN LOOP
# =========================================================

def main():

    print()
    print("===================================")
    print("        MarsLink Collector")
    print("===================================")
    print()
    print("Serial:", SERIAL_PORT)
    print("Server:", SERVER_URL)
    print()

    while True:

        ser = None

        try:

            print(
                f"Connecting to {SERIAL_PORT}..."
            )

            ser = serial.Serial(
                SERIAL_PORT,
                BAUD_RATE,
                timeout=1
            )

            print(
                "Serial connected!"
            )

            while True:

                raw = ser.readline()

                if not raw:
                    continue

                line = raw.decode(
                    "utf-8",
                    errors="replace"
                ).strip()

                if not line:
                    continue

                print(
                    "ESP32:",
                    line
                )

                parsed = parse_line(line)

                if parsed:

                    send_to_server(
                        parsed
                    )

        except serial.SerialException as e:

            print(
                "Serial error:",
                e
            )

        except KeyboardInterrupt:

            print(
                "\nCollector stopped."
            )

            break

        except Exception as e:

            print(
                "Unexpected error:",
                e
            )

        finally:

            if ser:

                try:
                    ser.close()
                except Exception:
                    pass

        print(
            "Retrying in 5 seconds..."
        )

        time.sleep(5)


if __name__ == "__main__":
    main()