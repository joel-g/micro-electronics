# Compass & Horizon — bill of materials

Module-level BOM for one combined instrument, recorded on 2026-10-30. This
prototype reuses the controller, display, and sensors from the standalone
projects and adds one push button. Quantities are per instrument, not additional
purchases. There is **one physical display** and both sensors are required.

Chip identifiers and module designations are listed separately. Exact module
manufacturer part numbers (MPNs), board revisions, and supplier links have not
been recorded; no specific seller or variant is inferred from a generic name.

## Required parts

| Item | Qty | Module / assembly identifier | Chip identifier | Specification / purpose | Module manufacturer / exact MPN | Supplier / purchase link |
| --- | --- | --- | --- | --- | --- | --- |
| CH-01 | 1 | ESP32-S3 SuperMini | ESP32-S3; exact ordering suffix not recorded | Controller; 4 MB flash, 2 MB PSRAM; USB programming and power | Not recorded | Not recorded |
| CH-02 | 1 | 1.28-inch round SPI TFT module | GC9A01 controller family; exact suffix not recorded | 240 × 240 pixels; one shared display; operated at 3.3 V | Not recorded | Not recorded |
| CH-03 | 1 | GY-271 magnetometer module, QMC5883P variant | QMC5883P | Compass sensor; I²C `0x2C`, chip ID `0x80`; 3.3 V; DRDY unconnected | Not recorded | Not recorded |
| CH-04 | 1 | MPU-6050 breakout; carrier board designation not recorded | MPU-6050 | Accelerometer/gyroscope for Horizon; I²C `0x68`; 3.3 V | Not recorded | Not recorded |
| CH-05 | 1 | Normally-open momentary push button | Not applicable | Connect GPIO13 to GND when pressed; internal pull-up enabled, no external pull-up resistor | Not recorded | Not recorded |
| CH-06 | 1 set | Signal and power interconnects | Not applicable | SPI, shared I²C, button, 3.3 V, and ground connections; wire lengths and connector specifications not recorded | Not recorded | Not recorded |
| CH-07 | 1 | USB data cable compatible with the fitted controller board | Not applicable | Programming, serial diagnostics, and bench power; exact connector/cable assembly not recorded | Not recorded | Not recorded |

Item IDs are documentation identifiers, not PCB reference designators. The USB
host or external bench supply is test equipment, not another instrument module.

## Compatibility notes

- GY-271 is not a unique sensor part number. The connected chip was identified as
  QMC5883P; QMC5883L and HMC5883L versions are not interchangeable with its driver.
- The MPU breakout has not been identified as a particular carrier such as
  GY-521. Both sensors share GPIO1/2 and must retain their distinct I²C addresses.
- PlatformIO's `esp32-s3-devkitc-1` target is the build configuration, not the
  physical board's MPN. The working board is the SuperMini described above.
  PSRAM is fitted but not enabled by this firmware.
- The Adafruit GC9A01A library does not identify the manufacturer or exact part
  number of the complete display assembly. Match the existing interface, voltage,
  resolution, and pin labels when identifying or replacing it.
- The button must be momentary and normally open. Its package, contact rating,
  mounting style, and exact part number remain to be recorded.
- No second display, CAN transceiver, GPS receiver, or additional sensor is part
  of this instrument. No alternative module assemblies have been qualified.

## Purchasing details to complete

Record each module's actual manufacturer, MPN, seller SKU, board markings/revision,
and original supplier link from the order record or packaging. Keep seller SKUs
distinct from manufacturer MPNs. Record wire/cable and connector specifications
when reproducing the assembly.

See the [README wiring table and diagram](../README.md#hardware-and-wiring) for
connections, and the [verification record](verification.md) for tested behavior.
