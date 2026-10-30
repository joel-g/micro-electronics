#!/usr/bin/env python3
"""Verify opt-in watchdog recovery or normal firmware ignoring the stall command.

Never requests a host reset or changes calibration. Raw logs include all records.
"""
import argparse
from pathlib import Path
import time

import serial
from serial_capture import open_port, parse


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("inject", "production"))
    parser.add_argument("--port", default="/dev/cu.usbmodem101")
    parser.add_argument("--log")
    parser.add_argument("--offset", help="Expected saved X,Y,Z offset string from preflight")
    args = parser.parse_args()
    path = Path(args.log or f"diagnostics/watchdog-{args.mode}.log")
    path.parent.mkdir(parents=True, exist_ok=True)
    expected_test = str(int(args.mode == "inject"))
    connection = open_port(args.port)
    with path.open("w") as log:
        def note(message):
            print(message, flush=True)
            log.write("# " + message + "\n")
            log.flush()

        def read_record(deadline):
            nonlocal connection
            while time.monotonic() < deadline:
                try:
                    line = connection.readline().decode(errors="replace").strip()
                except (serial.SerialException, OSError):
                    connection.close()
                    while time.monotonic() < deadline:
                        try:
                            connection = open_port(args.port)
                            break
                        except (serial.SerialException, OSError):
                            time.sleep(0.2)
                    else:
                        raise AssertionError("USB did not return before deadline")
                    continue
                if line:
                    log.write(line + "\n")
                    log.flush()
                    record = parse(line)
                    if record:
                        return record
            raise AssertionError("No diagnostics before deadline")

        def healthy(record, offset=None):
            for key in ("fresh", "valid", "cal", "saved", "wdt", "nvs"):
                check(record.get(key) == "1", f"Not ready: {key}: {record}")
            check(record.get("test") == expected_test, "Wrong firmware build")
            check(record["collecting"] == "0", "Calibration in progress")
            if offset is not None:
                check(record["offset"] == offset, "Saved calibration changed")

        try:
            # Allow buffered old logs and startup scan to pass. Observe several
            # live reports and sufficient uptime to distinguish a later restart.
            started = time.monotonic()
            deadline = started + 25
            baseline = []
            while True:
                record = read_record(deadline)
                if record.get("test") == expected_test and record["fresh"] == "1":
                    baseline.append(record)
                if (record.get("test") == expected_test and record["fresh"] == "1"
                        and time.monotonic() - started >= 4 and len(baseline) >= 3
                        and int(record["up"]) >= 10000):
                    break
            before = baseline[-1]
            healthy(before, args.offset)
            for record in baseline[-3:]:
                healthy(record, args.offset)
                check(record["err"] == before["err"] and record["overflow"] == before["overflow"],
                      "Sensor errors increasing before test")
            check(int(before["sample"]) > int(baseline[-3]["sample"]), "Sampling stopped")
            offset = before["offset"]
            note("Baseline: " + str(before))
            started = time.monotonic()
            note("Sending w once; no host reset will be issued")
            connection.write(b"w")
            if args.mode == "inject":
                while True:
                    after = read_record(started + 20)
                    if (after["prev"] == "8" and int(after["up"]) < int(before["up"])
                            and after["fresh"] == "1"):
                        break
                elapsed = time.monotonic() - started
                check(after["reset"] == "6", "Reset was not the task watchdog")
                check(3 <= elapsed <= 15, f"Unexpected recovery time: {elapsed:.2f}s")
                healthy(after, offset)
                check(int(after["frame"]) > 0, "No completed display transfer after restart")
                note(f"Watchdog recovery after {elapsed:.2f}s: {after}")
            else:
                after = before
            previous = after
            deadline = time.monotonic() + 12
            while time.monotonic() < deadline:
                record = read_record(time.monotonic() + 4)
                healthy(record, offset)
                check(int(record["up"]) > int(previous["up"]), "Unexpected reset")
                check(record["reset"] == after["reset"], "Reset reason changed")
                check(record["err"] == after["err"] and record["overflow"] == after["overflow"],
                      "New sensor errors after command")
                check(int(record["sample"]) > int(previous["sample"]), "Sampling stopped")
                # An unchanged compass screen correctly sends no new frames.
                check(int(record["frame"]) >= int(previous["frame"]), "Frame count regressed")
                previous = record
            note("Final: " + str(previous))
            note("PASS: watchdog recovery and saved calibration verified" if expected_test == "1"
                 else "PASS: normal firmware ignores w; sampling and saved calibration healthy")
        finally:
            connection.close()


if __name__ == "__main__":
    main()
