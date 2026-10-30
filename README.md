# Micro-electronics

Embedded electronics projects.

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
