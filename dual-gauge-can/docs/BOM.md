# Dual Gauge CAN — bill of materials

Assembly totals, recorded 2026-11-05. Exact ordering suffixes, display/module
revisions, seller links and unrecorded MPNs remain unknown rather than inferred.

| Item | Qty | Module / identifier | Purpose / known chip |
| --- | --- | --- | --- |
| DGC-01 | 1 | ESP32-S3 SuperMini, 4 MB flash / 2 MB PSRAM | Controller; PSRAM disabled |
| DGC-02 | 2 | 1.28-inch 240×240 GC9A01 SPI TFT; exact controller suffix/module MPN unknown | Horizon/engine faces on CS10; compass/engine faces on CS7 |
| DGC-03 | 1 | MPU-6050 breakout; carrier designation unknown | Attitude, I²C 0x68 |
| DGC-04 | 1 | GY-271 QMC5883P variant | Compass, I²C 0x2C; QMC5883L/HMC5883L not substitutes |
| DGC-05 | 2 | Normally-open momentary push buttons; MPN unknown | GPIO13 and GPIO4 to GND; internal pull-ups |
| DGC-06 | 1 | Waveshare 3.3 V CAN module, apparently SN65HVD230-based; exact revision/MPN unverified | TX5 / RX6, direct Link connection since November 8; RS and termination unverified |
| DGC-07 | 1 set | Signal/power interconnects | SPI, I²C, CS, buttons, CAN logic, 3.3 V and GND |
| DGC-08 | 1 | USB data/power cable matching fitted board | Bench power, upload and diagnostics |

The visible resistor marked 101 has not been traced or measured. No termination
switch/jumper or external RS pin was found by the user. Neither firmware nor a
reference-board schematic proves that the fitted board has 120 Ω termination.
Measure isolated CANH-to-CANL resistance with power disconnected before vehicle
connection, as described in [vehicle-connection.md](vehicle-connection.md).

These item IDs are documentation labels, not PCB reference designators. No
vehicle power regulator/protection assembly or vehicle harness has been specified
or qualified by this bench milestone. Purchasing records, exact manufacturer
MPNs and board revisions should be added when identified.

See the [wiring table and diagram](../README.md#wiring) and
[verification record](verification.md) for the tested assembly.

## Vehicle test connection, November 8, 2026

User confirmed G4X Monsoon (MonsoonX), maincode firmware 6.22.3. This supersedes
the earlier G4+ identification. Direct 12-inch twisted pair through a Deutsch
connector plus ground reference replaces the unplugged AEM lambda connection.
Only ECU and this module are on CAN during the test. ECU powered, engine stopped.
Connector series/part numbers and termination measurements remain unverified.
