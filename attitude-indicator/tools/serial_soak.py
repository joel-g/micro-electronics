#!/usr/bin/env python3
"""Exercise live diagnostics, USB backpressure, reconnect, and TFT commands.

Run with PlatformIO's Python (which supplies pyserial). Does not flash or reset
the MCU. Supports attitude-indicator v1.0.0 (and the v0.3 diagnostic format).
Requires exclusive access to the serial port.
"""
import argparse
import re
import time
import termios
from pathlib import Path

import serial


class PassiveSerial(serial.Serial):
    # ESP32 USB Serial/JTAG interprets certain RTS/DTR transitions as reset.
    # Do not issue either control-line ioctl when opening this diagnostic port.
    def _update_dtr_state(self):
        pass

    def _update_rts_state(self):
        pass


def open_port(port):
    connection = PassiveSerial(port=None, baudrate=115200, timeout=0.2)
    connection.port = port
    connection.open()
    attributes = termios.tcgetattr(connection.fileno())
    attributes[2] &= ~termios.HUPCL
    termios.tcsetattr(connection.fileno(), termios.TCSANOW, attributes)
    return connection


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/cu.usbmodem101")
    parser.add_argument("--log", default="diagnostics/serial-soak.log")
    parser.add_argument("--backpressure-seconds", type=int, default=60)
    args = parser.parse_args()
    Path(args.log).parent.mkdir(parents=True, exist_ok=True)
    connection = open_port(args.port)
    records = []
    demo_on = False
    with open(args.log, "w") as log:
        def note(message):
            print(message, flush=True)
            log.write("# " + message + "\n")
            log.flush()

        def capture(seconds):
            start_count = len(records)
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                line = connection.readline().decode(errors="replace").strip()
                if not line:
                    continue
                log.write(line + "\n")
                log.flush()
                if line.startswith("up="):
                    head, timing = line.split(" max_us[", 1)
                    timing, tail = timing.split("]", 1)
                    fields = dict(re.findall(r"(\w+)=(-?\d+(?:\.\d+)?)", head + tail))
                    fields.update({"max_" + k: v for k, v in
                                   re.findall(r"(\w+)=(\d+)", timing)})
                    record = {k: float(v) if "." in v else int(v)
                              for k, v in fields.items()}
                    record["frames"] = record["frame"]
                    records.append(record)
            assert len(records) > start_count, "No diagnostics received"
            note("Latest: " + str(records[-1]))
            return records[-1]

        try:
            note("Normal capture: 15 seconds")
            first = capture(15)
            assert first["demo"] == 0, "Start with demo disabled"
            note(f"Host stops reading for {args.backpressure_seconds} seconds")
            # Sleeping in small intervals allows interruption without a long
            # indivisible wait. The serial port stays open but unread.
            deadline = time.monotonic() + args.backpressure_seconds
            while time.monotonic() < deadline:
                time.sleep(min(0.2, max(0, deadline - time.monotonic())))
            after_stall = capture(5)
            assert after_stall["up"] - first["up"] >= args.backpressure_seconds * 900
            note("Close serial monitor for 20 seconds")
            connection.close()
            time.sleep(20)
            connection = open_port(args.port)
            after_close = capture(5)
            assert after_close["up"] > after_stall["up"] + 18000
            note("Enable display-only synthetic motion for 5 seconds")
            connection.write(b"d")
            demo_on = True
            assert capture(5)["demo"] == 1
            connection.write(b"d")
            demo_on = False
            assert capture(3)["demo"] == 0
            note("Reinitialize TFT, preserving sensor state")
            connection.write(b"r")
            last = capture(5)

            for a, b in zip(records, records[1:]):
                assert b["up"] > a["up"], "MCU restarted or uptime stopped"
                assert b["frames"] > a["frames"], "Frame transfers stopped"
                assert b["sample"] > a["sample"], "Sensor sampling stopped"
            assert all(r["fresh"] == 1 and r["wdt"] == 1 for r in records)
            assert last["err"] == first["err"], "I2C errors during test"
            assert first["heap"] - last["heap"] < 4096, "Possible heap leak"
            elapsed = (last["up"] - first["up"]) / 1000
            note("PASS: %.2f fps, %.2f samples/s over %.1f s; dropped logs=%d; "
                 "heap change=%d bytes. Physical TFT appearance still needs observation."
                 % ((last["frames"] - first["frames"]) / elapsed,
                    (last["sample"] - first["sample"]) / elapsed, elapsed,
                    last["drop"] - first["drop"], last["heap"] - first["heap"]))
        finally:
            if connection.is_open:
                if demo_on:
                    connection.write(b"d")
                connection.close()


if __name__ == "__main__":
    main()
