"""Short-horizon supervised forecasts for MarsLink environmental sensors.

Every forecast is learned from REAL stored, one-minute sensor measurements.
No weather API or synthetic data are used in production. Readings are treated
as observations at their timestamp; missing minutes are not silently filled.
"""
from datetime import datetime, timedelta, timezone
import math

import numpy as np
from sklearn.linear_model import Ridge
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler

HORIZON_MINUTES = 10
LOOKBACK_MINUTES = 10
MIN_TRAIN = 35
MIN_VALIDATE = 12
MAX_READING_AGE_SECONDS = 180

METRICS = {
    "temperature": {"unit": "°C", "min": -150, "max": 150, "trend_delta": 0.3},
    "dust": {"unit": "µg/m³", "min": 0, "max": 50000, "trend_delta": 2.0},
}


def _unavailable(message, status, count=0):
    return {
        "status": status,
        "message": message,
        "horizon_minutes": HORIZON_MINUTES,
        "observed_minutes": count,
    }


def _features(lookup, minute):
    """Use only measurements known at `minute` (never future features)."""
    window = [lookup.get(minute - i) for i in range(LOOKBACK_MINUTES, -1, -1)]
    if any(value is None for value in window):
        return None
    current = window[-1]
    recent5 = window[-5:]
    return [
        current,
        current - window[-2],
        current - window[-6],
        current - window[0],
        current - float(np.mean(recent5)),
        current - float(np.mean(window)),
        float(np.std(window)),
        max(window) - min(window),
    ]


def _model():
    # Regularized supervised regression: learn 10-minute *changes*, not dates.
    return make_pipeline(StandardScaler(), Ridge(alpha=12.0))


def forecast_metric(rows, metric, now=None):
    """Forecast metric 10 minutes ahead from ({bucket, metric...}) rows.

    `bucket` must be a timezone-aware datetime rounded to the minute. Rows can
    be unordered. A chronological hold-out (with a 10-minute purge gap)
    evaluates the model against the 'no change' baseline.
    """
    if metric not in METRICS:
        raise ValueError("Unknown forecast metric")
    cfg = METRICS[metric]
    now = now or datetime.now(timezone.utc)
    if now.tzinfo is None:
        raise ValueError("now must be timezone-aware")

    lookup = {}
    for row in rows:
        stamp = row.get("bucket")
        raw = row.get(metric)
        if not isinstance(stamp, datetime) or stamp.tzinfo is None or raw is None:
            continue
        try:
            number = float(raw)
        except (ValueError, TypeError, OverflowError):
            continue
        if not math.isfinite(number) or not cfg["min"] <= number <= cfg["max"]:
            continue
        lookup[int(stamp.timestamp() // 60)] = number

    if not lookup:
        return _unavailable(f"No valid {metric} measurements yet.", "collecting")

    last_minute = max(lookup)
    last_time = datetime.fromtimestamp(last_minute * 60, timezone.utc)
    age = (now - last_time).total_seconds()
    if age > MAX_READING_AGE_SECONDS or age < -60:
        return _unavailable("Latest sensor reading is too old for a live forecast.", "stale", len(lookup))

    latest_features = _features(lookup, last_minute)
    if latest_features is None:
        return _unavailable("Need 11 consecutive minutes of recent readings before forecasting.", "collecting", len(lookup))

    # Only use complete forward and backward windows. No gap-filling or leakage.
    samples = []
    for minute in range(min(lookup) + LOOKBACK_MINUTES, last_minute - HORIZON_MINUTES + 1):
        features = _features(lookup, minute)
        if features is None or any((minute + j) not in lookup for j in range(1, HORIZON_MINUTES + 1)):
            continue
        current = lookup[minute]
        target_change = lookup[minute + HORIZON_MINUTES] - current
        samples.append((minute, features, target_change))

    if len(samples) < MIN_TRAIN + MIN_VALIDATE + HORIZON_MINUTES:
        return _unavailable(
            "Collect roughly 80–100 minutes of uninterrupted measurements for training and validation.",
            "collecting", len(lookup)
        )

    split = int(len(samples) * 0.8)
    # Purge samples whose 10-minute future label overlaps validation period.
    train = samples[:split - HORIZON_MINUTES]
    validation = samples[split:]
    if len(train) < MIN_TRAIN or len(validation) < MIN_VALIDATE:
        return _unavailable("More continuous readings are needed for a separate validation period.", "collecting", len(lookup))

    x_train = np.asarray([sample[1] for sample in train], dtype=float)
    y_train = np.asarray([sample[2] for sample in train], dtype=float)
    x_val = np.asarray([sample[1] for sample in validation], dtype=float)
    y_val = np.asarray([sample[2] for sample in validation], dtype=float)

    evaluation_model = _model()
    evaluation_model.fit(x_train, y_train)
    val_pred = evaluation_model.predict(x_val)
    model_mae = float(np.mean(np.abs(val_pred - y_val)))
    baseline_mae = float(np.mean(np.abs(y_val)))  # predicts zero change

    # Don't advertise a regression prediction when it trails the simpler baseline.
    model_outperforms = model_mae < baseline_mae
    if model_outperforms:
        final_model = _model()
        final_model.fit(
            np.asarray([sample[1] for sample in samples], dtype=float),
            np.asarray([sample[2] for sample in samples], dtype=float),
        )
        change = float(final_model.predict(np.asarray([latest_features]))[0])
        method = "ridge_regression"
    else:
        change = 0.0
        method = "persistence_baseline"

    forecast_value = min(cfg["max"], max(cfg["min"], lookup[last_minute] + change))
    current = lookup[last_minute]
    difference = forecast_value - current
    threshold = max(cfg["trend_delta"], baseline_mae)
    trend = "Rising" if difference > threshold else "Falling" if difference < -threshold else "Stable"

    chart_history = [
        {
            "time": datetime.fromtimestamp(m * 60, timezone.utc).isoformat(),
            "value": round(lookup[m], 2),
        }
        for m in range(max(min(lookup), last_minute - 29), last_minute + 1)
        if m in lookup
    ]

    return {
        "status": "ready",
        "message": ("Model beats the no-change baseline on held-out readings."
                    if model_outperforms else
                    "Regression did not beat the no-change baseline; using the baseline instead."),
        "horizon_minutes": HORIZON_MINUTES,
        "current": round(current, 2),
        "forecast": round(forecast_value, 2),
        "forecast_at": (last_time + timedelta(minutes=HORIZON_MINUTES)).isoformat(),
        "last_observed_at": last_time.isoformat(),
        "unit": cfg["unit"],
        "trend": trend,
        "method": method,
        "model_mae": round(model_mae, 3),
        "baseline_mae": round(baseline_mae, 3),
        "training_samples": len(train),
        "validation_samples": len(validation),
        "observed_minutes": len(lookup),
        "history": chart_history,
    }
