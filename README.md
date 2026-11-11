# Micro-electronics

ESP32-S3 firmware and hardware documentation for round-display instruments.
Choose a project for its wiring, controls, calibration, and test results.

| Project | Hardware and features | Parts |
| --- | --- | --- |
| [Dual Gauge CAN](dual-gauge-can/README.md) | Two displays with configurable horizon, compass, and engine-data faces for a Link ECU using Generic Dash CAN. | [BOM](dual-gauge-can/docs/BOM.md) |
| [Dual Gauge with Buttons](dual-gauge-with-buttons/README.md) | Two displays run the horizon and compass together, with independent buttons for display modes, re-centering, and calibration. | [BOM](dual-gauge-with-buttons/docs/BOM.md) |
| [Compass & Horizon](compass-horizon/README.md) | One display switches between the compass and artificial horizon with a button. | [BOM](compass-horizon/docs/BOM.md) |
| [Standalone compass](compass/README.md) | One GC9A01 display and a QMC5883P magnetometer. | [BOM](compass/docs/BOM.md) |
| [Attitude indicator](attitude-indicator/README.md) | One GC9A01 display and an MPU-6050 accelerometer/gyroscope. | [BOM](attitude-indicator/docs/BOM.md) |

## Shared hardware

The [dual-gauge enclosure](hardware/dual-gauge-enclosure/README.md) fits the two-display
assembly for both Dual Gauge CAN and Dual Gauge with Buttons. Its guide covers
printable STLs, print orientation, fasteners, and fit checks.

## Build and upload

1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html)
   or open the PlatformIO terminal in VS Code.
2. Clone the full repository and enter the firmware directory for your hardware.
   Check its wiring and, for Dual Gauge CAN, its
   [instrument configuration](dual-gauge-can/docs/configuration.md).
3. Build the project, connect the ESP32 with a USB data cable, and find its port:

   ```sh
   pio run
   pio device list
   ```

4. Replace `YOUR_PORT` with the port shown for your board and upload:

   ```sh
   pio run -t upload --upload-port YOUR_PORT
   pio device monitor --port YOUR_PORT --baud 115200
   ```

Close serial monitors before uploading or using the project capture tools. The
Python serial helpers use `pyserial` and POSIX serial controls on macOS and Linux.
The PlatformIO Core installer supplies a Python environment with `pyserial`, or
you can install it in your own Python environment with
`python -m pip install pyserial`.

## Shared framebuffer and contributions

The indexed-color projects build against [IndexedFramebuffer](shared/IndexedFramebuffer)
in the same checkout. Tile detection, RGB565 conversion, and transfer batching
live there. Each firmware owns its display setup, palette, and render schedule.
The [GitHub Actions workflow](.github/workflows/indexed-framebuffer.yml) checks
the four indexed firmware builds. The project verification sections describe
native tests and checks that require physical hardware.

For a bug report or pull request, include the project and firmware version, your
board and modules, the steps to reproduce or verify the behavior, and any relevant
serial output. Identify which checks ran on hardware.

## License

Original code and technical documentation are licensed under the
[MIT License](LICENSE). Photographs, videos, and branding are excluded.
Dependencies retain their own licenses. See the [licensing scope](LICENSING.md).
