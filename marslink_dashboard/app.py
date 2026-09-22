from flask import Flask, jsonify, render_template, request
import serial
import threading
import time
import sqlite3
import os
from collections import deque

app = Flask(__name__)

# =========================================================
# CONFIGURATION
# =========================================================

SERIAL_PORT = "COM9"
BAUD_RATE = 115200

DATABASE = "marslink.db"

# How long until a node is considered offline
NODE_TIMEOUT = 20

lock = threading.Lock()

# =========================================================
# NODE CONFIGURATION
# =========================================================

nodes = {
    "BB": {
        "name": "Node 1",
        "temperature": None,
        "dust": None,
        "status": "Offline",
        "last_seen": None,
        "rssi": None,
        "snr": None,
        "message_id": None,
        "history": deque(maxlen=60),
    },

    "CC": {
        "name": "Node 2",
        "temperature": None,
        "dust": None,
        "status": "Offline",
        "last_seen": None,
        "rssi": None,
        "snr": None,
        "message_id": None,
        "history": deque(maxlen=60),
    },
}

events = deque(maxlen=100)

serial_status = {
    "connected": False,
    "port": SERIAL_PORT,
    "error": None
}


# =========================================================
# DATABASE
# =========================================================

def get_db():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn


def init_database():

    conn = get_db()

    conn.execute("""
        CREATE TABLE IF NOT EXISTS readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            date TEXT NOT NULL,
            node_id TEXT NOT NULL,
            temperature REAL,
            dust REAL,
            rssi REAL,
            snr REAL,
            message_id TEXT
        )
    """)

    conn.execute("""
        CREATE INDEX IF NOT EXISTS idx_readings_date
        ON readings(date)
    """)

    conn.execute("""
        CREATE INDEX IF NOT EXISTS idx_readings_node_date
        ON readings(node_id, date)
    """)

    conn.execute("""
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            date TEXT NOT NULL,
            message TEXT NOT NULL
        )
    """)

    conn.commit()
    conn.close()


def save_reading(node_id, temperature, dust, rssi, snr, message_id):

    now = time.localtime()

    timestamp = time.strftime(
        "%Y-%m-%d %H:%M:%S",
        now
    )

    date = time.strftime(
        "%Y-%m-%d",
        now
    )

    try:
        conn = get_db()

        conn.execute("""
            INSERT INTO readings
            (
                timestamp,
                date,
                node_id,
                temperature,
                dust,
                rssi,
                snr,
                message_id
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            timestamp,
            date,
            node_id,
            temperature,
            dust,
            rssi,
            snr,
            message_id
        ))

        conn.commit()
        conn.close()

    except Exception as e:
        print("Database error:", e)


def save_event(message):

    now = time.localtime()

    timestamp = time.strftime(
        "%Y-%m-%d %H:%M:%S",
        now
    )

    date = time.strftime(
        "%Y-%m-%d",
        now
    )

    try:
        conn = get_db()

        conn.execute("""
            INSERT INTO events
            (
                timestamp,
                date,
                message
            )
            VALUES (?, ?, ?)
        """, (
            timestamp,
            date,
            message
        ))

        conn.commit()
        conn.close()

    except Exception as e:
        print("Event database error:", e)


# =========================================================
# EVENTS
# =========================================================

def add_event(message):

    timestamp = time.strftime(
        "%H:%M:%S"
    )

    with lock:

        events.appendleft({
            "time": timestamp,
            "message": message
        })

    save_event(message)


# =========================================================
# SERIAL DATA PARSER
# =========================================================

def parse_structured_line(line):

    # Expected:
    #
    # DATA|NODE=BB|TEMP=24.0|DUST=15|RSSI=-55|SNR=8.5|MSG=3

    if not line.startswith("DATA|"):
        return

    parts = line.strip().split("|")

    data = {}

    for part in parts[1:]:

        if "=" in part:

            key, value = part.split("=", 1)

            data[key] = value

    node_id = data.get("NODE")

    if node_id not in nodes:
        return

    # -----------------------------------------------------
    # TEMPERATURE
    # -----------------------------------------------------

    try:
        temperature = float(
            data["TEMP"]
        )

    except (KeyError, ValueError):
        temperature = None

    # -----------------------------------------------------
    # DUST
    # -----------------------------------------------------

    try:
        dust = float(
            data["DUST"]
        )

    except (KeyError, ValueError):
        dust = None

    # -----------------------------------------------------
    # RSSI
    # -----------------------------------------------------

    try:
        rssi = float(
            data["RSSI"]
        )

    except (KeyError, ValueError):
        rssi = None

    # -----------------------------------------------------
    # SNR
    # -----------------------------------------------------

    try:
        snr = float(
            data["SNR"]
        )

    except (KeyError, ValueError):
        snr = None

    message_id = data.get("MSG")

    now = time.time()

    # -----------------------------------------------------
    # UPDATE LIVE DATA
    # -----------------------------------------------------

    with lock:

        n = nodes[node_id]

        n["temperature"] = temperature

        n["dust"] = dust

        n["rssi"] = rssi

        n["snr"] = snr

        n["message_id"] = message_id

        n["last_seen"] = now

        n["status"] = "Online"

        n["history"].append({

            "time": time.strftime("%H:%M:%S"),

            "temperature": temperature,

            "dust": dust,

            "rssi": rssi,

            "snr": snr
        })

    # -----------------------------------------------------
    # SAVE TO DATABASE
    # -----------------------------------------------------

    save_reading(
        node_id,
        temperature,
        dust,
        rssi,
        snr,
        message_id
    )


# =========================================================
# SERIAL WORKER
# =========================================================

def serial_worker():

    while True:

        try:

            print(
                f"Connecting to {SERIAL_PORT}..."
            )

            ser = serial.Serial(
                SERIAL_PORT,
                BAUD_RATE,
                timeout=1
            )

            serial_status["connected"] = True
            serial_status["error"] = None

            add_event(
                f"Serial connected: {SERIAL_PORT}"
            )

            print(
                f"Serial connected: {SERIAL_PORT}"
            )

            while True:

                raw = ser.readline()

                if not raw:
                    continue

                line = raw.decode(
                    "utf-8",
                    errors="replace"
                ).strip()

                if line:

                    print(line)

                    parse_structured_line(line)

        except Exception as e:

            serial_status["connected"] = False

            serial_status["error"] = str(e)

            add_event(
                f"Serial error: {e}"
            )

            print(
                "Serial error:",
                e
            )

            time.sleep(3)


# =========================================================
# NODE STATUS WORKER
# =========================================================

def status_worker():

    while True:

        now = time.time()

        with lock:

            for node_id, node in nodes.items():

                if node["last_seen"] is None:

                    node["status"] = "Offline"

                elif (
                    now - node["last_seen"]
                    > NODE_TIMEOUT
                ):

                    if node["status"] != "Offline":

                        add_event(
                            f"{node['name']} went offline"
                        )

                    node["status"] = "Offline"

        time.sleep(2)


# =========================================================
# MAIN DASHBOARD
# =========================================================

@app.route("/")
def index():

    return render_template(
        "index.html"
    )


# =========================================================
# LIVE API
# =========================================================

@app.route("/api/data")
def api_data():

    now = time.time()

    with lock:

        result_nodes = {}

        for node_id, node in nodes.items():

            item = {

                "id": node_id,

                "name": node["name"],

                "temperature": node["temperature"],

                "dust": node["dust"],

                "status": node["status"],

                "last_seen": (
                    round(
                        now - node["last_seen"],
                        1
                    )
                    if node["last_seen"]
                    else None
                ),

                "rssi": node["rssi"],

                "snr": node["snr"],

                "message_id": node["message_id"],

                "history": list(
                    node["history"]
                )
            }

            result_nodes[node_id] = item

        return jsonify({

            "time": time.strftime(
                "%Y-%m-%d %H:%M:%S"
            ),

            "serial": serial_status,

            "nodes": result_nodes,

            "events": list(events)
        })


# =========================================================
# AVAILABLE DATES
# =========================================================

@app.route("/api/dates")
def api_dates():

    conn = get_db()

    rows = conn.execute("""
        SELECT DISTINCT date
        FROM readings
        ORDER BY date DESC
    """).fetchall()

    conn.close()

    dates = [
        row["date"]
        for row in rows
    ]

    return jsonify({
        "dates": dates
    })


# =========================================================
# HISTORICAL DATA
# =========================================================

@app.route("/api/history")
def api_history():

    selected_date = request.args.get(
        "date"
    )

    if not selected_date:

        selected_date = time.strftime(
            "%Y-%m-%d"
        )

    conn = get_db()

    rows = conn.execute("""
        SELECT
            timestamp,
            node_id,
            temperature,
            dust,
            rssi,
            snr,
            message_id
        FROM readings
        WHERE date = ?
        ORDER BY timestamp ASC
    """, (
        selected_date,
    )).fetchall()

    conn.close()

    readings = []

    for row in rows:

        readings.append({

            "time": row["timestamp"],

            "node": row["node_id"],

            "temperature": row["temperature"],

            "dust": row["dust"],

            "rssi": row["rssi"],

            "snr": row["snr"],

            "message_id": row["message_id"]
        })

    return jsonify({

        "date": selected_date,

        "readings": readings
    })


# =========================================================
# DATE SUMMARY
# =========================================================

@app.route("/api/summary")
def api_summary():

    selected_date = request.args.get(
        "date"
    )

    if not selected_date:

        selected_date = time.strftime(
            "%Y-%m-%d"
        )

    conn = get_db()

    rows = conn.execute("""
        SELECT
            node_id,
            COUNT(*) AS samples,
            AVG(temperature) AS avg_temp,
            MIN(temperature) AS min_temp,
            MAX(temperature) AS max_temp,
            AVG(dust) AS avg_dust
        FROM readings
        WHERE date = ?
        GROUP BY node_id
    """, (
        selected_date,
    )).fetchall()

    conn.close()

    summary = {}

    for row in rows:

        summary[row["node_id"]] = {

            "samples": row["samples"],

            "avg_temp": row["avg_temp"],

            "min_temp": row["min_temp"],

            "max_temp": row["max_temp"],

            "avg_dust": row["avg_dust"]
        }

    return jsonify({

        "date": selected_date,

        "summary": summary
    })


# =========================================================
# START
# =========================================================

if __name__ == "__main__":

    init_database()

    threading.Thread(
        target=serial_worker,
        daemon=True
    ).start()

    threading.Thread(
        target=status_worker,
        daemon=True
    ).start()

    print()
    print("================================")
    print("        MarsLink Dashboard")
    print("================================")
    print()
    print(
        "Open http://127.0.0.1:5000"
    )
    print()
    print(
        "Database:",
        os.path.abspath(DATABASE)
    )
    print()

    app.run(
        host="0.0.0.0",
        port=5000,
        debug=False
    )