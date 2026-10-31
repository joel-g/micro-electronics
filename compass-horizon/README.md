# Compass & Horizon — v1.0.0

Two instruments on **one** GC9A01 display, selected with a
GPIO13 push button. The project directory is `compass-horizon` (the chosen name
replaces the initially proposed `multi-gauge`).

This project reuses the released [compass](../compass/README.md) and
[attitude indicator](../attitude-indicator/README.md) implementations. Those
projects are preserved independently. No CAN, GPS, second display, or additional
sensor is implemented.

## Release status

**v1.0.0 accepted for publication by the user on 2026-10-30**, with the outstanding
hardware checks below explicitly accepted as release limitations. `VERSION` and
the firmware startup banner identify the release. The version change from the
tested development build does not alter gauge behavior.

The core acceptance checks passed: both instrument faces respond, GPIO13 toggles
once per press/hold, thirty automated switches preserve sensor operation, and
saved mode selection survives a software restart. Compass calibration remains
intact, and magnetometer disconnection leaves the horizon operating.

Complete MPU-fault isolation and fault-screen appearance remain unverified.
Cold power-cycle mode persistence, an extended soak, and intentional watchdog
injection have not been tested in this combined application. Earlier standalone
project results do not substitute for these checks. See the
[verification record](docs/verification.md) for measured evidence and limitations.

## Hardware and wiring

The dedicated [bill of materials](docs/BOM.md) lists quantities, known module/chip
identifiers, compatibility requirements, and unrecorded purchasing details.

| Qty | Part / module identifier | Purpose |
| --- | --- | --- |
| 1 | ESP32-S3 SuperMini, 4 MB flash / 2 MB PSRAM | Controller; PSRAM is not enabled by this firmware |
| 1 | 1.28-inch 240 × 240 GC9A01 SPI TFT | Shared display for both faces |
| 1 | GY-271 with QMC5883P | Magnetometer, `0x2C`, chip ID `0x80` |
| 1 | MPU-6050 breakout | Accelerometer/gyroscope, `0x68` |
| 1 | Normally-open momentary push button | Mode selection; exact manufacturer/part number not recorded |
| 1 set | Interconnects and USB data/power cable | Prototype wiring and programming |

Exact module manufacturers, revisions, and supplier links remain unrecorded.
See the source projects' [compass BOM](../compass/docs/BOM.md) and
[attitude BOM](../attitude-indicator/docs/BOM.md) for known identifiers. GY-271
is a module designation: a QMC5883L/HMC5883L variant is not interchangeable with
this QMC5883P driver.

| ESP32-S3 | Connect to |
| --- | --- |
| GPIO8 | Display SCL / SCLK (SPI clock) |
| GPIO9 | Display SDA / MOSI (SPI data) |
| GPIO10 | Display CS |
| GPIO11 | Display DC |
| GPIO12 | Display RST |
| GPIO1 | MPU-6050 SDA and QMC5883P SDA |
| GPIO2 | MPU-6050 SCL and QMC5883P SCL |
| GPIO13 | One button contact |
| GND | Other button contact, display GND, both sensor GNDs |
| 3V3 | Display VCC and both sensor VCCs |

QMC5883P DRDY is unconnected. The display's SDA/SCL labels denote **SPI**;
the sensors use the separate shared I²C bus. GPIO numbers are not header positions.

```text
ESP32-S3                       GC9A01 (one display)
  GPIO8 ---------------------- SCL / SCLK
  GPIO9 ---------------------- SDA / MOSI
  GPIO10 --------------------- CS
  GPIO11 --------------------- DC
  GPIO12 --------------------- RST

                               QMC5883P        MPU-6050
  GPIO1 ---- shared SDA ------- SDA ----------- SDA
  GPIO2 ---- shared SCL ------- SCL ----------- SCL
                               DRDY unused

  GPIO13 ---- [normally-open push button] ---- GND
    internal pull-up enabled; no external resistor needed

  3V3 ------------------------ VCC of display and both sensors
  GND ------------------------ GND of display and both sensors
```

## Operation

- First use defaults to **Compass**. Later boots restore the last saved mode.
- Press the button to toggle **Compass ↔ Horizon** without restarting.
- GPIO13 is `INPUT_PULLUP`: released HIGH, pressed LOW. A stable falling edge
  after 40 ms generates one event. Holding the button does not repeat or perform
  a long-press action. Release it for at least 40 ms before another press.
- A button held during startup must be released before a press can be registered.
- Mode storage is automatic after 500 ms without another switch. Allow about a
  second before removing power. Serial `MODE save=1` confirms storage. Mode uses
  its own NVS namespace `compass-horizon`, separate from magnetic calibration.
- Both sensors continue updating in either mode. Switching does not re-zero the
  horizon, restart filtering, recalibrate the compass, or reinitialize hardware.

### Compass

The rotating rose, cardinal letters, fixed gold top marker, numeric heading,
wraparound, smoothing, and calibration indicators are reused from the standalone
compass. It uses **magnetic north**, assumes the sensor is level, and has **no
tilt compensation** even though an MPU is now active. Sensor +X is forward and
+Y is right; startup orientation is not automatically zeroed to north.

Previously saved compass calibration is loaded from the same `qmc5883p` NVS
namespace and `hardiron` record format. A regular upload using the same partition
layout preserves it. Erasing flash removes saved calibration and mode settings.
Saving/erasing compass calibration here also changes what the standalone compass
loads when flashed back onto this same ESP32.

To recalibrate, select Compass and close any other serial monitor:

```sh
cd ~/repos/microelectronics/compass-horizon
~/.platformio/penv/bin/python tools/serial_capture.py --send c --seconds 65
```

During the 60-second countdown, slowly rotate flat through a full circle, then
end-over-end and sideways through full circles. Point every face up and down
without straining wires. Keep away from metal/magnets. Return the sensor level.
`CAL INCOMPLETE` means repeat with more motion; previous offsets are retained.
After success, **`CAL RAM` is temporary**. Save before disconnecting power:

```sh
~/.platformio/penv/bin/python tools/serial_capture.py --send s --seconds 5
```

Confirm `SAVE ok=1` / `CAL SAVED`. Reboot without saving restores the older stored
calibration, if any. Calibration corrects hard-iron offsets only. Changing faces
does not cancel calibration; it continues sampling in the background.

### Artificial horizon

The blue/brown horizon, white pitch ladder and bank marks, yellow fixed aircraft
symbol, and moving bank dot are reused from the attitude indicator.

Hold the MPU level during **power-up**, even when booting into Compass. Its first
valid sample establishes pitch/roll zero. Entering Horizon later preserves that
reference. To re-zero without restarting, hold the desired level position and
send `z`. This reference is temporary; an ESP32 restart establishes a new one.

The existing ±2 g / ±250°/s ranges, low-pass setting, axis mapping, and
time-dependent complementary filter are preserved. Filter recovery after a long
gap preserves the original zero. Numeric pitch/roll, frame count, and heartbeat
are hidden by default; the `debug` build enables them on the horizon face.

## Build, upload, and diagnostics

```sh
cd ~/repos/microelectronics/compass-horizon
~/.platformio/penv/bin/pio run
~/.platformio/penv/bin/pio run -t upload
~/.platformio/penv/bin/python tools/serial_capture.py --seconds 30
```

The configured port is `/dev/cu.usbmodem101`, baud 115200. Override upload with
`--upload-port YOUR_PORT`, or capture with `--port YOUR_PORT`. Python needs
`pyserial`; PlatformIO's Python environment includes it. The helper avoids
DTR/RTS resets on macOS/Linux. Buffered older diagnostics can appear first when
reconnecting; check uptime. Close other serial readers before upload/capture.

```sh
# Optional horizon debug overlay:
~/.platformio/penv/bin/pio run -e debug -t upload
# Restore the default build:
~/.platformio/penv/bin/pio run -e esp32-s3-devkitc-1 -t upload
```

| Serial command | Action |
| --- | --- |
| `m` | Toggle face (same switch path as the physical button) |
| `z` | Zero the MPU from its current fresh measurement |
| `c` / `f` / `x` | Start 60-second compass calibration / finish early / cancel |
| `s` / `l` / `e` | Save / load / erase compass calibration |
| `i` | Repeat the incremental I²C scan |
| `r` | Restart ESP32 (unlike the standalone horizon's TFT-only `r`) |
| `?` | Print help |

One `INSTRUMENT` line per second reports current mode, raw button level, physical
press and total switch counts, mode save state, both sensors' sample/error/retry
counts, pitch/roll, heading/XYZ, calibration, heap, timing maxima, and watchdog
state. `max_us` is MPU service, magnetometer read, drawing, SPI batch, and loop.
`gap_us` is the maximum MPU sample gap. A growing `drop` count without a serial
reader is expected: logging drops records instead of blocking the gauges.

`prev` records the prior operation: idle 0, input 1, MPU 2, magnetometer 3,
drawing 4, transfer 5, logging 6, storage 7, scan 8. Interpret it with `reset`;
a manual reset can interrupt any stage. `wdt=1` means the 5-second loop watchdog
is registered. This project does not enable an intentional stall command.

## Architecture and fault handling

```text
src/main.cpp                 scheduling, mode selection/storage, diagnostics
src/input/Button.cpp         debounced input state machine
src/sensors/IMU.cpp           reused MPU reads/filter; nonblocking wake-up
src/sensors/Magnetometer.cpp  reused heading, calibration, freshness/retry logic
src/sensors/qmc5883p.cpp      unchanged QMC5883P register driver
src/sensors/calibration_store.cpp  unchanged saved-calibration format
src/gauges/CompassGauge.cpp   reused compass artwork
src/gauges/AttitudeGauge.cpp  reused horizon artwork and row-span geometry
src/gauges/Display.cpp        shared canvas, palette, and bounded SPI transfers
```

I²C and the display initialize once at startup. Device recovery only configures
the affected sensor: it never tears down the shared bus or runs on a mode switch.
MPU wake-up keeps its 100 ms settling interval as a nonblocking state. Missing
devices retry at most once per second, with 20 ms I²C transaction timeouts.

The selected face shows `MAG FAULT` or `MPU FAULT` for unavailable/stale data.
A missing sensor does not prevent switching to or sampling the other instrument.
A physical short or device holding the **shared** bus low can affect both sensors;
software cannot electrically isolate that fault. The button still selects faces.

The display uses the proven 20 MHz SPI configuration. Two static indexed buffers
total 115,200 bytes, plus a 256-byte transfer tile. Both faces use one fixed palette
with the original colors. Only changed 16 × 8 tiles are sent, up to eight per
loop; each transaction completes before sensor/button processing resumes. Mode
switches replace the old face, including when a previous frame was partly sent.
No framebuffer allocation or display reset occurs during switching.

Drawing is scheduled up to 20 Hz for Compass and 15 Hz for Horizon, subject to
transfer time. An unchanged screen sends no tiles, so a stationary frame counter
alone is not evidence of a freeze. Serial logs verify MCU progress; visual checks
are still needed to confirm the physical display.

## Verification

See [verification notes](docs/verification.md) for test results and limitations.
Native tests use the copied compass math and horizon geometry tests plus new
button bounce/hold/release/startup-held/timer-rollover checks:

```sh
c++ -std=c++17 -O2 -I include test/button_test.cpp src/input/Button.cpp -o /tmp/ch-button && /tmp/ch-button
c++ -std=c++17 -O2 -I include test/compass_math_test.cpp -o /tmp/ch-compass && /tmp/ch-compass
c++ -std=c++17 -O2 -I include test/horizon_geometry_test.cpp -o /tmp/ch-horizon && /tmp/ch-horizon
```

The hardware switch test sends 20 regular and 10 rapid mode switches, checks
continued sensor progress without reinitialization or new errors, saves Horizon,
restarts to verify mode persistence, and returns to Compass. Keep the button
released and MPU level while it runs; it leaves magnetic calibration unchanged.

```sh
~/.platformio/penv/bin/python tools/switch_test.py
```

Physical acceptance: rotate the level magnetometer in Compass, press once and
tilt the MPU in Horizon, then press again to return. Repeat at least ten switches,
hold a press for several seconds to confirm no repeat, and reboot after a saved
Horizon selection to confirm mode persistence. Separate sensor-disconnection
checks can confirm the unaffected face remains available.
