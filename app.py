from flask import Flask, jsonify, render_template, request
import os
import time
from datetime import datetime, timezone

import psycopg
from psycopg.rows import dict_row


app = Flask(__name__)

DATABASE_URL = os.getenv("DATABASE_URL")
INGEST_TOKEN = os.getenv("INGEST_TOKEN", "")

NODE_INFO = {
    "BB": {
        "name": "Node 1",
    },
    "CC": {
        "name": "Node 2",
    },
}


# ---------------------------------------------------------
# DATABASE
# ---------------------------------------------------------

def get_db():
    if not DATABASE_URL:
        raise RuntimeError("DATABASE_URL is not configured")

    return psycopg.connect(
        DATABASE_URL,
        row_factory=dict_row
    )


def init_db():
    with get_db() as conn:
        with conn.cursor() as cur:
            cur.execute("""
                CREATE TABLE IF NOT EXISTS sensor_data (
                    id BIGSERIAL PRIMARY KEY,
                    node_id VARCHAR(20) NOT NULL,
                    timestamp TIMESTAMPTZ NOT NULL DEFAULT NOW(),
                    temperature DOUBLE PRECISION,
                    dust DOUBLE PRECISION,
                    rssi DOUBLE PRECISION,
                    snr DOUBLE PRECISION,
                    message_id VARCHAR(100)
                );
            """)

            cur.execute("""
                CREATE INDEX IF NOT EXISTS idx_sensor_data_timestamp
                ON sensor_data(timestamp);
            """)

            cur.execute("""
                CREATE INDEX IF NOT EXISTS idx_sensor_data_node_timestamp
                ON sensor_data(node_id, timestamp);
            """)

        conn.commit()


# ---------------------------------------------------------
# HELPERS
# ---------------------------------------------------------

def parse_float(value):
    if value is None:
        return None

    try:
        return float(value)
    except (ValueError, TypeError):
        return None


def valid_node(node_id):
    return node_id in NODE_INFO


def verify_token(req):
    if not INGEST_TOKEN:
        return True

    supplied = req.headers.get("X-API-Key", "")

    return supplied == INGEST_TOKEN


# ---------------------------------------------------------
# ROUTES
# ---------------------------------------------------------

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/health")
def health():
    try:
        with get_db() as conn:
            with conn.cursor() as cur:
                cur.execute("SELECT 1")
                cur.fetchone()

        return jsonify({
            "status": "ok",
            "database": "connected"
        })

    except Exception as e:
        return jsonify({
            "status": "error",
            "database": "disconnected",
            "error": str(e)
        }), 500


# ---------------------------------------------------------
# TELEMETRY INGESTION
# ---------------------------------------------------------

@app.route("/api/ingest", methods=["POST"])
def ingest():
    if not verify_token(request):
        return jsonify({
            "success": False,
            "error": "Unauthorized"
        }), 401

    data = request.get_json(silent=True)

    if not data:
        return jsonify({
            "success": False,
            "error": "Invalid JSON"
        }), 400

    node_id = str(data.get("node_id", "")).upper().replace("0X", "")

    if not valid_node(node_id):
        return jsonify({
            "success": False,
            "error": f"Unknown node: {node_id}"
        }), 400

    temperature = parse_float(data.get("temperature"))
    dust = parse_float(data.get("dust"))
    rssi = parse_float(data.get("rssi"))
    snr = parse_float(data.get("snr"))
    message_id = data.get("message_id")

    try:
        with get_db() as conn:
            with conn.cursor() as cur:
                cur.execute("""
                    INSERT INTO sensor_data
                    (
                        node_id,
                        temperature,
                        dust,
                        rssi,
                        snr,
                        message_id
                    )
                    VALUES (%s, %s, %s, %s, %s, %s)
                """, (
                    node_id,
                    temperature,
                    dust,
                    rssi,
                    snr,
                    str(message_id) if message_id is not None else None
                ))

            conn.commit()

        return jsonify({
            "success": True
        })

    except Exception as e:
        return jsonify({
            "success": False,
            "error": str(e)
        }), 500


# ---------------------------------------------------------
# CURRENT DASHBOARD DATA
# ---------------------------------------------------------

@app.route("/api/data")
def api_data():

    now = datetime.now(timezone.utc)

    nodes = {}

    try:
        with get_db() as conn:
            with conn.cursor() as cur:

                for node_id, info in NODE_INFO.items():

                    cur.execute("""
                        SELECT
                            node_id,
                            timestamp,
                            temperature,
                            dust,
                            rssi,
                            snr,
                            message_id
                        FROM sensor_data
                        WHERE node_id = %s
                        ORDER BY timestamp DESC
                        LIMIT 60
                    """, (node_id,))

                    rows = cur.fetchall()

                    rows.reverse()

                    latest = rows[-1] if rows else None

                    if latest:
                        age = (
                            now - latest["timestamp"]
                        ).total_seconds()

                        status = "Online" if age <= 20 else "Offline"

                        latest_data = {
                            "temperature": latest["temperature"],
                            "dust": latest["dust"],
                            "rssi": latest["rssi"],
                            "snr": latest["snr"],
                            "message_id": latest["message_id"],
                            "last_seen": round(age, 1)
                        }

                    else:
                        status = "Offline"

                        latest_data = {
                            "temperature": None,
                            "dust": None,
                            "rssi": None,
                            "snr": None,
                            "message_id": None,
                            "last_seen": None
                        }

                    history = []

                    for row in rows:
                        history.append({
                            "time": row["timestamp"].strftime("%H:%M:%S"),
                            "temperature": row["temperature"],
                            "dust": row["dust"],
                            "rssi": row["rssi"],
                            "snr": row["snr"]
                        })

                    nodes[node_id] = {
                        "id": node_id,
                        "name": info["name"],
                        "status": status,
                        **latest_data,
                        "history": history
                    }

        online = sum(
            1 for n in nodes.values()
            if n["status"] == "Online"
        )

        return jsonify({
            "time": datetime.now().strftime(
                "%Y-%m-%d %H:%M:%S"
            ),
            "online_nodes": online,
            "total_nodes": len(nodes),
            "nodes": nodes
        })

    except Exception as e:

        return jsonify({
            "error": str(e)
        }), 500


# ---------------------------------------------------------
# HISTORY
# ---------------------------------------------------------

@app.route("/api/history")
def api_history():

    date_string = request.args.get("date")

    if not date_string:
        date_string = datetime.now().strftime("%Y-%m-%d")

    try:
        selected_date = datetime.strptime(
            date_string,
            "%Y-%m-%d"
        ).date()

    except ValueError:
        return jsonify({
            "error": "Date must be YYYY-MM-DD"
        }), 400

    start = datetime(
        selected_date.year,
        selected_date.month,
        selected_date.day,
        tzinfo=timezone.utc
    )

    end = start.replace(
        day=start.day
    )

    from datetime import timedelta

    end = start + timedelta(days=1)

    result = {}

    try:
        with get_db() as conn:
            with conn.cursor() as cur:

                for node_id, info in NODE_INFO.items():

                    cur.execute("""
                        SELECT
                            id,
                            node_id,
                            timestamp,
                            temperature,
                            dust,
                            rssi,
                            snr,
                            message_id
                        FROM sensor_data
                        WHERE node_id = %s
                          AND timestamp >= %s
                          AND timestamp < %s
                        ORDER BY timestamp ASC
                    """, (
                        node_id,
                        start,
                        end
                    ))

                    rows = cur.fetchall()

                    readings = []

                    temperatures = []
                    dust_values = []

                    for row in rows:

                        if row["temperature"] is not None:
                            temperatures.append(
                                row["temperature"]
                            )

                        if row["dust"] is not None:
                            dust_values.append(
                                row["dust"]
                            )

                        readings.append({
                            "id": row["id"],
                            "timestamp": row["timestamp"].isoformat(),
                            "time": row["timestamp"].strftime(
                                "%H:%M:%S"
                            ),
                            "temperature": row["temperature"],
                            "dust": row["dust"],
                            "rssi": row["rssi"],
                            "snr": row["snr"],
                            "message_id": row["message_id"]
                        })

                    def average(values):
                        if not values:
                            return None
                        return round(
                            sum(values) / len(values),
                            2
                        )

                    result[node_id] = {
                        "name": info["name"],
                        "readings": readings,
                        "count": len(readings),

                        "temperature": {
                            "min": min(temperatures)
                            if temperatures else None,

                            "max": max(temperatures)
                            if temperatures else None,

                            "average": average(
                                temperatures
                            )
                        },

                        "dust": {
                            "min": min(dust_values)
                            if dust_values else None,

                            "max": max(dust_values)
                            if dust_values else None,

                            "average": average(
                                dust_values
                            )
                        }
                    }

        return jsonify({
            "date": date_string,
            "nodes": result
        })

    except Exception as e:

        return jsonify({
            "error": str(e)
        }), 500


# ---------------------------------------------------------
# AVAILABLE DATES
# ---------------------------------------------------------

@app.route("/api/dates")
def api_dates():

    try:

        with get_db() as conn:
            with conn.cursor() as cur:

                cur.execute("""
                    SELECT DISTINCT
                        DATE(timestamp) AS day
                    FROM sensor_data
                    ORDER BY day DESC
                """)

                rows = cur.fetchall()

        dates = [
            row["day"].isoformat()
            for row in rows
        ]

        return jsonify({
            "dates": dates
        })

    except Exception as e:

        return jsonify({
            "error": str(e)
        }), 500


# ---------------------------------------------------------
# DATABASE STARTUP
# ---------------------------------------------------------

try:
    init_db()
except Exception as e:
    print("Database initialization warning:", e)


# ---------------------------------------------------------
# LOCAL DEVELOPMENT
# ---------------------------------------------------------

if __name__ == "__main__":

    app.run(
        host="0.0.0.0",
        port=int(os.getenv("PORT", 5000)),
        debug=False
    )