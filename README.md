# Micro-electronics

Embedded electronics projects.

## Dual Gauge with Buttons — v1.0.0

[Hardware, wiring diagram, button controls, calibration, and testing](dual-gauge-with-buttons/README.md)

Two GC9A01 screens run the artificial horizon and compass continuously. Independent
buttons select Standard/Numerical attitude and Dark/Light compass presentations.
Hold the attitude button to re-center; hold the compass button for a 60-second
calibration that automatically saves valid results. Both display selections and
saved magnetic calibration survive power cycles. The fitted CAN transceiver is
unused by this firmware.

```sh
cd dual-gauge-with-buttons
pio run
pio run -t upload
```

## Compass & Horizon — v1.1.0

[Hardware, button operation, calibration, and testing](compass-horizon/README.md)

Both instruments on one GC9A01 display, selected with a GPIO13 momentary button.
The last-used mode is saved, and both sensors keep updating in the background.
Hold the button for two seconds to re-center Horizon or toggle the Compass light/dark theme.
This third project preserves the two standalone projects below.

```sh
cd compass-horizon
pio run
pio run -t upload
```

## Standalone compass — v1.0.0

[Hardware, wiring, calibration, and testing](compass/README.md)

An ESP32-S3 SuperMini magnetic compass using a QMC5883P magnetometer and the
existing single GC9A01 display. The MPU-6050 can remain connected but its
measurements are not used. This is separate firmware from the attitude indicator.

```sh
cd compass
pio run
pio run -t upload
```

## Attitude indicator — v1.0.0

[Project documentation and wiring diagram](attitude-indicator/README.md)

An ESP32-S3 SuperMini artificial horizon using an MPU-6050 accelerometer/gyroscope
and a GC9A01 1.28-inch round TFT. Built with PlatformIO and Arduino.

```sh
cd attitude-indicator
pio run
pio run -t upload
```

The default firmware hides the debug overlay. Use `pio run -e debug -t upload`
to show pitch/roll readouts, the frame counter, and the heartbeat indicator.
See the project README for hardware connections, calibration, verification, and
the separate opt-in watchdog test.
