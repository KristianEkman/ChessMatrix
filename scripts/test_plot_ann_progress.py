#!/usr/bin/env python3

import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
	sys.path.insert(0, str(SCRIPT_DIR))

from plot_ann_progress import build_plot_series, load_progress_rows


SAMPLE_CSV = textwrap.dedent(
	"""\
	kind,game,roots,window_roots,avg_abs_error,avg_sq_error,total_avg_abs_error,total_avg_sq_error,last_abs_error,validation_samples,validation_avg_abs_error,validation_avg_sq_error,validation_last_abs_error,elapsed_ms,roots_per_sec,learn_rate
	progress,1,2,2,0.200000,0.050000,0.200000,0.050000,0.200000,4,0.300000,0.090000,0.300000,1000,2.0,0.050000
	final,1,3,1,0.100000,0.010000,0.150000,0.030000,0.100000,4,0.250000,0.070000,0.250000,1500,2.0,0.025000
	"""
)


class PlotAnnProgressTests(unittest.TestCase):
	def write_csv(self, content: str) -> Path:
		handle = tempfile.NamedTemporaryFile("w", delete=False, suffix=".csv")
		self.addCleanup(lambda: Path(handle.name).unlink(missing_ok=True))
		handle.write(content)
		handle.close()
		return Path(handle.name)

	def test_load_progress_rows_parses_numeric_fields(self) -> None:
		csv_path = self.write_csv(SAMPLE_CSV)

		rows = load_progress_rows(csv_path)

		self.assertEqual(2, len(rows))
		self.assertEqual("progress", rows[0]["kind"])
		self.assertEqual(2, rows[0]["roots"])
		self.assertEqual(4, rows[1]["validation_samples"])
		self.assertAlmostEqual(0.15, rows[1]["total_avg_abs_error"])
		self.assertAlmostEqual(2.0, rows[1]["roots_per_sec"])
		self.assertAlmostEqual(0.025, rows[1]["learn_rate"])

	def test_build_plot_series_filters_final_rows_when_requested(self) -> None:
		csv_path = self.write_csv(SAMPLE_CSV)
		rows = load_progress_rows(csv_path)

		series = build_plot_series(rows, x_axis="roots", include_final=False)

		self.assertEqual("Trained roots", series["x_label"])
		self.assertEqual([2.0], series["x"])
		self.assertEqual([0.2], series["train_abs"])
		self.assertEqual([0.3], series["validation_abs"])
		self.assertEqual(["progress"], series["kinds"])

	def test_build_plot_series_supports_elapsed_minutes_axis(self) -> None:
		csv_path = self.write_csv(SAMPLE_CSV)
		rows = load_progress_rows(csv_path)

		series = build_plot_series(rows, x_axis="minutes", include_final=True)

		self.assertEqual("Elapsed minutes", series["x_label"])
		self.assertEqual([1000.0 / 60000.0, 1500.0 / 60000.0], series["x"])
		self.assertEqual([0.05, 0.03], series["train_sq"])
		self.assertEqual([0.09, 0.07], series["validation_sq"])
		self.assertEqual([0.05, 0.025], series["learn_rate"])
		self.assertEqual(["progress", "final"], series["kinds"])

	def test_load_progress_rows_returns_empty_for_header_only_csv(self) -> None:
		csv_path = self.write_csv(SAMPLE_CSV.splitlines()[0] + "\n")

		rows = load_progress_rows(csv_path)

		self.assertEqual([], rows)


if __name__ == "__main__":
	unittest.main()