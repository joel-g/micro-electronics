#!/usr/bin/env python3
"""Capture Dual Gauge with Buttons diagnostics without toggling USB reset/control lines.

Example: python tools/serial_capture.py --send c --seconds 65 --log diagnostics/calibration.log
"""
import argparse
from pathlib import Path
import termios
import time
import serial


class PassiveSerial(serial.Serial):
    def _update_dtr_state(self):
        pass

    def _update_rts_state(self):
        pass


def open_port(port="/dev/cu.usbmodem1101"):
    connection = PassiveSerial(port, 115200, timeout=0.2)
    attrs = termios.tcgetattr(connection.fileno())
    attrs[2] &= ~termios.HUPCL
    termios.tcsetattr(connection.fileno(), termios.TCSANOW, attrs)
    return connection


def parse(line):
    if not line.startswith("DUAL up="):
        return None
    return dict(item.split("=", 1) for item in line.split()[1:] if "=" in item)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/cu.usbmodem1101")
    parser.add_argument("--seconds", type=float, default=15)
    parser.add_argument("--send", default="")
    parser.add_argument("--log", default="diagnostics/serial.log")
    args = parser.parse_args()
    Path(args.log).parent.mkdir(parents=True, exist_ok=True)
    with open_port(args.port) as connection, open(args.log, "w") as log:
        if args.send:
            connection.write(args.send.encode("ascii"))
        deadline = time.monotonic() + args.seconds
        while time.monotonic() < deadline:
            line = connection.readline().decode(errors="replace")
            if line:
                log.write(line)
                log.flush()
                print(line.rstrip(), flush=True)


if __name__ == "__main__":
    main()
