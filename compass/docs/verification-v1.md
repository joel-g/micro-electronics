# Compass v1 verification record

Watchdog firmware under test: development v0.1.0, with opt-in fault injection.
The user accepted publication as v1.0.0 on 2026-10-30 after this test. The release
changes the version banner to v1.0.0; it does not change watchdog or compass
behavior. During publication, the user reported the compass had been running
for more than 30 minutes; the manual soak observation is recorded below.

## Watchdog recovery — 2026-10-30

### Setup

- ESP32-S3 SuperMini, QMC5883P at `0x2C`, one GC9A01 display; see the
  [BOM](BOM.md) and [wiring](../README.md#hardware-and-connections).
- MPU-6050 present on the bus but unused by compass firmware.
- USB serial `/dev/cu.usbmodem101`; passive serial control lines.
- Existing accepted calibration saved in NVS. The test neither recalibrates nor
  writes calibration; it compares offsets before and after restart and flashing.
- Five-second task watchdog registered on the main loop task.

### Procedure

Run from the `compass` directory with other serial monitors closed. Record the
current saved offsets from healthy diagnostics first:

```sh
~/.platformio/penv/bin/python tools/serial_capture.py --seconds 5 --log diagnostics/watchdog-preflight.log
pio run -e watchdog-test -t upload
~/.platformio/penv/bin/python tools/watchdog_test.py inject
# Always restore normal firmware, including if the test fails:
pio run -e esp32-s3-devkitc-1 -t upload
~/.platformio/penv/bin/python tools/watchdog_test.py production
```

The helper accepts `--offset=X,Y,Z` to compare against the exact offset string
captured before flashing. It also accepts `--port` and `--log` overrides.

The inject check requires healthy readings, stored calibration, `wdt=1`, and
`test=1` before sending `w` once. This command deliberately stops the main loop
from feeding the watchdog while yielding to other tasks. It does not issue a
software reset or toggle USB reset lines. Normal firmware does not compile this
command's handler or stall message.

### Acceptance criteria

- Restart caused by the task watchdog: `reset=6`, retained `prev=8`, uptime reset.
- First healthy post-reset report arrives within 3–15 seconds of the command.
- Saved calibration reloads with identical offsets: `cal=1 saved=1`.
- Fresh, valid magnetometer samples and completed display transfers resume.
- A further 12 seconds of diagnostics show increasing uptime and sample counts,
  no new sensor errors/overflows, and no repeated resets. Frame counts may stay
  unchanged when the screen is unchanged; serial transfer counts alone cannot
  establish that the physical display looks correct.
- Restored normal firmware reports `test=0`, ignores `w`, and passes another
  12-second health check with calibration preserved.
- The stall message is present only in the test binary, absent in the normal one.

### Results

**PASS. Normal firmware restored and verified.** Both PlatformIO environments
built successfully, and esptool verified the uploaded flash contents for each.

| Check | Observed result |
| --- | --- |
| Deliberate stall | Panic log identifies `loopTask (CPU 1)` as failing to feed the task watchdog |
| Restart cause | First healthy report: `reset=6 prev=8 wdt=1 test=1` |
| Recovery time | 7.23 seconds from sending `w` to receiving the first healthy report, including restart and sensor initialization |
| Calibration | Offsets `1654.5,-21.5,1050.5` unchanged from preflight through watchdog restart and restoration; `cal=1 saved=1` |
| Post-restart operation | Over the next 12.002 seconds, samples increased from 49 to 639 and completed changed frames from 3 to 26; no sensor errors, overflows, or further resets |
| Normal firmware | `test=0`; sending `w` caused no stall/reset; samples increased from 535 to 1,126 over 12.005 seconds, with fresh/valid readings and no errors/overflows |
| Heap | Free heap remained 225,920 bytes across the healthy post-restart and production reports |
| Binary isolation | Stall message present in the test image and absent from the normal image |

Preflight included historical error counters from the earlier physical wire
disconnection (`err=5`) and one earlier overflow. These counters were stable
during preflight. After the test upload, all baseline and post-restart sensor
error/overflow counters were zero.

Raw captures (local, ignored by Git): `diagnostics/watchdog-preflight.log`,
`diagnostics/watchdog-inject.log`, and `diagnostics/watchdog-production.log`.
The source changes were not yet committed at test time. Application binary
SHA-256 values identify the exact images tested:

```text
watchdog-test:
5e691ece58e653c79fa3c561cdf30f2425e26b0b519f7c6065e91bdcfea55852
esp32-s3-devkitc-1 (restored normal firmware):
08499c30e0b09e0b92da68cd3e13771699f6120bb0acaeeaf3e6ec1ffd8e9849
```

### Scope

This checks a yielding main-loop stall. It does not simulate every possible
interrupt, power, bus, or display hardware failure. Previously completed physical
checks are recorded in the [README verification section](../README.md#verification).

## Manual soak observation — 2026-10-30

The user reported the compass had been running for **more than 30 minutes** and
authorized publication as v1.0.0. This is accepted as the manual soak observation.
The exact duration, firmware image throughout that interval, serial-monitor
state, and any interruptions were not recorded. No continuous serial capture was
provided, so this observation does not establish zero resets/errors or quantify
memory stability over the full interval. The separately logged short checks
above provide the measured evidence for their stated durations.
