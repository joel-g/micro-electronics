#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
dgc_test_dir=$(mktemp -d "${TMPDIR:-/tmp}/dgc-native.XXXXXX")
trap 'rm -rf "$dgc_test_dir"' EXIT HUP INT TERM
python3 test/config_test.py
python3 tools/configure.py --config test/fixtures/instrument.json --output "$dgc_test_dir/config/instrument_config.h"
python3 tools/configure.py --config config/examples/listen-only.json --output "$dgc_test_dir/passive/instrument_config.h"
c++ -std=c++17 -O2 -I "$dgc_test_dir/config" -I include test/gauge_style_test.cpp -o "$dgc_test_dir/style"
c++ -std=c++17 -O2 -I "$dgc_test_dir/config" -I include test/link_decoder_test.cpp -o "$dgc_test_dir/decoder"
c++ -std=c++17 -O2 -I "$dgc_test_dir/config" -I include test/engine_data_test.cpp -o "$dgc_test_dir/data"
c++ -std=c++17 -O2 -I test/fakes -I "$dgc_test_dir/config" -I include test/settings_test.cpp src/display_settings.cpp -o "$dgc_test_dir/settings"
c++ -std=c++17 -O2 -I test/fakes -I "$dgc_test_dir/config" -I include test/can_receiver_test.cpp src/can/CanReceiver.cpp -o "$dgc_test_dir/can"
c++ -std=c++17 -O2 -I test/fakes -I "$dgc_test_dir/passive" -I include test/can_receiver_test.cpp src/can/CanReceiver.cpp -o "$dgc_test_dir/can_passive"
c++ -std=c++17 -O2 -I "$dgc_test_dir/config" -I include test/button_test.cpp src/input/Button.cpp -o "$dgc_test_dir/button"
framebuffer_src=../shared/IndexedFramebuffer/src
c++ -std=c++17 -O2 -I test/fakes -I "$dgc_test_dir/config" -I include -I "$framebuffer_src" \
    test/display_test.cpp src/gauges/Display.cpp "$framebuffer_src/IndexedFramebuffer.cpp" -o "$dgc_test_dir/display"
c++ -std=c++17 -O2 -I "$dgc_test_dir/config" -I include test/compass_math_test.cpp -o "$dgc_test_dir/compass"
c++ -std=c++17 -O2 -I "$dgc_test_dir/config" -I include test/horizon_geometry_test.cpp -o "$dgc_test_dir/horizon"
c++ -std=c++17 -O2 -I test/fakes -I "$dgc_test_dir/config" -I include test/calibration_workflow_test.cpp src/sensors/Magnetometer.cpp src/sensors/calibration_store.cpp -o "$dgc_test_dir/calibration"
for dgc_test in style decoder data settings can can_passive button display compass horizon calibration; do
    "$dgc_test_dir/$dgc_test"
done
