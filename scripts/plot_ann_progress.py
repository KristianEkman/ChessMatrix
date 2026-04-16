#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Any

REQUIRED_COLUMNS = {
    "kind",
    "game",
    "roots",
    "window_roots",
    "avg_abs_error",
    "avg_sq_error",
    "total_avg_abs_error",
    "total_avg_sq_error",
    "last_abs_error",
    "validation_samples",
    "validation_avg_abs_error",
    "validation_avg_sq_error",
    "validation_last_abs_error",
    "elapsed_ms",
    "roots_per_sec",
    "learn_rate",
}

INT_FIELDS = {
    "game",
    "roots",
    "window_roots",
    "validation_samples",
    "elapsed_ms",
}

FLOAT_FIELDS = {
    "avg_abs_error",
    "avg_sq_error",
    "total_avg_abs_error",
    "total_avg_sq_error",
    "last_abs_error",
    "validation_avg_abs_error",
    "validation_avg_sq_error",
    "validation_last_abs_error",
    "roots_per_sec",
    "learn_rate",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot ChessMatrix ANN training progress CSV files."
    )
    parser.add_argument("csv_path", type=Path, help="Path to the trainer progress CSV file.")
    parser.add_argument(
        "--watch",
        action="store_true",
        help="Reload the CSV and redraw the plot while training is still running.",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=2.0,
        help="Refresh interval in seconds when --watch is used. Default: 2.0.",
    )
    parser.add_argument(
        "--x-axis",
        choices=("roots", "minutes"),
        default="roots",
        help="Plot progress by trained roots or elapsed minutes. Default: roots.",
    )
    parser.add_argument(
        "--hide-final",
        action="store_true",
        help="Exclude the final summary row from the plot.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Optional PNG path to save the current figure.",
    )
    parser.add_argument(
        "--title",
        default=None,
        help="Optional custom figure title.",
    )
    return parser.parse_args()


def load_progress_rows(csv_path: Path) -> list[dict[str, Any]]:
    if not csv_path.exists():
        raise FileNotFoundError(f"CSV file not found: {csv_path}")

    with csv_path.open("r", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            return []

        missing_columns = REQUIRED_COLUMNS.difference(reader.fieldnames)
        if missing_columns:
            missing = ", ".join(sorted(missing_columns))
            raise ValueError(f"CSV file is missing required columns: {missing}")

        rows: list[dict[str, Any]] = []
        for row_number, raw_row in enumerate(reader, start=2):
            if raw_row is None or not any((value or "").strip() for value in raw_row.values()):
                continue

            parsed_row: dict[str, Any] = {"kind": (raw_row.get("kind") or "").strip()}
            for field in INT_FIELDS:
                parsed_row[field] = _parse_int_field(raw_row, field, row_number)
            for field in FLOAT_FIELDS:
                parsed_row[field] = _parse_float_field(raw_row, field, row_number)
            rows.append(parsed_row)

    return rows


def build_plot_series(
    rows: list[dict[str, Any]],
    x_axis: str = "roots",
    include_final: bool = True,
) -> dict[str, Any]:
    if x_axis not in {"roots", "minutes"}:
        raise ValueError(f"Unsupported x-axis: {x_axis}")

    x_values: list[float] = []
    train_abs: list[float] = []
    validation_abs: list[float] = []
    train_sq: list[float] = []
    validation_sq: list[float] = []
    learn_rate: list[float] = []
    kinds: list[str] = []

    for row in rows:
        row_kind = row.get("kind", "")
        if row_kind not in {"progress", "final"}:
            continue
        if row_kind == "final" and not include_final:
            continue

        x_value = float(row["roots"])
        x_label = "Trained roots"
        if x_axis == "minutes":
            x_value = float(row["elapsed_ms"]) / 60000.0
            x_label = "Elapsed minutes"

        x_values.append(x_value)
        train_abs.append(float(row["total_avg_abs_error"]))
        validation_abs.append(float(row["validation_avg_abs_error"]))
        train_sq.append(float(row["total_avg_sq_error"]))
        validation_sq.append(float(row["validation_avg_sq_error"]))
        learn_rate.append(float(row["learn_rate"]))
        kinds.append(row_kind)

    return {
        "x": x_values,
        "x_label": "Trained roots" if x_axis == "roots" else "Elapsed minutes",
        "train_abs": train_abs,
        "validation_abs": validation_abs,
        "train_sq": train_sq,
        "validation_sq": validation_sq,
        "learn_rate": learn_rate,
        "kinds": kinds,
    }


def main() -> int:
    args = parse_args()
    if args.interval <= 0.0:
        raise SystemExit("--interval must be greater than zero")

    pyplot = _load_pyplot()
    pyplot.style.use("seaborn-v0_8-whitegrid")

    figure, axes = pyplot.subplots(3, 1, figsize=(11, 9.0), sharex=True)
    if args.watch:
        pyplot.ion()
        pyplot.show(block=False)

    last_signature: tuple[int, int] | None = None
    try:
        while True:
            signature = _get_file_signature(args.csv_path)
            if signature != last_signature:
                _refresh_figure(
                    figure,
                    axes,
                    args.csv_path,
                    x_axis=args.x_axis,
                    include_final=not args.hide_final,
                    output_path=args.output,
                    custom_title=args.title,
                )
                last_signature = signature

            if not args.watch:
                break
            if not pyplot.fignum_exists(figure.number):
                break
            pyplot.pause(args.interval)
    except KeyboardInterrupt:
        pass

    if not args.watch and args.output is None:
        pyplot.show()
    return 0


def _parse_int_field(row: dict[str, str], field: str, row_number: int) -> int:
    raw_value = (row.get(field) or "").strip()
    try:
        return int(raw_value)
    except ValueError as exc:
        raise ValueError(f"Invalid integer in row {row_number} column {field}: {raw_value}") from exc


def _parse_float_field(row: dict[str, str], field: str, row_number: int) -> float:
    raw_value = (row.get(field) or "").strip()
    try:
        return float(raw_value)
    except ValueError as exc:
        raise ValueError(f"Invalid float in row {row_number} column {field}: {raw_value}") from exc


def _load_pyplot():
    try:
        import matplotlib.pyplot as pyplot
    except ImportError as exc:
        raise SystemExit(
            "matplotlib is required for plotting. Run: bash scripts/setup_plot_venv.sh "
            "and then: source .venv-plot/bin/activate"
        ) from exc
    return pyplot


def _get_file_signature(csv_path: Path) -> tuple[int, int]:
    if not csv_path.exists():
        return (0, 0)
    stat = csv_path.stat()
    return (stat.st_mtime_ns, stat.st_size)


def _refresh_figure(
    figure,
    axes,
    csv_path: Path,
    x_axis: str,
    include_final: bool,
    output_path: Path | None,
    custom_title: str | None,
) -> None:
    title = custom_title or f"ANN Training Progress: {csv_path.name}"

    try:
        rows = load_progress_rows(csv_path)
        series = build_plot_series(rows, x_axis=x_axis, include_final=include_final)
    except (FileNotFoundError, ValueError) as exc:
        _draw_message(figure, axes, title, str(exc))
        return

    if not series["x"]:
        _draw_message(figure, axes, title, f"Waiting for progress rows in {csv_path.name}")
        return

    latest_row = rows[-1]
    latest_kind = latest_row["kind"]
    latest_roots = latest_row["roots"]
    latest_train_abs = latest_row["total_avg_abs_error"]
    latest_validation_abs = latest_row["validation_avg_abs_error"]
    latest_learn_rate = latest_row["learn_rate"]
    subtitle = (
        f"latest={latest_kind} roots={latest_roots} "
        f"train-abs={latest_train_abs:.4f} validation-abs={latest_validation_abs:.4f} "
        f"lr={latest_learn_rate:.6f}"
    )

    abs_axis, sq_axis, lr_axis = axes
    abs_axis.clear()
    sq_axis.clear()
    lr_axis.clear()

    abs_axis.plot(series["x"], series["train_abs"], color="#155e75", linewidth=2.2, label="train avg abs")
    abs_axis.plot(series["x"], series["validation_abs"], color="#b45309", linewidth=2.0, label="validation avg abs")
    sq_axis.plot(series["x"], series["train_sq"], color="#0f766e", linewidth=2.2, label="train avg sq")
    sq_axis.plot(series["x"], series["validation_sq"], color="#b91c1c", linewidth=2.0, label="validation avg sq")
    lr_axis.plot(series["x"], series["learn_rate"], color="#4338ca", linewidth=2.1, label="learn rate")

    if series["kinds"][-1] == "final":
        abs_axis.scatter(series["x"][-1], series["train_abs"][-1], color="#155e75", s=36, zorder=4)
        abs_axis.scatter(series["x"][-1], series["validation_abs"][-1], color="#b45309", s=36, zorder=4)
        sq_axis.scatter(series["x"][-1], series["train_sq"][-1], color="#0f766e", s=36, zorder=4)
        sq_axis.scatter(series["x"][-1], series["validation_sq"][-1], color="#b91c1c", s=36, zorder=4)
        lr_axis.scatter(series["x"][-1], series["learn_rate"][-1], color="#4338ca", s=36, zorder=4)

    abs_axis.set_ylabel("Absolute error")
    sq_axis.set_ylabel("Squared error")
    lr_axis.set_ylabel("Learn rate")
    lr_axis.set_xlabel(series["x_label"])
    abs_axis.set_title(subtitle, fontsize=10)
    abs_axis.legend(loc="upper right")
    sq_axis.legend(loc="upper right")
    lr_axis.legend(loc="upper right")

    figure.suptitle(title, fontsize=14)
    figure.tight_layout()

    if output_path is not None:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        figure.savefig(output_path, dpi=140, bbox_inches="tight")

    figure.canvas.draw_idle()


def _draw_message(figure, axes, title: str, message: str) -> None:
    for axis in axes:
        axis.clear()
        axis.set_axis_off()

    axes[0].text(
        0.5,
        0.55,
        message,
        ha="center",
        va="center",
        fontsize=12,
        transform=axes[0].transAxes,
    )
    axes[0].text(
        0.5,
        0.35,
        "Start the trainer with --progress-csv and rerun with --watch to follow updates.",
        ha="center",
        va="center",
        fontsize=10,
        color="#475569",
        transform=axes[0].transAxes,
    )
    figure.suptitle(title, fontsize=14)
    figure.tight_layout()
    figure.canvas.draw_idle()


if __name__ == "__main__":
    raise SystemExit(main())