# Dual Gauge CAN verification — bench and vehicle bring-up

Release `1.0.0`, independent `dual-gauge-can` sibling based on released
`dual-gauge-with-buttons` v1.0.0. Earlier project directories and enclosure work
are preserved. The November 5 bench record is retained below. November 8
vehicle reception, firmware changes and outstanding comparisons follow it.

## November 5 bench scope

This initial stage used a disconnected CAN bus; it did not verify ECU reception
or engine decoding.

## Host checks

- Default and debug PlatformIO builds pass without warnings. Static RAM 256,280
  bytes; application 444,558 bytes default / 444,726 bytes debug; PSRAM disabled.
- Engine model/conversion tests pass: never-received versus valid zero,
  stale/invalid data, timer rollover, Fahrenheit/PSI conversion, fresh absolute
  MAP plus barometer required for boost, fan state validation, and an unknown
  Link decoder that never populates engine values.
- Settings tests pass: independently saved stable gauge IDs, invalid/removed-ID
  fallbacks, five-gauge wraparound, inherited read-only presentation preferences,
  coalesced writes, save retries, and unchanged original NVS namespaces.
- CAN receiver transport tests pass: listen-only/zero TX queue, initialization
  retry, disconnected receive timeouts, bounded frame flood processing, sampled
  logging, malformed DLC rejection, alerts, and listen-only restart after bus-off.
  The fake has no transmit function; production receiver compiles without one.
  Simulated received frames are test fixtures, not physical bus reception.
- Existing button, tile-transfer/partial-frame replacement, compass math,
  horizon geometry (285,753,600 classifications), and calibration workflow tests
  pass. Sensor drivers, fusion, magnetic calibration math/storage, shared display
  reset and SPI transfer implementation are preserved.

## Physical checks

- Default firmware uploaded and flash hash verified.
- User confirms both buttons cycle only their own display, every CAN face shows
  NO CAN DATA, local instruments resume and respond on return, and a Horizon
  hold sets level without cycling on release.
- The disconnected-bus capture completed a 299.560-second continuous interval,
  with 300 periodic DUAL reports and 301 CAN summaries including startup. MPU
  averaged 89.792 samples/s; magnetometer 49.055/s. Both remained fresh with zero
  read errors, no overflow, and one initialization each.
- The user exercised 15 Display 1 and 20 Display 2 short actions. Logged mode
  changes cover all five faces on each screen. The GPIO13 hold reported
  `LEVEL ok=1 pitch=0.00 roll=0.00`, preserving the mode and initialization count.
- CAN driver initialization succeeded in LISTEN_ONLY at the explicitly unconfirmed
  500000 bench bitrate. Every CAN summary reported zero received frames/IDs,
  zero driver/bus errors and no queue loss. This verifies disconnected operation,
  **not CAN reception**.
- One startup `TWAI_ALERT_ERR_PASS` (`0x1000`) was observed with TEC=0 / REC=0;
  subsequent summaries reported no alerts. Its cause is unconfirmed and it is
  retained in the record for the next bring-up, not suppressed or declared a bus
  communication test. No bus-off or unexpected MCU reset occurred.
- Free heap ranged 108,072–107,880 bytes during initial use, then remained exactly
  107,880 for the final 120 seconds. The initial settings/cache activity is not
  treated as proof of a leak or as proof of indefinite memory stability.
- Local ignored logs: `diagnostics/can-bench.log` and
  `diagnostics/can-bench-summary.json`. A host-attachment gap/queued prefix was
  excluded from the continuous interval.
- User selected Coolant/Fuel pressure and confirmed both returned after unplugging
  and reconnecting USB. The cold-start capture confirms those independent modes,
  `reset=1`, inherited Light/Numerical presentation, and unchanged saved offsets
  `860.0,-383.5,1629.5`. Both sensors were fresh with zero read errors and one
  initialization each; engine values remained unavailable.
- The cold start also showed one initial error-passive CAN alert with TEC/REC=0,
  followed by no alerts, and one startup magnetic overflow followed by valid fresh
  readings. These startup transients are recorded rather than omitted. No CAN
  frames or bus/driver errors were received/reported. Cold-start log:
  `diagnostics/cold-start.log`; the pre-power-cycle selections are in
  `diagnostics/final-status.log`.

## Hardware identification and remaining work

Waveshare module inspection is recorded in the BOM and vehicle procedure. Its
actual RS wiring and termination are not yet verified. CANH/CANL were
unconnected during the bench tests above; see the November 8 vehicle capture below.
The configured 500000 bitrate must still be checked against PCLink settings.
No ECU configuration has been modified, and no CAN IDs/layouts are implemented.

At the end of bench testing, real frame reception, transceiver physical signaling,
vehicle wiring/ground, correct bitrate and termination, and actual Link channel
decoding remained future checks. Host injected-frame tests do not substitute for them. Longer vehicle/bus
load tests, fault injection, and deliberate watchdog injection are not claimed.

## Original bench application identifiers

```text
Default (flashed):
77755f8af122a024d0622b251756ad92cd9c411157abe7be4bdd96f3bb6d30b0
Debug (build only):
e42596a7332c465debb4d67cc98689d40c536d06d248dd62fe027ff7e26774d2
```

## First vehicle capture — November 8, 2026

User connected the module directly to the Link using approximately 12 inches of
twisted pair and a ground reference, replacing the unplugged AEM lambda CAN
connection. User reports no fuel-pressure sensor fitted. ECU ignition/engine
state, remaining CAN devices, PCLink stream configuration and measured termination
were not yet confirmed when this capture was taken.

Passive USB capture (no reset, firmware upload, ECU configuration change or CAN
transmission) is saved locally in ignored
`diagnostics/vehicle-2026-11-08-initial.log`. The `j` serial command requested
observed IDs. Capture includes buffered startup output; the continuous live
interval runs from device uptime 185630 to 209644 ms (24.014 seconds).

- Receiver configured at 500000 bit/s, listen-only, accept-all filter.
- Received count increased from 652147 to 736712: 84565 frames, approximately
  3521 frames/s. Only one ID was observed by the receiver: standard `0x000`.
- All 131 raw serial samples (including buffered output) have DLC 8, RTR false,
  payload `01 00 00 00 00 00 00 00`. Serial samples are rate limited; this is not
  a lossless payload trace.
- RX queue-full alert `0x800`; missed count increased from 939 to 1530 (591).
  Hardware FIFO overrun, bus-error count, TEC and REC all remained zero in
  reported status. These receiver counters do not establish the ECU's error state.
- Both local sensors remained fresh with zero I2C read errors. Free heap remained
  108104 bytes during the live interval; recorded maximum loop duration 10.363 ms.
- `decoder=UNCONFIGURED`: the firmware intentionally maps no raw frames to engine
  channels yet. Receiving traffic alone cannot populate the CAN gauge faces.

Working hypothesis: with the AEM removed, the ECU may have no acknowledging
peer and may repeatedly retransmit an unacknowledged frame. Listen-only does
not acknowledge frames, as documented by
[Espressif](https://docs.espressif.com/projects/esp-idf/en/v5.5-beta1/esp32s3/api-reference/peripherals/twai.html#operating-modes).
This hypothesis requires confirmation from actual topology/ECU diagnostics or a
controlled comparison with an acknowledging peer. ID `0x000` and its payload
have not been assigned a meaning. No engine measurements have been decoded.

Next: confirm PCLink bitrate, transmit streams/IDs and CAN errors; establish
acknowledgment and termination for the direct two-device arrangement; then
capture the defined stream and implement only supported channels. Fuel pressure
must remain unavailable without an actual source. The RX queue drops also need
resolution/validation before claiming lossless reception under heavy traffic.

## Generic Dash implementation — November 8, 2026

User subsequently confirmed G4X Monsoon maincode firmware 6.22.3, superseding
our earlier G4+ identification. PCLink screenshot confirms 500 kbit/s; Channel 2
prepared as Generic Dash, Normal ID 1000, 20 Hz. Apply/Store is not yet confirmed.

Ten native test executables passed, including documented decoder values,
negative coolant, byte order, independent channel expiry, invalid ranges,
malformed frames, absent sources, and both explicit CAN modes with zero TX queue
and mode-preserving retries. Queue drains are tested against separate frame and
time budgets; multiplexed snapshots are checked. Existing sensor, display,
settings and button tests passed as part of the same suite.

Default, debug and vehicle PlatformIO builds passed without warnings. Static RAM
is 256552 bytes in each; default and vehicle flash usage is 446978 bytes.
The vehicle variant enables ACK/error frames, with no application TX API calls.
No new firmware has been flashed at this point; live decoded values and heavy-bus
queue improvements remain unverified. User confirmation of the change from the
original listen-only requirement is pending.

Build SHA-256 values:

```text
esp32-s3-devkitc-1: c03dee5c19dfa60a835b2fa72df0e6e6b92a2055c6b33798147aa0d096a73fa3
debug: 628cc965b623a4fbe962e1b7a92f8b64eb6fe2fd74fa057a04a17bf923dd964e
vehicle: 81d0312a1d3d766e11467922b14b4867635511ad18cf14163364c561c64de9cb
```

### First acknowledging-build hardware result

User explicitly approved flashing the acknowledging vehicle firmware after the
listen-only difference was explained. PlatformIO upload succeeded and esptool
verified the flash hash. Capture:
`diagnostics/vehicle-2026-11-08-ack-baseline.log`.

Following upload, CAN STATUS reports NORMAL_RX at 500000. Actual standard ID
0x3E8 frames arrive at approximately 280/s. Snapshots show all 14 multiplexors
0 through 13, consistent with 20 sets/s. No ID 0 appeared in this capture. Both
ACK behavior and the ECU stream configuration differ from the earlier capture,
so disappearance of the original flood does not by itself isolate its cause.
Although the screenshot was described as prepared, the capture establishes that
the Generic Dash stream is now active; whether it was stored in ECU nonvolatile
memory remains unconfirmed.

Initial decoded readings: RPM 0; coolant 14 °C / 57.2 °F; ECU supply roughly
11.81–11.85 V; MAP 91–92 kPa; BAP 100.9 kPa. The stopped-engine MAP/BAP difference
is being checked against PCLink rather than corrected or calibrated by software.
All observed CAN statuses show zero missed frames, FIFO overruns, bus errors,
TEC/REC and driver errors, with no alerts. Decode malformed/unknown/invalid
counters are zero. Unsupported sensor channels remain unavailable. These are
initial reception results; PCLink/display comparisons and stale/recovery testing
are still pending.

### Follow-up capture and display confirmation

`diagnostics/vehicle-2026-11-08-generic-dash.log` contains a 59.012-second
continuous interval after excluding queued pre-attachment output. The receiver
reports about 280 frames/s, with zero queue misses, FIFO overruns, bus/driver
errors, TEC/REC or alerts throughout reported statuses. Decoder errors remain
zero. Both sensors remained fresh with zero read errors; maximum recorded loop
8.442 ms. Free heap ranged 106596–106744 bytes while modes/settings changed;
this short capture is not an extended leak test.

The user confirms coolant, battery and boost are now visible on the displays.
This confirms working rendering/reception, not yet numerical agreement with
PCLink. Final logged values: RPM 0, coolant 14 °C, voltage 11.80 V, MAP 91 kPa,
BAP 100.9 kPa. Derived boost is approximately -1.44 PSI. The stopped-engine
pressure difference needs PCLink comparison; no calibration/offset correction
was made. Stale/recovery and ECU stored-setting restart checks remain pending.
All changes are local to dual-gauge-can; earlier projects are unchanged. Nothing
from this milestone has been committed or pushed.

### PCLink pressure comparison — November 8, 2026

User read MAP 92 kPa and BAP 100.9 kPa from PCLink Runtime Values with ignition
on and engine stopped. Both agree with the observed CAN readings (MAP fluctuates
91–92 kPa). At MAP 92 kPa, (92 - 100.9) / 6.894757 = approximately -1.29 PSI.
This verifies the pressure decoder/scaling against the ECU and explains the
negative boost display. It does not validate physical sensor accuracy or explain
why the two pressures differ with the engine stopped. No ECU calibration or gauge
offset was changed. Coolant/battery visibility is confirmed; their numerical
comparison with PCLink, dynamic RPM/boost, stale/recovery and stored ECU settings
across restart remain unverified.

### Temperature comparison — November 8, 2026

User reports PCLink IAT 59 and ECT 58, with units not explicitly restated;
Fahrenheit is inferred from context. A fresh six-second serial capture is saved
as `diagnostics/vehicle-2026-11-08-temperature-check.log`. ECT remains 14 °C
(57.2 °F); raw IAT in the prior subframe-3 snapshot is 65, corresponding to 15 °C
(59 °F). The small ECT difference is consistent with the Generic Dash whole-degree
Celsius encoding and display rounding, but exact agreement is not claimed. IAT
is a diagnostic raw-frame comparison only, not an added gauge/channel.

## IAT gauge addition and graphics pass — November 8, 2026

IAT is now decoded from Generic Dash subframe 3, bytes 2–3, unsigned little endian,
raw minus 50 °C. It is Gauge ID 10, appended after Battery on Display 2. Existing
saved IDs are unchanged. Default/debug/vehicle builds and native tests passed;
IAT firmware was flashed and verified. `diagnostics/vehicle-2026-11-08-iat.log`
shows IAT 15 °C (59 °F), ECT 14 °C, about 280 CAN frames/s, no CAN errors/losses.

User then requested colored engine graphics and a black compass. User approved
coolant bands: blue below 60 °C, green 60 to below 100 °C, orange 100 to below
110 °C, red at 110 °C and above. Thresholds are configurable display settings,
not ECU settings. The renderer adds a segmented arc, white pointer, larger
readings and text status; stale/missing/invalid states suppress the live scale.
IAT/battery use cyan, vacuum blue. Other gauge ranges are visual scales only.

Eleven native executables passed, including exact coolant boundary colors and
labels, clamped pointer ranges, decoder IAT/endian/range/unit checks and IAT NVS
selection/reload. A host preview uses the actual pinned Adafruit GFX rasterizer
and production renderer. All coolant states, IAT, battery, vacuum, unavailable,
black compass (even with legacy light=true), stale and invalid faces rendered.
Unchanged-frame suppression and palette indices are asserted. Preview images
were visually inspected for fit and legibility, including the stale face.

The old projects and their saved presentation preferences remain untouched.

### Graphics firmware hardware capture

All three final PlatformIO environments built without warnings; vehicle firmware
upload succeeded with esptool hash verification. Static RAM: 256584 bytes;
vehicle application: 453238 bytes. Capture:
`diagnostics/vehicle-2026-11-08-graphics.log` and its JSON summary.

During 34.024 seconds of continuous reports, CAN stayed at approximately 280
frames/s with zero receive misses/overruns, bus/driver errors, TEC/REC or alerts.
Both sensors stayed fresh with zero read errors. Free heap was constant at
106844 bytes across that interval. Maximum recorded loop duration was 13.578 ms;
maximum draw times were 7.886 / 7.570 ms. No additional framebuffer was allocated;
existing serialized changed-tile transfers are retained.

The engine was running during this capture (observed RPM 835–901). Final values
were RPM 849, coolant 73 °C, IAT 23 °C, battery 13.38 V, MAP 43 kPa, BAP 100.9 kPa.
These dynamic values establish incoming changes, not a complete engine-running
comparison against PCLink. Coolant correctly belongs in the configured green
NORMAL band at this temperature. Theme logs report dark, and button-driven mode
changes include Compass, IAT, Coolant, RPM and Boost. User subsequently confirmed: all faces look good and remain responsive,
including the coolant colors, black compass and IAT after Battery. Host previews
were also inspected independently. This graphics pass is hardware-tested;
extended vehicle soak, transmission-loss recovery and ECU Store/restart checks
remain separate outstanding verification items.

Final graphics firmware SHA-256:

```text
esp32-s3-devkitc-1: 908dd01d80e5fd9318cf438096b924dffc286244fb87d547f33ce853bff1ca2b
debug: b1ab995cb6c09127e5108203547f2a228b3899e48d9cd70e9f4befdd02558ad9
vehicle: eda76ccca58d83bf23aeb8b389438e1e7a012c570c2dc1862d5c4329dedb486d
```


## JSON configuration pass — November 8, 2026

Configuration now comes from `config/instrument.json`, validated by a PlatformIO
pre-build script and compiled into constants. The shipped manifest is Display 1:
Horizon, Coolant, RPM, Boost; Display 2: Compass, Battery, IAT. Optional absent
sources no longer appear in either cycle. Stable saved IDs are retained; removed
selections fall back to the first configured face. JSON also selects the CAN
bitrate/ID/format/mode, units, theme, attitude presentation, scales, stale timeout
and coolant thresholds. ECU bus/channel/rate metadata is recorded, not sent to
the ECU. Default/debug/vehicle now all use the shipped acknowledging JSON;
`listen-only` uses a separate passive example. See `configuration.md`.

Verification completed:

- Nine configuration tests passed, including duplicate/unknown keys, malformed
  JSON, invalid numbers/IDs/units/profiles, empty/duplicate/unsupported faces and
  deterministic generated headers.
- An alternate generated configuration compiled and executed with extended ID
  0x18FF1234, 250 kbit/s, passive mode, Celsius/kPa, light compass, standard
  horizon, AFR and optional oil/fuel/lambda fields enabled. It checked actual
  decoder values, receiver settings and removed saved-selection fallback.
- All eleven existing native executables passed with a fixed test fixture;
  user configuration edits do not change their expected baseline behavior.
- Default, debug, vehicle and listen-only PlatformIO builds passed without
  compiler warnings. Static RAM is 256616 bytes; vehicle flash is 454130 bytes,
  debug flash 454314 bytes. Existing tile transfers/framebuffers are retained.
- A temporary PlatformIO environment verified that JSON-only edits to ID, units
  and the face list regenerated the header and recompiled firmware without a
  clean build. An empty manifest then stopped the actual PlatformIO build with
  an explanatory error, before compilation. Main configuration was not edited
  for these integration checks.
- Updated production-renderer preview was inspected for legibility and copied
  to `docs/images/gauge-preview.png`.
- Baseline hashes confirmed all earlier project sources remained unchanged.

Build/test logs are kept locally under ignored `diagnostics/config-*.log`.
**This configuration revision has not been flashed or hardware-tested.** USB was
not connected; the user explicitly chose to leave the build ready for later.
The previously flashed graphics revision remains on the ESP32. For the next hardware validation,
flash the vehicle build and verify the shorter four-/three-face button cycles,
responsive local/CAN faces and retained selections/calibration after restart.

Ready-to-flash vehicle firmware SHA-256: `a934a14fd2ad347595bbd96747a6d1a65f41df3526bc60deec2c91520bc2cdef`.

## v1.0.0 publication — November 8, 2026

The user authorized publication with the recorded validation limits. Version
metadata and the serial startup banner are now 1.0.0. The final vehicle build
passed without compiler warnings; it was not uploaded. Firmware SHA-256:

`053f632cc4335beb39c9c2c133909c3305cea784ae7be361f62360fb8492baf9`
