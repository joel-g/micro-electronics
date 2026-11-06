# Dual Gauge with Buttons verification — 2026-11-05

New independent project `dual-gauge-with-buttons`, release version `1.0.0`. The four
existing project directories are preserved byte-for-byte, including the working
uncommitted `dual-gauge` prototype. Its previous hardware tests are a baseline,
not proof of this button milestone.

## Development host checks

- Default and debug PlatformIO builds passed without compiler warnings.
- Static RAM: 255,480 bytes. Application size: 431,538 bytes default;
  431,706 bytes debug. No new framebuffers or PSRAM are used.
- Button tests passed: debounce, 1999/2000 ms classification, one-shot holds,
  release suppression, startup-held, rollover, and simultaneous independent buttons.
- Display settings tests passed: defaults, independent keys, reload after a
  simulated restart, toggle coalescing, bounded save retries, and isolation from
  the calibration namespace.
- Calibration tests passed using actual orchestration, validation math, and
  storage code with simulated readings/NVS: full 60-second auto-save/reload,
  duplicate-start/early-finish guards, stationary and invalid-data rejection,
  retained prior offsets, storage write failure, feedback expiry, and the existing
  serial RAM-first workflow.
- Display transport tests passed, including replacing a partly transmitted frame
  without affecting the other display or leaving stale pixels.
- Compass math and horizon geometry tests passed (285,753,600 pixel classifications).
- MPU filter/driver, QMC5883P register driver, compass math, horizon geometry,
  and the display transfer implementation are reused; shared reset and SPI
  transaction handling are unchanged. GPIO5/6 and CAN remain unused.

## Hardware checks

- Preflight capture confirmed fresh readings from both sensors and saved offsets
  `1654.5,-21.5,1050.5`, with no sensor read errors.
- Default firmware flashed successfully on `/dev/cu.usbmodem1101`; esptool verified it.
- A 179.076-second continuous post-flash interval passed the capture analyzer.
  Both sensors remained fresh, with zero read errors/overflows and one
  initialization each. MPU averaged 96.077 samples/s; magnetometer 48.923/s.
  Heap remained exactly 110,292 bytes; maximum loop work was 9.326 ms and maximum
  MPU sample gap 18.009 ms. Saved compass offsets remained unchanged.
- The initial capture reports the intended first-start defaults: Dark and Standard.
- The user subsequently confirmed physical GPIO4 short presses switch Dark/Light,
  GPIO13 short presses switch Standard/Numerical with readable signed angles,
  and a GPIO13 hold re-centers with LEVEL SET without changing mode on release.
  Light and Numerical were left selected.
- A commanded software restart restored Light and Numerical, with fresh sensor
  streams and the original saved magnetic offsets. The reboot reported `reset=3`,
  matching the commanded software reset. No extra sensor retries or read errors
  appeared in the restart capture (`diagnostics/restart-modes.log`).
- Physical GPIO4 long-press calibration completed the full 60-second window,
  collecting 2,939 samples and automatically saving offsets
  `860.0,-383.5,1629.5`. Serial confirmation: `CAL applied saved=1`.
  Sixty periodic reports during collection show both sensors fresh, zero read
  errors/overflows, and one sensor initialization each. Horizon changed-frame
  count advanced from 655 to 1,510 while the compass displayed the countdown.
  Light/Numerical remained selected and the compass short-press count did not
  increment when the long press was released.
- The user confirmed CALIBRATION SAVED appeared, the compass returned to Light,
  both screens remained responsive, and level heading/full-circle wraparound
  checks passed.
- A subsequent USB power disconnect/reconnect restored Light, Numerical, and
  CAL SAVED, confirmed visually by the user. `diagnostics/cold-start.log` also
  reports `reset=1`, the new offsets `860.0,-383.5,1629.5`, `cal=1 saved=1`,
  fresh readings, zero sensor read errors/overflows, and one initialization each.
  Per-boot button and level-set counters returned to zero as expected.
- Local logs: `diagnostics/preflight.log`, `diagnostics/button-checks.log`, and
  `diagnostics/button-checks-summary.json`. Two queued startup reports and a
  20.011-second host-attachment gap were excluded from the measured interval.

Tested development application binary SHA-256:

```text
Default (flashed):
7d9c795dc553257f258e17d9f13a878c99575c633c550da9479eb1c263f3c192
Debug (build only):
e0f9574ab7a4980fa4b4562bd74f65f33a9df4ae8ecdc28e4fcd4acce78befef
```

## Final v1.0.0 release checks

- Default and debug release builds passed without warnings after changing only
  the startup version string from the tested development firmware.
- Release static RAM: 255,480 bytes; application: 431,522 bytes default and
  431,706 bytes debug. Default v1.0.0 was uploaded with esptool flash verification.
- The release startup capture retained Light/Numerical and saved magnetic offsets
  `860.0,-383.5,1629.5`, with both sensors fresh, zero read errors/overflows, and
  one initialization each (`diagnostics/v1-startup.log`).
- README controls/calibration/persistence, exact wiring diagram, BOM, and test
  results are included. The repository index links to this release.
- The earlier project directories and separate enclosure work are outside this
  release commit. Build outputs and raw diagnostic captures remain ignored.

Release application SHA-256:

```text
esp32-s3-devkitc-1:
d323971b3ac123fa48c8abe3a7a092ad208d09e8c34f6bbf221f192d45115477
debug:
e74628447038127f62da141a61aabc140c75d45aa7e03a5e5cb5c70f177eddcd
```

## Limits

The implemented control, valid auto-calibration, and persistence paths passed
the physical checks above. Invalid/stationary calibration rejection and NVS write
failure were exercised with host fakes, not deliberately induced on the board.
Simultaneous independent holds were checked by the host button test. A longer
soak, sensor unplug/recovery, and deliberate watchdog injection have not been
performed on this button build. The earlier project's ten-minute dual-screen run
is not substituted for those tests. Release v1.0.0 was authorized by the user on 2026-11-05 with these test limits documented.
