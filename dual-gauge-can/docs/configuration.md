# Instrument configuration

Edit [`config/instrument.json`](../config/instrument.json), build, then upload.
This is build-time configuration: there is no runtime JSON parser, filesystem
upload, network setup or extra framebuffer. PlatformIO validates the file before
compilation and writes a generated header under `.pio/build/<environment>/generated`.
Do not edit that generated header. Ordinary configuration changes need no C++ edits.

## Choose the available faces

The two `displays` arrays define exactly what each button cycles through, in order:

```json
"displays": {
  "1": ["horizon", "coolant", "rpm", "boost"],
  "2": ["compass", "battery", "iat"]
}
```

This fragment belongs inside the complete supplied JSON file. For an ECU-only
installation without an MPU or magnetometer, use, for example:

```json
"displays": {
  "1": ["coolant", "rpm", "boost"],
  "2": ["battery", "iat"]
}
```

Every screen needs at least one face; duplicates within one list are rejected.
A single face stays selected when its button is pressed. Both screens may show
the same face, and either local instrument may be assigned to either screen.
Local sensor drivers still run/retry for diagnostics even when their faces are
omitted; no sensor is required for the ECU faces to work.

| Face name | Required source |
| --- | --- |
| `horizon` | MPU-6050 on I²C |
| `compass` | QMC5883P on I²C, with magnetic calibration |
| `coolant` | Generic Dash engine coolant temperature |
| `iat` | Generic Dash intake air temperature |
| `rpm` | Generic Dash engine speed |
| `boost` | Generic Dash absolute MAP and barometric pressure |
| `battery` | Generic Dash ECU supply voltage |
| `oil` | Generic Dash oil pressure, explicitly enabled by adding this face |
| `fuel` | Generic Dash fuel pressure, explicitly enabled by adding this face |
| `lambda` | Generic Dash Lambda 1, explicitly enabled by adding this face |

Adding an optional face asserts that its source is installed and configured in
the ECU. The ECU may transmit plausible placeholders for missing sensors; the
firmware cannot reliably discover installed sensors from those bytes. Verify new
sources against PCLink. Fuel pressure is not differential fuel pressure. `fans`
is deliberately rejected: this profile does not yet define a fan-status mapping.
Selected faces still display NO CAN DATA, STALE or INVALID when data is missing,
expired or invalid; they are not silently removed during a transient fault.

Last-used selections remain stored by stable gauge ID, not array position.
Reordering preserves the selection. If a saved face is removed, that display
starts with the first entry in its new list. A new installation also starts with
the first entries. A normal reflash keeps these selections and magnetic
calibration with the existing partition layout. Relative attitude zero is RAM-only.

GPIO13 short press cycles Display 1; GPIO4 short press cycles Display 2. Existing
long-press behavior remains: GPIO13 re-centers only when Display 1 shows Horizon;
GPIO4 long press is unassigned, including if Horizon is placed on Display 2.

## Match PCLink

The shipped settings describe the tested G4X Monsoon, firmware 6.22.3:

| JSON property | Shipped value | Meaning |
| --- | --- | --- |
| `ecu.model`, `ecu.firmware` | Link G4X Monsoon / 6.22.3 | Descriptive record, not auto-detection |
| `ecu.profile` | `link_generic_dash` | Only supported decoder; select **Transmit Generic Dash** in PCLink |
| `ecu.can_bus` | `1` | Record of the ECU's CAN port; 1 or 2 |
| `ecu.transmit_channel` | `2` | Record of the PCLink channel; 1–10 |
| `ecu.transmit_rate_hz` | `20` | Declared stream-cycle rate; 1–100 Hz |
| `can.bitrate` | `500000` | Actual controller bitrate; 125000, 250000, 500000 or 1000000 |
| `can.id` | `1000` | Decimal receive ID; JSON does not support `0x` numbers |
| `can.format` | `standard` | `standard` (11-bit) or `extended` (29-bit), matching PCLink |
| `can.mode` | `normal_rx` | Acknowledging receive mode; alternative: `listen_only` |

The `ecu` bus/channel/rate fields are documentation and startup diagnostics;
those values are not embedded in each CAN frame and cannot be used as receive
filters. `can.id` and `can.format` select decoded traffic; the raw monitor still
observes all IDs. Changing this file does **not** configure PCLink or the ECU.
Retain other existing ECU channels and match the selected stream separately.
Generic Dash 2 and arbitrary user streams have different layouts and are not
supported by changing a name or ID. See [field mappings](link-generic-dash.md).

`normal_rx` sends controller ACK/error frames but no application messages or ECU
commands. It supplies the acknowledging peer needed in the tested Link + gauge
two-device connection. `listen_only` is passive and needs another acknowledging
peer for normal ECU transmission. No automatic mode or bitrate switching occurs.

## Appearance and freshness

| Property under `display` | Allowed values / meaning |
| --- | --- |
| `temperature_unit` | `F` or `C`; default `F` |
| `pressure_unit` | `psi` or `kPa`; default `psi` |
| `compass_theme` | `dark` or `light`; default `dark` |
| `horizon_style` | `standard` or `numerical`; default `numerical` |
| `stale_ms` | 100–60000 ms; default 1500, measured independently per channel |
| `coolant_c.warm` | Blue → green boundary; default 60°C |
| `coolant_c.high` | Green → orange boundary; default 100°C |
| `coolant_c.overheat` | Orange → red boundary; default 110°C |
| `lambda.unit` | `lambda` or `afr`; default `lambda` |
| `lambda.stoichiometric_afr` | AFR multiplier for the actual fuel; default 14.7 |

Coolant boundaries must be ascending. They are display warnings, not ECU control
settings. JSON presentation settings take precedence over old `dual-buttons`
preferences; the old namespace is left untouched. Buttons do not change themes.

`scales` defines each engine face's `[minimum, maximum]` arc range. Keep all scale
entries, even for omitted faces. Values use **canonical units**, regardless of
the display unit selection:

- `coolant`, `iat`: °C.
- `boost`: kPa relative to actual barometric pressure, allowing negative vacuum.
- `oil`, `fuel`: kPa, using the ECU's transmitted pressure reference.
- `rpm`: revolutions per minute; `battery`: volts; `lambda`: lambda ratio.

The build converts scales and thresholds to the selected display units. AFR
scales multiply the lambda range by the configured stoichiometric ratio. A value
outside its arc range clamps the pointer only; its numeric value remains intact.

## Validate, build and flash

Run from the `dual-gauge-can` directory. Replace `YOUR_PORT` with the ESP32
port shown by `pio device list`. See the
[repository setup guide](../../README.md#build-and-upload) for PlatformIO setup.

```sh
python3 tools/configure.py             # validate without building
pio run -e vehicle                    # validate, generate constants, compile
pio run -e vehicle -t upload --upload-port YOUR_PORT
pio device monitor --port YOUR_PORT --baud 115200
```

Flashing retains NVS. Syntax errors, unknown
keys/faces, duplicates, empty lists, invalid IDs/units/ranges and unsupported
profiles stop the build with an explanatory error. Use strict JSON: no comments,
trailing commas, hexadecimal numbers, NaN or Infinity. Keep `schema_version: 1`.

Default, `debug` and `vehicle` environments all use `config/instrument.json`.
`vehicle` is a convenient alias, not a CAN-mode override. To use a different
complete configuration, set `custom_gauge_config` in that environment's
`platformio.ini` section. The supplied `listen-only` environment demonstrates this
with [`config/examples/listen-only.json`](../config/examples/listen-only.json):

```sh
pio run -e listen-only
```

The example is an independent file; edits to the main file do not propagate to
it. A rebuild automatically picks up changed JSON without a clean build.
Serial command `v` prints the compiled configuration hash, ECU record, expected
ID/format and face counts; `CAN STATUS` reports the actual receiver settings.
Changing a JSON file on the computer does not change firmware already flashed.

`sh tools/run_native.sh` checks configuration parsing and generated-code behavior
as well as the existing firmware logic. It uses a fixed test configuration for
behavioral assertions, so users can edit their own manifest without rewriting
tests. Alternate-configuration tests cover metric units, extended ID, different
bitrate, passive mode, optional channels and removed saved selections.
