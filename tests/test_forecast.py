import unittest
from datetime import datetime, timedelta, timezone
from forecast import forecast_metric


class ForecastTests(unittest.TestCase):
    def setUp(self):
        self.now = datetime.now(timezone.utc)
        self.start = self.now.replace(second=0, microsecond=0) - timedelta(minutes=199)

    def rows(self, n=200, temperature=lambda i: 12 + i * 0.08,
             dust=lambda i: 20 + i * 0.15):
        return [
            {
                "bucket": self.start + timedelta(minutes=i),
                "temperature": temperature(i) if temperature else None,
                "dust": dust(i) if dust else None,
            }
            for i in range(n)
        ]

    def test_temperature_ridge_learns_trend(self):
        result = forecast_metric(self.rows(), "temperature", self.now)
        self.assertEqual(result["status"], "ready")
        self.assertEqual(result["method"], "ridge_regression")
        self.assertGreater(result["forecast"], result["current"])
        self.assertLess(result["model_mae"], result["baseline_mae"])
        self.assertEqual(result["horizon_minutes"], 10)
        self.assertGreaterEqual(result["validation_samples"], 12)

    def test_dust_forecast(self):
        result = forecast_metric(self.rows(), "dust", self.now)
        self.assertEqual(result["status"], "ready")
        self.assertGreaterEqual(result["forecast"], 0)
        self.assertEqual(result["unit"], "µg/m³")

    def test_flat_data_falls_back_to_baseline(self):
        result = forecast_metric(self.rows(temperature=lambda i: 25), "temperature", self.now)
        self.assertEqual(result["status"], "ready")
        self.assertEqual(result["method"], "persistence_baseline")
        self.assertEqual(result["forecast"], 25.0)

    def test_no_sensor_data_is_collecting_not_imputed(self):
        result = forecast_metric(self.rows(temperature=None), "temperature", self.now)
        self.assertEqual(result["status"], "collecting")

    def test_short_history_is_not_reported_as_prediction(self):
        rows = self.rows()[160:]
        result = forecast_metric(rows, "temperature", self.now)
        self.assertEqual(result["status"], "collecting")

    def test_stale_readings_are_not_predicted(self):
        result = forecast_metric(self.rows(), "temperature", self.now + timedelta(minutes=8))
        self.assertEqual(result["status"], "stale")

    def test_recent_missing_readings_block_prediction(self):
        rows = [row for row in self.rows() if row["bucket"] != self.start + timedelta(minutes=196)]
        result = forecast_metric(rows, "temperature", self.now)
        self.assertEqual(result["status"], "collecting")

    def test_nan_and_bad_measurements_not_used(self):
        rows = self.rows()
        rows[-1]["temperature"] = float("nan")
        result = forecast_metric(rows, "temperature", self.now)
        self.assertEqual(result["status"], "ready")
        self.assertEqual(result["observed_minutes"], 199)
        self.assertEqual(result["last_observed_at"],
                         (self.start + timedelta(minutes=198)).isoformat())


if __name__ == "__main__":
    unittest.main()
