#!/usr/bin/env python3
"""Summarize the last continuous interval of a Dual Gauge serial capture.

Queued startup lines or disconnects can leave gaps in a capture. Report them and
analyze only the latest interval with <=2.5 seconds between periodic reports.
This checks firmware counters, not physical display appearance.
"""
import argparse
import json
from pathlib import Path
from serial_capture import parse


def analyze(lines, minimum_seconds=0):
    rows = [row for line in lines if (row := parse(line)) is not None]
    if len(rows) < 2:
        raise ValueError("Need at least two complete DUAL reports")
    required = ("up", "imu_fresh", "mag_fresh", "mag_valid", "imu_sample",
                "mag_sample", "imu_err", "mag_err", "imu_retry", "mag_retry",
                "overflow", "h_frame", "c_frame", "h_done", "c_done", "h_tiles",
                "c_tiles", "heap", "min", "maxalloc", "gap_us", "max_us", "drop",
                "offset", "cal", "saved", "reset", "wdt", "h_fps", "c_fps")
    for row in rows:
        missing = set(required) - row.keys()
        if missing:
            raise ValueError(f"Incomplete report: {sorted(missing)}")
    start = 0
    breaks = []
    for i in range(1, len(rows)):
        gap = int(rows[i]["up"]) - int(rows[i-1]["up"])
        if gap <= 0 or gap > 2500:
            breaks.append({"before_up": int(rows[i-1]["up"]),
                           "after_up": int(rows[i]["up"]), "gap_ms": gap})
            start = i
    # Waking the MPU and obtaining the first magnetic sample take startup time.
    while start < len(rows) and int(rows[start]["up"]) < 3000:
        start += 1
    selected = rows[start:]
    if len(selected) < 2:
        raise ValueError("Need two post-startup reports in the latest continuous interval")
    first, last = selected[0], selected[-1]
    seconds = (int(last["up"]) - int(first["up"])) / 1000
    issues = []
    if any(item["gap_ms"] <= 0 for item in breaks):
        issues.append("Uptime restarted or moved backwards within the capture")
    if seconds < minimum_seconds:
        issues.append(f"Continuous interval {seconds:.1f}s is shorter than {minimum_seconds}s")
    for key in ("imu_fresh", "mag_fresh", "mag_valid", "wdt"):
        if any(row[key] != "1" for row in selected):
            issues.append(f"{key} was not continuously 1")
    for key in ("imu_err", "mag_err", "imu_retry", "mag_retry", "overflow", "drop"):
        if any(row[key] != first[key] for row in selected):
            issues.append(f"{key} changed during the interval")
    for key in ("offset", "cal", "saved", "reset"):
        if any(row[key] != first[key] for row in selected):
            issues.append(f"{key} changed during the interval")
    rates = {}
    for key in ("imu_sample", "mag_sample", "h_frame", "c_frame", "h_done", "c_done"):
        values = [int(row[key]) for row in selected]
        if any(b < a for a, b in zip(values, values[1:])):
            issues.append(f"{key} went backwards")
        rates[key + "_per_second"] = round((values[-1] - values[0]) / seconds, 3)
    for key in ("imu_sample", "mag_sample"):
        if rates[key + "_per_second"] <= 0:
            issues.append(f"{key} stopped advancing")
    return {
        "total_reports": len(rows), "continuous_reports": len(selected),
        "excluded_prefix_reports": start, "breaks": breaks,
        "continuous_seconds": round(seconds, 3), "rates": rates,
        "peak_changed_fps": {key: max(float(row[key]) for row in selected)
                             for key in ("h_fps", "c_fps")},
        "heap_range": [min(int(row["heap"]) for row in selected),
                       max(int(row["heap"]) for row in selected)],
        "heap_first_last": [int(first["heap"]), int(last["heap"])],
        "lifetime_min_heap": int(last["min"]),
        "largest_free_block": int(last["maxalloc"]),
        "lifetime_max_us": last["max_us"], "lifetime_mpu_gap_us": int(last["gap_us"]),
        "counters_first_last": {key: [first[key], last[key]] for key in
            ("imu_err", "mag_err", "imu_retry", "mag_retry", "overflow", "drop")},
        "calibration": {key: last[key] for key in ("offset", "cal", "saved")},
        "issues": issues,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    parser.add_argument("--min-seconds", type=float, default=0)
    args = parser.parse_args()
    try:
        result = analyze(args.log.read_text().splitlines(), args.min_seconds)
    except (ValueError, KeyError) as error:
        parser.exit(2, str(error) + "\n")
    print(json.dumps(result, indent=2))
    raise SystemExit(bool(result["issues"]))


if __name__ == "__main__":
    main()
