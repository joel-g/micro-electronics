# Micro-electronics

Embedded electronics projects.

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
