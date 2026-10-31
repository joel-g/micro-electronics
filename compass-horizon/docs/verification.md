# Compass & Horizon verification

## v1.1.0 long-press and theme release — 2026-10-30

- Native button tests pass for 40 ms debounce, short actions only on release,
  the 1999/2000 ms boundary, one long event per hold, suppression of the release
  after a long press, held-at-startup, timer rollover, and Compass/Horizon actions.
- Re-centering reuses the existing `IMU::zero()` method. The MPU driver/filter
  source is unchanged; only its current filtered angles are captured as offsets.
- Button handling runs without waits. `LEVEL SET` is scheduled for 1.5 seconds
  without suspending sensor updates. Stale data cannot produce a success banner.
- The initial re-centering build passed both default/debug builds and was flashed
  with successful verification. The user confirmed Horizon re-centering with
  `LEVEL SET`, suppression of a face switch on release, and continued short-press
  operation. Two logged button re-centers each reported pitch/roll `0.00`, with
  MPU initialization count still one. Both sensor streams stayed fresh with no
  errors or restarts; saved magnetic offsets remained unchanged.
- A Compass long press initially had no action in that capture; the later user
  request assigns it light/dark theme toggling. Updated button-policy tests pass
  for that action. Both final PlatformIO builds passed and the default theme
  firmware was flashed with esptool verification.
- The user confirmed that a Compass hold selects a readable white face, that
  white remains selected after visiting Horizon and returning, and that another
  hold restores the black face without switching instruments on release.
- The theme capture records the light-theme event followed by switches to
  Horizon and back. Later reports show two theme changes, one successful
  Horizon re-center, and continued button switching. All captured periodic
  reports show both sensors fresh, zero sensor errors, and one initialization
  each; saved magnetic offsets are unchanged. There is a serial reporting gap
  (128 dropped diagnostic messages), so this is not a continuous soak test.
  Physical theme appearance and release behavior were confirmed by the user.
- Local logs: `diagnostics/long-press.log` and `diagnostics/compass-theme.log`.
  Theme and attitude zero references remain RAM-only. The v1.0.0 outstanding
  hardware checks below are still outstanding for this release.

### Final v1.1.0 release checks

- Default and debug PlatformIO builds passed after removing the development
  suffix. Static RAM: 139,800 bytes; application: 427,150 bytes default and
  427,990 bytes debug.
- Button, compass math, and horizon geometry native tests passed again;
  geometry covered 285,753,600 pixel classifications.
- The installed, user-tested firmware is the preceding `1.1.0-dev` build.
  The release changes only its startup version string and documentation; the
  final version-labelled binaries have been built, not reflashed.
- Release authorized by the user on 2026-10-30. Standalone project files are
  unchanged. Previously recorded hardware limitations remain applicable.

Release application binary SHA-256:

```text
esp32-s3-devkitc-1:
853bb141f9476c3694e2a62194f6356d10d68ae185b4a922997b92da6c13565a
debug:
81c8cc489151f8ae2fb739e45e39ee16e8c6b44d150112da1055d391e123684b
```

## v1.0.0 release baseline

Hardware test baseline: development v0.1.0, 2026-10-30. Source projects are
copied/adapted from repository commit `a3829ab`; neither source project is modified
by this milestone. The v1.0.0 release changes only the firmware version banner
and adds release documentation and a dedicated BOM. The user explicitly approved
publication on 2026-10-30 with the outstanding checks recorded here accepted as
release limitations; those checks are not represented as passed.

## Completed host checks

- The v1.0.0 default and debug builds passed. Button, compass math, and
  horizon geometry checks were rerun and passed. The release image has not been
  flashed; the installed firmware is the functionally identical v0.1.0 baseline
  described in the hardware results below.
- PlatformIO default build passed: static RAM 139,776 / 327,680 bytes;
  application 425,210 / 1,310,720 bytes.
- Button state-machine checks passed: bounce suppression, one event while held,
  release/rearm, held-at-startup, and unsigned timer rollover.
- Reused compass math checks passed: headings, wraparound, smoothing, calibration
  recovery, and rejection of insufficient calibration data.
- Reused horizon geometry test passed 285,753,600 pixel classifications.
- Preflight standalone compass capture loaded saved offset
  `1654.5,-21.5,1050.5`, with fresh samples and no sensor errors.
- Optional `debug` build passed without flashing it. Default firmware remains
  installed, with the horizon debug overlay hidden.

Application binary SHA-256 identifiers for this test session:

```text
Default (flashed):
5948ada3f3cf4bfbdf4227cd3dad86a96ef29c69d8e0e2f28860aea2f3e83d7b
Debug (build only):
e0eaf84758f087f978fe81255bc0e2687ca910cbe55f9a2be0b79ced58ddddad
```

v1.0.0 release build identifiers (build checks only):

```text
Default:
186b6b26e7fb002505d83c9f9e6955c19f34c2a120ae893824a62eca6de02baf
Debug:
52fc886735e4afbd400fa67ee7da7aff10c68b689a9a5d7ba6e40807cf8aa7d1
```

## Hardware checks

- Default combined firmware uploaded successfully; esptool verified the flash.
- Both sensor addresses detected and both streams live. Previously saved compass
  offsets loaded unchanged, with `cal=1 saved=1`.
- User confirmed both faces respond, repeated physical presses switch faces, and
  a three-second hold switches only once. The captured sequence contains ten
  physical press events and ten matching switches, with no sensor reinitialization
  or errors. Subsequent extra presses were also visible before the automated test.
- A 59.011-second captured interval measured 97.05 MPU samples/s and 49.08
  magnetometer samples/s. Both streams remained fresh; free heap was 225,972 bytes
  throughout. Maximum loop time was 9.366 ms and MPU sample gap 19.094 ms.
- The automated test passed 20 regular plus 10 rapid switches without new sensor
  errors, resets, or reinitialization. Sampling averaged 94.7 Hz MPU / 49.1 Hz
  magnetometer. Free heap changed from 225,884 to 225,736 bytes over that test;
  this short test does not establish long-term memory behavior.
- Saved Horizon selection survived a commanded software restart; the test then
  selected and saved Compass. Both sensors resumed fresh readings and the same
  magnetic offsets. MPU startup zero was re-established, as designed.
- Magnetometer fault independence passed in the serial capture: eight reports
  showed magnetometer data unavailable while MPU data remained fresh, with MPU
  initialization count unchanged. Magnetometer sampling recovered on reconnection;
  saved offsets were preserved and no ESP32 reset occurred.
- The user reported soldered gyro connections; no desoldering was requested.
  The final capture nevertheless contains an MPU detection failure, five read
  errors, eleven initialization attempts, and a maximum sample gap of 9.160 s,
  followed by healthy MPU readings without an ESP32 reset. Magnetometer error
  and initialization counts did not increase. Most of the outage fell between
  captures, so its full timeline and physical cause were not recorded. Complete
  MPU-fault independence and fault-screen appearance remain unverified.

These checks apply to this combined firmware; the source projects' earlier tests
are not treated as proof of its behavior. Cold power-cycle mode persistence,
an extended soak, and deliberate watchdog injection in this combined application
have not been performed. Serial completion counters alone cannot prove display
appearance; the physical face/hold observations above came from the user.

Raw serial logs are local files under ignored `diagnostics/`:
`preflight-compass.log`, `physical-button.log`, `switch-test.log`, and
`sensor-faults.log`, and `mpu-fault.log` (partial outage/recovery evidence and
final health; not a continuously captured MPU-disconnection test).
