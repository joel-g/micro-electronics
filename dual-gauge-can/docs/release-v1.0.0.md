# Dual Gauge CAN v1.0.0

Released November 8, 2026 as an independent PlatformIO project. Earlier projects
and enclosure work are unchanged.

## Included

- Two GC9A01 displays with independent GPIO13/GPIO4 buttons, saved face selections,
  bounded shared-SPI transfers, sensor recovery and serial diagnostics.
- MPU-6050 artificial horizon and calibrated QMC5883P magnetic compass.
- Link Generic Dash decoding for RPM, coolant, IAT, battery and MAP/barometer
  boost, with optional oil pressure, fuel pressure and Lambda 1.
- Colored engine scales, coolant temperature bands, black compass by default,
  and explicit missing/stale/invalid data states.
- Validated JSON configuration for ordered face manifests, CAN settings, units,
  presentation, scales and thresholds. No C++ edits needed for these settings.
- Wiring diagram, BOM, configuration guide, decoder reference and verification
  record. See the project README for build/upload commands.

The shipped manifest omits unavailable oil, fuel, lambda and fan faces. It uses
500 kbit/s, standard ID 1000 and acknowledging receive mode for the tested Link
G4X Monsoon 6.22.3 setup. It sends no application commands or data messages.
The separate listen-only example is available for passive monitoring.

## Validation and limitations

All four build environments and automated checks passed for the configuration
revision. Tests cover invalid JSON, alternate CAN/units settings, optional
channels, saved-selection fallback, sensor math, display transfers and input
handling. The final vehicle build was rebuilt with the v1.0.0 version string.

Earlier hardware tests confirmed independent controls, live Link data, responsive
local instruments and the new graphics. **The final JSON configuration revision
has not been flashed or hardware-tested**: the user chose to leave it ready for
later and authorized publication with that status documented. Next hardware
checks are the shorter four-/three-face cycles, retained selections/calibration,
and responsive local/CAN faces after restart.

Extended vehicle soak, ECU-transmission loss/recovery, stored ECU configuration
across power cycles and measured bus termination remain outstanding. Optional
pressure/lambda sources have host tests but are not vehicle-verified; enable
only sources actually installed and configured. Fan status, arbitrary user CAN
streams and Generic Dash 2 are unsupported. The compass has no tilt compensation.
See [verification](verification.md) for the precise test evidence and limits.
