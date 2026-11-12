# MarsLink Mission Control — Sensor Forecasting

A responsive **Flask + PostgreSQL** dashboard for three ESP32/LoRa sensor nodes (`BB`, `CC`, `DD`), including live readings, charts, archive, network route, and an experimental supervised machine-learning forecast.

## New: AI Environmental Forecast

- Endpoint: **`GET /api/forecast`**. The dashboard loads it at startup, on demand, and every minute.
- Forecasts **temperature (°C)** and **dust concentration (µg/m³)** independently for each node, 10 minutes ahead.
- PostgreSQL aggregates valid telemetry into 1-minute measurements over the last 24 hours.
- `forecast.py` trains a regularized **Ridge regression** on preceding value, short-term changes, averages, and variability. Targets are the true measured changes **10 minutes later**.
- A *chronological validation set*, separated from training by a 10-minute gap, compares model MAE with a no-change baseline. If regression is worse, the app shows the clearly labeled **baseline fallback** rather than pretending the ML forecast is stronger.
- Data must be reasonably continuous (normally about **80–100 minutes**) and recent (no older than 3 minutes). Missing data is **never fabricated**. Dust predictions require actual dust sensor readings.
- Each forecast includes measured history, a dashed forecast on the chart, observed value, forecast time, trend, validation MAE, and the fallback explanation.
- Models are refitted using fresh saved readings; database query and prediction results are cached for **60 seconds per Gunicorn worker**.

**Important:** This is **short-term sensor extrapolation**, not a meteorological weather forecast or dust-storm warning. It does not know atmospheric pressure, wind, rain, or global Martian conditions. Validation MAE is an estimate from recent test data, not a guaranteed error bound. Predictions are experimental.

## Set up locally

1. Install Python 3.10+ and PostgreSQL.
2. Install project dependencies:

   ```bash
   python -m pip install -r requirements.txt
   ```

3. Set environment variables using your shell or hosting environment (copy the placeholders from `.env.example`):

   **PowerShell:**
   ```powershell
   $env:DATABASE_URL="postgresql://USERNAME:PASSWORD@HOST:5432/postgres"
   $env:INGEST_TOKEN="your-long-random-secret"
   python app.py
   ```

   The app creates `sensor_data` and necessary indexes if the database connection is available.

4. Open **http://127.0.0.1:5000**. Without incoming data, the live readings and forecasts correctly show empty/collecting states.

## Ingest real node readings

The serial collector (`collector.py`) listens for master ESP32 lines and forwards readings to the Flask app. Set the collector's `SERIAL_PORT`, `SERVER_URL`, and environment variable `MARSLINK_TOKEN` to match the server's `INGEST_TOKEN`. Run `python collector.py` on the machine connected to the master ESP32.

Example serial line:

```text
DATA|NODE=BB|TEMP=25.0|DUST=14.2|RSSI=-55|SNR=8.2|MSG=3
```

You can also POST JSON to `/api/ingest` with the `X-API-Key: <INGEST_TOKEN>` header:

```json
{"node_id": "BB", "temperature": 25.0, "dust": 14.2, "rssi": -55, "snr": 8.2, "message_id": "3"}
```

**Security:** Always set a strong `INGEST_TOKEN` in deployments and don't publish tokens in your repository. The dashboard's current GET endpoints are public; add authentication if you're hosting private telemetry.

## Deploy

`render.yaml` uses Gunicorn and installs all packages from `requirements.txt` (now including scikit-learn). Configure `DATABASE_URL` and `INGEST_TOKEN` as hosting environment secrets. The machine forwarding the ESP32 serial readings must remain connected and able to reach the deployed URL.

## Run forecast tests

```bash
python -m unittest discover -s tests -v
```

Tests use generated sample **test data only**; production never displays test data as actual sensor measurements.

## Files

- `app.py` — Flask routes and PostgreSQL queries, including `/api/forecast`.
- `forecast.py` — regression features, training, time-aware evaluation, and fallback.
- `static/script.js` — live dashboard updates, prediction cards, and Chart.js graphs.
- `static/style.css` — responsive mission-control styling.
- `templates/index.html` — dashboard layout.
- `collector.py` — forwards serial telemetry from master ESP32.
- `sketches/` — hardware firmware from the original archive.
