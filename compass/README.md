# Standalone digital compass — v1.0.0

Firmware v1.0.0 for an ESP32-S3 SuperMini, one 1.28-inch GC9A01 round
display, and a GY-271 module containing a **QMC5883P**. Built with PlatformIO and
Arduino. The previous [attitude indicator](../attitude-indicator/README.md) remains
a separate project.

The display shows a rotating compass rose, N/E/S/W, a fixed yellow heading marker
at the top, and a large `000`–`359°` magnetic heading. Heading animation takes the
short path across north, including `359° → 000°` in either direction.

## Release status

**v1.0.0 accepted for publication by the user on 2026-10-30.** The version is
recorded in `VERSION` and the serial startup banner. Completed checks include
build/upload, heading and calibration math, physical heading response and north
wraparound, calibration persistence through five power cycles, a qualitative
iPhone compass comparison, sensor fault recovery, watchdog recovery, and a
user-reported run of more than 30 minutes.

The soak result is a user observation: exact duration, serial-monitor state,
and continuous error/memory measurements were not recorded. This release uses
magnetic north, assumes a level sensor, and provides hard-iron calibration only. See the
[verification report](docs/verification-v1.md) for measured results and limits.

## Hardware and connections

See the [bill of materials](docs/BOM.md) for quantities, module and chip
identifiers, and the manufacturer/supplier details still to be recorded.

- ESP32-S3 SuperMini: 4 MB flash, 2 MB PSRAM. This firmware does not require or
  enable PSRAM.
- GC9A01: 240 × 240 round SPI TFT, one display only.
- GY-271 **QMC5883P** three-axis magnetometer.
- MPU-6050 remains connected at `0x68`. The scanner acknowledges its presence;
  the compass does not configure it or read its measurements.

| Module | Pin | ESP32-S3 |
| --- | --- | --- |
| GC9A01 | VCC | 3V3 |
| GC9A01 | GND | GND |
| GC9A01 | SCL / SCLK | GPIO8 |
| GC9A01 | SDA / MOSI | GPIO9 |
| GC9A01 | CS | GPIO10 |
| GC9A01 | DC | GPIO11 |
| GC9A01 | RST | GPIO12 |
| QMC5883P | VCC | 3V3 |
| QMC5883P | GND | GND |
| QMC5883P | SDA | GPIO1 |
| QMC5883P | SCL | GPIO2 |
| QMC5883P | DRDY | Not connected |
| MPU-6050 | SDA | GPIO1, shared |
| MPU-6050 | SCL | GPIO2, shared |
| MPU-6050 | VCC / GND | Shared 3V3 / GND |

```text
ESP32-S3 SuperMini              GC9A01 SPI display
  GPIO8  --------------------> SCL / SCLK
  GPIO9  --------------------> SDA / MOSI
  GPIO10 --------------------> CS
  GPIO11 --------------------> DC
  GPIO12 --------------------> RST

                               QMC5883P         MPU-6050
  GPIO1  ---- SDA bus --------- SDA ------------ SDA
  GPIO2  ---- SCL bus --------- SCL ------------ SCL
                               DRDY: unused

  3V3    --------------------> VCC of all three modules
  GND    --------------------> GND of all three modules
```

The display's pins labeled SDA/SCL carry **SPI**, not I²C. The sensors share the
separate I²C bus on GPIO1/2. Pin labels on the module matter more than header order.

## Sensor identification

The connected hardware was scanned and returned:

```text
QMC5883P: 0x2C, chip ID register 0x00 = 0x80
MPU-6050: 0x68, present but ignored
No ACK at 0x0D or 0x1E
```

GY-271 is a module designation, not a guarantee of the chip fitted. This driver
checks the QMC5883P address and ID before configuring it; it does not apply a
QMC5883L or HMC5883L register map to an unidentified sensor.

Configuration uses ±8 G range, normal measurement mode at 50 Hz, automatic
set/reset, and OSR1/OSR2 of 8. Register readback is `0x0A = 0xC5`, `0x0B = 0x08`.
XYZ data comes from registers `0x01`–`0x06`; status/DRDY is polled at `0x09`.
Raw readings are signed counts, approximately 37.5 counts per µT at this range.

Register references: [QST QMC5883P datasheet](https://cdn-shop.adafruit.com/product-files/6388/C2847467.pdf)
and [Adafruit's QMC5883P driver](https://github.com/adafruit/Adafruit_QMC5883P).

## Heading reference and limitations

Keep the module approximately **level during normal use**. Sensor +X is defined
as forward and +Y as right; both belong in the horizontal plane. The calculation
is `atan2(-(Y - offsetY), X - offsetX)`, normalized to 0–360°. Mount the module to
match those axes; module edge orientation is not automatically detected.

- `000°` is magnetic north, `090°` east, `180°` south, and `270°` west.
- Startup does **not** make the current orientation north or zero the heading.
- There is no declination correction, GPS, or true-north conversion.
- There is no tilt compensation and no MPU-6050 input. Tilting can change the
  heading even when the module has not turned horizontally.
- Calibration corrects a fixed hard-iron offset only. It does not correct
  soft-iron distortion, axis scale differences, or changing magnetic interference.
- Keep the module away from magnets, speakers, steel work surfaces, and large
  current-carrying wires. Calibrate again if the mounting environment changes.

## Calibration and retaining it across restarts

Before calibration the display says `CAL REQUIRED`; the heading is provisional.
Use the serial command `c` to start a nonblocking 60-second calibration countdown.

During the countdown, slowly rotate the module through many orientations:

1. **Yaw:** keep it flat and turn it through a full circle.
2. **Pitch:** tip it end-over-end through a full circle.
3. **Roll:** roll it sideways through a full circle.
4. Repeat with different starting orientations so every face points up and down.
   Avoid straining the connected wires. Return the module level afterward.

The firmware records minimum and maximum X/Y/Z and uses each midpoint as its
offset. Acceptance requires at least 10 seconds, 200 samples, and a span of at
least 400 counts on **each** axis. These checks reject insufficient movement;
they do not prove that calibration is accurate. A rejected attempt displays
`CAL INCOMPLETE` briefly and preserves the previous calibration.

Successful calibration initially says `CAL RAM`. Send `s` to store it in ESP32
NVS; `CAL SAVED` confirms success. Storage is explicit, versioned, checksummed,
and read back after writing. Saved offsets load automatically on restart. An
unsaved recalibration does not overwrite an older saved calibration.

### Recalibrate, save, and check

Connect the ESP32 by USB and close any other serial monitor. Start calibration
with the following commands on this development Mac:

```sh
cd ~/repos/microelectronics/compass
~/.platformio/penv/bin/python tools/serial_capture.py --send c --seconds 65 --log diagnostics/calibration.log
```

During the 60-second countdown, perform the yaw, pitch, and roll movements above.
Return the sensor level when the countdown ends. If the display shows
`CAL INCOMPLETE`, repeat with more movement across all three axes; the previous
calibration remains intact.

When the display shows `CAL RAM`, check that the heading responds as expected.
**Before unplugging or restarting**, save the new calibration:

```sh
~/.platformio/penv/bin/python tools/serial_capture.py --send s --seconds 5 --log diagnostics/save.log
```

Confirm the display changes to **`CAL SAVED`** and the serial output reports
`SAVE ok=1`. If saving fails, keep the board powered and check the diagnostic
output before retrying. After a successful save, unplug and reconnect USB, then
check the heading again with the sensor level. It should still show `CAL SAVED`.

These commands use PlatformIO's Python environment, which includes `pyserial`.
On another system, substitute a Python interpreter with `pyserial` installed and
add `--port YOUR_PORT` to each capture command if needed.

### Why the heading can change after reconnecting

`CAL RAM` means the current calibration exists only in temporary memory. If you
unplug at this point, the new calibration is lost. On startup the firmware loads
the **last saved calibration**, if one exists, and displays `CAL SAVED`. That
label confirms stored calibration is active; it does not mean the most recent
unsaved calibration was retained. Without a saved calibration, it displays
`CAL REQUIRED` instead.

If the compass worked well before unplugging but quarter-turns no longer match
approximately 90° changes afterward, recalibrate and explicitly save before
disconnecting. If the mismatch remains after saving and restarting, keep the
sensor level and investigate nearby magnetic interference and soft-iron
distortion; this basic calibration only corrects hard-iron offsets.

## Build, upload, and serial commands

```sh
cd compass
pio run
pio run -t upload
python tools/serial_capture.py --seconds 20
```

The configured USB port is `/dev/cu.usbmodem101`. Override upload with
`pio run -t upload --upload-port YOUR_PORT` and capture with `--port YOUR_PORT`.
Close other serial monitors before uploading or using the capture helper.

| Command | Action |
| --- | --- |
| `c` | Start a fresh 60-second calibration |
| `f` | Finish early, subject to the same minimum data requirements |
| `x` | Cancel calibration, keeping previous offsets |
| `s` | Save the accepted calibration to NVS |
| `l` | Load previously saved calibration |
| `e` | Erase saved calibration and reset RAM offsets |
| `i` | Scan I²C again, one address per loop |
| `r` | Restart the ESP32 |
| `?` | Print command help |

Commands are single lowercase characters; a newline is optional. The capture
helper avoids changing DTR/RTS and clears HUPCL on macOS/Linux. Some other serial
tools can reset native USB ESP32 boards when opening/closing the port. Buffered
older log lines can appear first when reconnecting; check the `up` timestamp.

For the diagnostic-only firmware showing live XYZ on the display:

```sh
pio run -e probe -t upload
```

Upload the default environment again to restore the compass. To restore the
artificial horizon, build/upload from `../attitude-indicator` instead.

## Diagnostics and responsiveness

One serial `COMPASS` line per second reports uptime, samples, completed changed
frames, tiles sent, raw XYZ, filtered/target heading, offsets, calibration state,
sensor errors/overflows/reinitializations, heap, and maximum operation durations.

- `fresh=1` means a sample arrived within 250 ms. `valid=1` indicates enough
  horizontal field for heading calculation; it does not certify calibration.
- `err` counts I²C read failures; `overflow` counts rejected overflow samples.
- `retry=1` normally means the initial initialization, not a failure.
- `drop` counts periodic diagnostic lines skipped when USB cannot accept them.
  A growing count with no serial reader is expected; the application keeps running.
- `max_us` is I²C read, drawing, a transfer batch, and full loop, respectively.
- `wdt=1` means the loop task watchdog is registered; its timeout is 5 seconds.
- `test=1` identifies the opt-in watchdog test build; normal firmware reports `0`.
- `reset` is the ESP reset reason. `prev` is the retained prior stage: idle 0,
  sensor 1, draw 2, SPI transfer 3, logging 4, recovery 5, NVS 6, scan 7,
  intentional watchdog test 8. Interpret a retained stage with the reset reason;
  a manual reset can interrupt any stage.
  `4294967295` means no valid retained stage was available.

Missing/stale sensor data replaces the heading with `MAG FAULT`. I²C operations
have a 20 ms timeout; repeated bus errors or one second without samples trigger
reinitialization, no more than once per second. A horizontal field below about
2 µT shows `WEAK FIELD`. No stale heading is intentionally presented as live.

Two static indexed buffers total 115,200 bytes, plus a 256-byte tile transfer
buffer. No framebuffer depends on PSRAM or repeated heap allocation. Only changed
16 × 8 tiles are sent over the existing 20 MHz SPI connection, at most four tiles
per loop. Frames are prepared at most 20 times per second; complete redraw speed
depends on how many tiles change. Each SPI transaction finishes before returning.
An unchanged screen causes no pixel transfers, so a stationary `frame` counter
alone is **not** a freeze. USB logging never waits for a connected host.

## Verification

The [v1 verification report](docs/verification-v1.md) documents the watchdog
procedure and results. Fault injection is available only in the explicitly
selected `watchdog-test` environment; the default environment remains normal
compass firmware.

Native heading and calibration math tests:

```sh
c++ -std=c++17 -O2 -I include test/compass_math_test.cpp -o /tmp/compass-math-test
/tmp/compass-math-test
```

They cover both directions across north, cardinal axis mapping, weak-field
rejection, insufficient calibration movement/time, known synthetic offsets, and
heading recovery around a full circle.

Hardware observations on 2026-10-30:

- Default and probe firmware build and upload successfully.
- Scan and chip ID confirm QMC5883P; approximately 49 new samples/second.
- Live XYZ and compass headings respond to physical movement; the user confirmed
  the rotating rose is visible and changes with rotation.
- Initial compass capture: no sensor errors or overflows, stable free heap around
  225,920 bytes, largest observed loop about 7.8 ms, watchdog registered.
- A physical 60-second calibration accepted 2,949 samples, with X/Y/Z spans of
  4,934 / 5,487 / 5,028 counts. Offsets were saved with successful readback and
  reloaded unchanged after a commanded restart (`cal=1 saved=1`). Live samples
  resumed without sensor errors.
- The user confirmed clockwise heading progression, smooth north wraparound,
  and the `CAL SAVED` indication after calibration.
- Closing and reopening the serial reader confirmed continued sampling near
  49 Hz, no reset, and no sensor errors. USB backpressure increased the dropped
  report count without stopping the compass.
- The user secured the sensor to the bench and completed five USB unplug/replug
  cycles, reporting that the cold-start persistence check passed. They also
  compared the reading with an iPhone compass and reported agreement. This is a
  qualitative reference check; no angular error measurements or phone north
  reference setting were recorded.
- The user disconnected a wire and observed `MAG FAULT`, then reconnected it
  and confirmed the compass resumed working. Sensor fault detection and automatic
  recovery passed this physical check; the wire and recovery time were not recorded.
- Watchdog fault injection passed: the deliberate loop stall caused a task
  watchdog restart (`reset=6 prev=8`), with healthy readings received after
  7.23 seconds and saved calibration unchanged. Normal firmware was restored;
  it reports `test=0`, ignores the stall command, and passed the follow-up check.
  See the [verification report](docs/verification-v1.md) for evidence and scope.
- The user reported the compass had been running for more than 30 minutes at
  release acceptance. This completes the manual soak observation; no continuous
  diagnostic capture or exact duration was supplied for that interval.
- Quantified quarter-turn accuracy/repeatability has not been recorded.

Generated diagnostic captures are kept in ignored `diagnostics/`. This milestone
does not implement a second display, button navigation, CAN, GPS, or tilt
compensation. Sensor access, heading math, calibration storage, and rendering are
separate modules to support those later changes.
