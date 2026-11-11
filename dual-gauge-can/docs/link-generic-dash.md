# Link Generic Dash bring-up

## Identified ECU and prepared settings

On November 8, 2026 the user read **G4X Monsoon, maincode firmware 6.22.3**
from PCLink. This supersedes the earlier G4+ assumption. A G5 PCLink icon does
not identify a G5 ECU: Link distributes a shared G5/G4X application.

The photographed setup is CAN1, User Defined, 500 kbit/s, OBD off;
Channel 1 Receive User Stream 1 remains unchanged. Channel 2 is prepared as
Transmit Generic Dash, Normal (11-bit), ID 1000 decimal / 0x3E8, 20 Hz.
The screenshot alone did not establish Apply/Store, but subsequent live capture
confirmed this stream active. Whether it is stored across ECU power cycles is
still unconfirmed.

## Decoder contract

`LinkGenericDashDecoder` accepts only the JSON-configured ID/format (shipped:
standard ID 1000), DLC exactly
8, reserved byte 1 zero. Byte 0 selects the subframe; words are unsigned 16-bit
little-endian. Generic Dash 2 is a different layout and is not accepted by design.

| Subframe byte 0 | Payload bytes (zero-based) | Channel | Conversion |
| --- | --- | --- | --- |
| 0 | 2–3 | Engine speed | RPM = raw |
| 0 | 4–5 | Absolute manifold pressure | kPa = raw |
| 1 | 2–3 | Barometric pressure | kPa = raw / 10 |
| 2 | 6–7 | Coolant | °C = raw − 50 |
| 3 | 2–3 | Intake air temperature | °C = raw − 50 |
| 3 | 4–5 | ECU supply voltage | V = raw / 100 |
| 6 | 4–5 | Lambda 1 (opt-in) | lambda = raw / 1000 |
| 7 | 6–7 | Fuel pressure (opt-in) | kPa = raw |
| 8 | 4–5 | Oil pressure (opt-in) | kPa = raw |

RPM 0 is a valid stopped engine. Values outside documented ranges invalidate
only the affected channel. Zero barometer or ECU voltage is also rejected for
this powered-ECU use. Range checks cannot detect every sensor fault or plausible
ECU fallback value; compare against PCLink and ECU fault status. Each channel
expires independently after the configured timeout (default 1500 ms). Boost requires fresh MAP and barometer,
subtracts the latter, then converts kPa to PSI. Coolant and IAT display °F. IAT uses the documented
−20 to 205 °C range; out-of-range values show INVALID DATA rather than a
plausible number. Generic Dash represents these temperatures in whole °C.
IAT follows Battery in the Display 2 / GPIO4 cycle.

Oil pressure is disabled pending confirmation of an installed/configured source.
Fuel pressure is disabled because the user reports no sensor. Lambda is disabled
while the AEM is disconnected. Fan bits have not been defined in this stream.
These three faces are omitted from the shipped manifest and their fields are
not decoded. Adding `oil`, `fuel` or `lambda` enables its defined field; verify
that source in PCLink first. Fuel pressure is the stream’s Fuel Pressure field,
not a computed differential-pressure channel. `fans` is rejected until a mapping
exists. See [configuration](configuration.md).

The implementation is based on Link's published baseline Generic Dash layout
and Link staff's statement that later generations preserve it and add frames.
Live capture on this G4X 6.22.3 now matches the expected 14-subframe layout and
produces readings. User comparison confirms MAP 92 kPa and BAP 100.9 kPa
agree with PCLink; coolant and battery numerical comparisons remain pending.
The stopped-engine MAP/BAP difference is present in the ECU values, not introduced
by the display conversion. Physical sensor accuracy remains unverified.

## Receiving and acknowledging

The shipped JSON selects `can.mode: "normal_rx"` for default, debug and vehicle
builds, so this node acknowledges valid incoming messages. The `listen-only`
environment selects `config/examples/listen-only.json` instead. Normal mode also permits controller error frames. It
is **not electrically passive**. TX queue length remains zero and the project
has no application transmit calls: no commands, requests or simulated data.
The driver retries in the selected build mode; it never changes mode automatically.

This matters for the user's two-device arrangement (Link and ESP32 transceiver,
12-inch twisted pair, ground reference, AEM unplugged). Listen-only cannot provide
the ECU an acknowledging peer. The initial repeated ID 0 traffic was consistent
with missing acknowledgment. After the approved vehicle flash and activation of
Generic Dash, ID 1000 arrives at about 280 frames/s with no reported reception
errors in the one-minute capture. Both variables changed, so this does not
isolate the original ID 0 cause.

Build and upload the shipped configuration with
`pio run -e vehicle -t upload --upload-port YOUR_PORT`.
The normal partition layout and NVS calibration are retained. To select passive
behavior, set `can.mode` to `listen_only` or use
`pio run -e listen-only -t upload --upload-port YOUR_PORT`. See
[build and upload](configuration.md#validate-build-and-flash) for port selection.
The vehicle/default environment names do not override JSON. The user approved
acknowledging mode for this two-device connection during bring-up.

The RX queue is 128 frames. A service call consumes at most 64 frames, checking
a 750 µs processing budget between frames; no waiting for frames. This expands
the original 8-frame drain that dropped frames during the initial flood. Hardware
load testing is still needed; no lossless-recording claim is made.

`ENGINE` logs show decoded values or explicit UNAVAILABLE/INVALID/STALE and decode
counters. `j` prints observed IDs plus latest snapshots and counts for each seen
subframe 0–15 at the configured ID/format. Snapshot age is logged. They are sampled latest values,
not a lossless trace. This avoids relying on a 5 Hz serial sample to observe every
subframe of a 20 Hz stream. Unknown extra subframes do not update known channels.

## Verification sequence

1. Confirm the acknowledging build is agreed and flashed, engine stopped.
2. Apply only the prepared Channel 2 settings; preserve Channel 1 and bitrate.
3. Capture raw frames, per-subframe snapshots, CAN counters and decoded values.
4. Compare RPM (0), coolant, IAT, battery, MAP and barometer against PCLink runtime
   values. Inspect ECU CAN errors and gauge behavior. Do not enable absent sources.
5. Verify stale behavior when ECU transmission stops and recovery when it resumes.
6. Store ECU settings only after reviewing the successful test; record whether
   Apply/Store was actually performed. Termination remains unmeasured until the
   user supplies an unpowered resistance reading.

## Sources

- [Link-authored G4+ help, Generic Dash table (third-party mirror)](https://www.akao.co.uk/manuals/Link%20G4%2B%20Manual/device_specific_can_informatio.htm).
- [Link staff: Generic Dash compatibility across generations](https://forums.linkecu.com/topic/26949-pclink-can-setup-confusion-multiple-can-ids/).
- [Link PCLink generation compatibility](https://linkecu.com/pages/pc-link).
- [Espressif TWAI operating modes](https://docs.espressif.com/projects/esp-idf/en/v5.5-beta1/esp32s3/api-reference/peripherals/twai.html#operating-modes).
