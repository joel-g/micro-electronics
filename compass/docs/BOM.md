# Standalone compass — bill of materials

Module-level BOM for the current compass prototype, recorded on 2026-10-30.
Quantities are for one instrument with **one display**. The controller and display
are reused from the attitude-indicator prototype; the two BOMs are per-project
requirements, not a combined inventory of separately purchased parts.

A chip number does not identify the complete module or display assembly. Unknown
module manufacturer part numbers (MPNs) and supplier details are marked explicitly
rather than inferred from common listings.

## Required parts

| Item | Qty | Module / assembly identifier | Chip identifier | Specification / purpose | Module manufacturer / exact MPN | Supplier / purchase link |
| --- | --- | --- | --- | --- | --- | --- |
| CO-01 | 1 | ESP32-S3 SuperMini | ESP32-S3; exact chip ordering suffix not recorded | Controller board; 4 MB flash, 2 MB PSRAM; USB programming and power | Not recorded | Not recorded |
| CO-02 | 1 | GY-271 magnetometer module, QMC5883P variant | QMC5883P | Three-axis magnetic-field sensor; I²C address `0x2C`, chip ID `0x80`; powered from 3.3 V; DRDY unused | Not recorded | Not recorded |
| CO-03 | 1 | 1.28-inch round SPI TFT module | GC9A01 controller family; exact suffix not recorded | 240 × 240 pixels; VCC, GND, SCL/SCLK, SDA/MOSI, CS, DC, RST connections; operated at 3.3 V | Not recorded | Not recorded |
| CO-04 | 1 set | Signal and power interconnects | Not applicable | Connections in the README wiring table, including shared 3.3 V and ground; wire lengths, connector types, and individual quantities not recorded | Not recorded | Not recorded |
| CO-05 | 1 | USB data cable compatible with the fitted controller board | Not applicable | Programming, serial calibration commands, diagnostics, and bench power; exact connector/cable assembly not recorded | Not recorded | Not recorded |

Item IDs are documentation identifiers, not PCB reference designators. The USB
host or suitable bench supply is external equipment, not an additional module
in this instrument BOM.

## Optional hardware present in the shared prototype

| Item | Qty required | Qty currently connected | Module / chip identifier | Role | Module manufacturer / exact MPN | Supplier / purchase link |
| --- | --- | --- | --- | --- | --- | --- |
| CO-06 | 0 | 1 | MPU-6050 breakout; carrier board designation not recorded | Left connected on the shared I²C bus at `0x68`; compass firmware does not configure it or use its measurements | Not recorded | Not recorded |

The MPU-6050 is needed by the separate attitude-indicator firmware, not by this
standalone compass. No second display, buttons, GPS, or CAN interface is included.

## Compatibility and identification notes

- **GY-271 alone is not a sufficient purchase specification.** The fitted sensor
  was confirmed by the I²C scan and chip ID as QMC5883P. A GY-271 listing with
  another sensor is not an approved substitute; QMC5883L and HMC5883L require
  different drivers/register maps.
- The firmware's PlatformIO `esp32-s3-devkitc-1` target is a build configuration,
  not the purchased board's part number. The physical board is a SuperMini with
  the flash configuration above. PSRAM is fitted but not enabled by this firmware.
- The Adafruit GC9A01A software library does not establish the display module's
  manufacturer or exact controller suffix.
- Check any replacement assembly against the established pinout, voltage
  requirements, and firmware. No alternate assemblies have been qualified.

## Details still to capture

For each purchased module, add its manufacturer, exact MPN or seller SKU, board
revision/markings, and original supplier product link from packaging or the order
record. Record cable and interconnect specifications if reproducing the physical
assembly. Seller SKUs should remain distinguishable from manufacturer MPNs.

See the [project README](../README.md#hardware-and-connections) for connections,
sensor identification, and calibration instructions.
