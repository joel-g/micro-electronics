#!/usr/bin/env python3
"""Verify an opt-in watchdog stall, or that production firmware ignores it.

Uses passive serial control lines; never commands a host-driven MCU reset.
"""
import argparse
import re
import time
from pathlib import Path

import serial
from serial_soak import open_port


def parse_record(line):
    if not line.startswith("up="):
        return None
    head, rest = line.split(" max_us[", 1)
    _, tail = rest.split("]", 1)
    return {k: int(v) for k, v in re.findall(r"\b(\w+)=(-?\d+)\b", head + tail)
            if k not in ("pitch", "roll")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("inject", "production"))
    parser.add_argument("--port", default="/dev/cu.usbmodem101")
    parser.add_argument("--log")
    args = parser.parse_args()
    log_path = Path(args.log or f"diagnostics/watchdog-{args.mode}.log")
    log_path.parent.mkdir(parents=True, exist_ok=True)
    connection = open_port(args.port)
    expected_test = int(args.mode == "inject")
    with log_path.open("w") as log:
        def note(text):
            print(text, flush=True)
            log.write("# " + text + "\n")
            log.flush()

        def read_record(deadline):
            nonlocal connection
            while time.monotonic() < deadline:
                try:
                    line = connection.readline().decode(errors="replace").strip()
                except (serial.SerialException, OSError):
                    connection.close()
                    # USB can briefly disappear during the firmware panic/reset.
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
                    record = parse_record(line)
                    if record:
                        return record
            raise AssertionError("No diagnostics before deadline")

        try:
            baseline = []
            for _ in range(3):
                baseline.append(read_record(time.monotonic() + 8))
            for record in baseline:
                assert record["test"] == expected_test, "Wrong firmware build; no command sent"
                assert record["wdt"] == 1 and record["fresh"] == 1, "Board not ready"
            before = baseline[-1]
            assert baseline[0]["frame"] < before["frame"]
            assert baseline[0]["sample"] < before["sample"]
            note("Baseline: " + str(before))
            started = time.monotonic()
            note("Sending w once; no host reset will be issued")
            connection.write(b"w")
            if args.mode == "inject":
                deadline = started + 20
                while True:
                    after = read_record(deadline)
                    if after["prev"] == 7 and after["up"] < before["up"]:
                        break
                    assert time.monotonic() < deadline, "Watchdog did not restart MCU"
                elapsed = time.monotonic() - started
                assert after["reset"] == 6, f"Expected task-watchdog reset (6): {after}"
                assert 3 <= elapsed <= 15, f"Unexpected recovery time: {elapsed:.2f}s"
                note(f"Watchdog recovery observed after {elapsed:.2f}s: {after}")
            else:
                after = before
            previous = after
            deadline = time.monotonic() + 12
            while time.monotonic() < deadline:
                record = read_record(time.monotonic() + 4)
                assert record["up"] > previous["up"], "Unexpected reset"
                assert record["frame"] > previous["frame"], "Frame updates stopped"
                assert record["sample"] > previous["sample"], "Sensor sampling stopped"
                assert record["fresh"] == 1 and record["wdt"] == 1
                assert record["test"] == expected_test
                assert record["err"] == after["err"], "New I2C errors"
                previous = record
            note("Final: " + str(previous))
            note("PASS: watchdog reset and recovery verified" if expected_test else
                 "PASS: production build ignores w; frame/sensor updates continue")
        finally:
            connection.close()


if __name__ == "__main__":
    main()
