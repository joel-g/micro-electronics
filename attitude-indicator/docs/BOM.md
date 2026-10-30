# Attitude indicator — bill of materials

Module-level BOM for the v1.0.0 prototype, recorded on 2026-10-30. Quantities are
for one instrument. The compass project reuses this prototype's controller and
display; these are not necessarily two separate sets of purchased hardware.

The identifiers below come from the documented working prototype. A chip number
does not identify the complete breakout board or display assembly. Unknown
module manufacturer part numbers (MPNs) and supplier details are marked explicitly
rather than inferred from common listings.

## Required parts

| Item | Qty | Module / assembly identifier | Chip identifier | Specification / purpose | Module manufacturer / exact MPN | Supplier / purchase link |
| --- | --- | --- | --- | --- | --- | --- |
| AI-01 | 1 | ESP32-S3 SuperMini | ESP32-S3; exact chip ordering suffix not recorded | Controller board; 4 MB flash, 2 MB PSRAM; USB programming and power | Not recorded | Not recorded |
| AI-02 | 1 | MPU-6050 breakout; exact board designation not recorded | MPU-6050 | Accelerometer/gyroscope; I²C address `0x68`; powered from 3.3 V | Not recorded | Not recorded |
| AI-03 | 1 | 1.28-inch round SPI TFT module | GC9A01 controller family; exact suffix not recorded | 240 × 240 pixels; VCC, GND, SCL/SCLK, SDA/MOSI, CS, DC, RST connections; operated at 3.3 V | Not recorded | Not recorded |
| AI-04 | 1 set | Signal and power interconnects | Not applicable | Connections in the README wiring table, including shared 3.3 V and ground; wire lengths, connector types, and individual quantities not recorded | Not recorded | Not recorded |
| AI-05 | 1 | USB data cable compatible with the fitted controller board | Not applicable | Programming, serial diagnostics, and bench power; exact connector/cable assembly not recorded | Not recorded | Not recorded |

Item IDs are documentation identifiers, not PCB reference designators. The USB
host or suitable bench supply is external equipment, not an additional module
in this instrument BOM.

## Compatibility and identification notes

- The firmware's PlatformIO `esp32-s3-devkitc-1` target is a build configuration,
  not the purchased board's part number. The physical board is a SuperMini with
  the flash configuration above. PSRAM is fitted but not enabled by this firmware.
- The MPU-6050 breakout has not been identified as a particular carrier board
  such as GY-521. Record its actual marking before assigning a module number.
- The Adafruit GC9A01A software library does not establish the display module's
  manufacturer or exact controller suffix.
- A replacement board/module must be checked against the established pinout,
  voltage requirements, and firmware before being treated as an equivalent part.
  No alternate assemblies have been qualified.
- No magnetometer, second display, buttons, or CAN interface is required for
  this attitude-indicator release.

## Details still to capture

For each purchased module, add its manufacturer, exact MPN or seller SKU, board
revision/markings, and original supplier product link from packaging or the order
record. Record cable and interconnect specifications if reproducing the physical
assembly. Seller SKUs should remain distinguishable from manufacturer MPNs.

See the [project README](../README.md#hardware) for hardware operation and the
[wiring diagram](../README.md#wiring-diagram) for connections.
