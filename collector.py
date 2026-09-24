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
SERVER_URL = "https://marslink-dashboard.onrender.com"

# Must match Render's INGEST_TOKEN
INGEST_TOKEN = os.getenv(
    "MARSLINK_TOKEN",
    "marslink-secret-2026"
)


# =========================================================
# SERIAL DATA PARSER
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


    # =====================================================
    # NODE ID
    # =====================================================

    node_id = data.get("NODE", "")

    node_id = (
        node_id
        .upper()
        .replace("0X", "")
    )


    # =====================================================
    # ACCEPT NODE 1, NODE 2 AND NODE 3
    # =====================================================

    if node_id not in ["BB", "CC", "DD"]:
        return None


    # =====================================================
    # NUMBER CONVERTER
    # =====================================================

    def number(key):

        try:

            return float(data[key])

        except (
            KeyError,
            ValueError
        ):

            return None


    # =====================================================
    # RETURN PARSED DATA
    # =====================================================

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

    print(
        "Serial:",
        SERIAL_PORT
    )

    print(
        "Server:",
        SERVER_URL
    )

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


                # =========================================
                # SHOW ESP32 SERIAL DATA
                # =========================================

                print(
                    "ESP32:",
                    line
                )


                # =========================================
                # PARSE DATA
                # =========================================

                parsed = parse_line(line)


                if parsed:

                    print(
                        "Parsed:",
                        json.dumps(parsed)
                    )


                    # =====================================
                    # SEND TO SERVER
                    # =====================================

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


# =========================================================
# START
# =========================================================

if __name__ == "__main__":

    main()