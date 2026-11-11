# CR-V / Link G4X Monsoon connection

On November 8, 2026 the user confirmed **G4X Monsoon, maincode 6.22.3**.
The previous G4+ assumption was incorrect. The module is now connected directly
in place of the AEM through a 12-inch twisted pair/Deutsch connector and a ground
reference. The initial engine-off capture confirmed reception at 500 kbit/s.
The [verification record](verification.md#follow-up-capture-and-display-confirmation)
covers the later Generic Dash capture and display checks. The fitted module's
termination remains unverified.

The checklist below is a historical bring-up plan, not a claim that every item
has been completed. Current firmware configuration is documented in
[configuration.md](configuration.md). The [Generic Dash guide](link-generic-dash.md)
records the Channel 2 setup, the acknowledging build, and how to select passive mode.

## Sources and current physical record

- The [Link downloads index](https://linkecu.com/pages/downloads) lists **G4X
  MonsoonX** under **G4X Wire-in Quick Start Guides**. See page 3 of that guide
  for wiring and connector orientation. PCLink G4X/G5 Help contains the full
  installation and CAN stream documentation. Match the ECU and connector view
  before wiring.
- [TI SN65HVD230 datasheet](https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf),
  device functional modes and physical-layer termination guidance.
- [Waveshare SN65HVD230 reference schematic](https://files.waveshare.com/upload/c/c5/SN65HVD230-CAN-Board-Schematic.pdf).
  It depicts RS pulled down through 10 kΩ and a 120 Ω CANH/CANL resistor.
  That published reference does **not** establish the fitted module's revision,
  component values, or actual termination.

User inspection of the installed Waveshare 3.3 V module: SN65HVD230-based
appearance; no exposed RS/standby pin, no obvious termination jumper/switch;
a resistor marked 101 is visible but its purpose is unknown. Module CAN TX is
on GPIO5, CAN RX on GPIO6. Actual RS connection and onboard termination are
**unverified**. Do not assume a 120 Ω resistor is fitted from the board name or
resistor marking. Driver initialization cannot inspect these physical properties.

On the SN65HVD230, RS low selects high-speed operation; a pull-down resistor can
select slope control. RS high selects standby with its transmitter disabled and
receiver active. This differs from SN65HVD231 sleep behavior. Identify the fitted
chip and compare its actual RS network to the manufacturer's data before choosing
vehicle settings; firmware does not control an unexposed RS pin.

## Before connecting

1. Confirm the exact ECU is **G4X Monsoon (MonsoonX)**, identify the loom connector and
   verify the CAN-H and CAN-L cavities using the official pinout and its connector
   viewing direction. Trace and label the fitted harness wires. Do not connect
   by guessed cavity numbers or wire colors; actual harness correspondence remains
   to be verified before this prototype is attached.
2. Open existing PCLink settings read-only. Save a copy/export and record ECU
   firmware, CAN port, bitrate, existing devices, transmit streams, stream IDs,
   standard/extended format, rates and channel definitions. Do not alter ECU
   configuration as part of this milestone.
3. Match `can.bitrate`, `can.id` and `can.format` in `config/instrument.json`
   to PCLink. The shipped configuration uses 500000, standard ID 1000 and
   `normal_rx` (ACK/error frames) for the approved two-device setup. Use
   `listen_only` for passive monitoring with another acknowledging peer.
   See [configuration](configuration.md) and the current test plan.
4. With the module **unpowered and isolated from the vehicle**, measure resistance
   directly between CANH and CANL and record the result. Inspect the module RS
   network if accessible without modifying the board. Do not measure resistance
   on a powered bus. An unexplained reading needs investigation before connection.
5. Inspect the existing bus devices, topology and termination. A conventional
   high-speed CAN trunk has one 120 Ω terminator at each end (approximately 60 Ω
   in parallel); a mid-bus gauge branch must not simply add a third terminator.
   Verify actual termination and any ECU internal resistor rather than assuming
   it. Do not remove/alter components until the actual network and module resistor
   have been identified.
6. Identify a suitable common ground reference and bench/vehicle power arrangement.
   This prototype uses regulated 3.3 V; no direct 12 V power input is implemented.
   Plan CANH/CANL as a short twisted-pair branch with the correct polarity.

## First reception session

7. After the physical checks, connect CAN-H, CAN-L and the agreed ground reference.
   Retain listen-only reception and existing ECU configuration. Observe driver
   status/errors and capture actual raw frames with timestamps and bitrate recorded.
   A running controller or zero errors does not prove frame reception. A monitor
   in listen-only does not ACK: an isolated transmitting ECU needs a suitable
   existing acknowledging node/network for normal CAN traffic.
8. Record observed IDs, formats, DLCs, payloads, frame rates and changing values.
   Compare them against the **actual PCLink transmit configuration** and Link's
   documented stream layout. The raw monitor samples serial output and has a
   bounded ID table; use appropriate capture equipment for lossless busy-bus analysis.
9. If no suitable stream exists, prepare a proposed Link Generic Dash or defined
   user stream for later review. Do not change the tune/CAN setup now. Fan status,
   oil pressure and fuel pressure may need custom configured channels and actual
   sensors/wiring; their presence on a gauge list does not establish availability.
10. Implement and validate one signal at a time against PCLink runtime values.
    Confirm units, scaling, byte order, signedness, availability, update rate and
    stale handling. Boost requires absolute MAP and measured atmospheric pressure;
    AFR requires the correct fuel stoichiometric ratio. Keep decoding unavailable
    until its definition is supported by both documentation and actual frames.

Record hardware photographs/revision, isolated termination resistance, exact ECU
CAN pins, PCLink settings/export, chosen ground/power connections, frame captures
and signal validation results in the verification record before calling the
vehicle connection or any decoded channel verified.
