#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
preview_tmp=$(mktemp -d "${TMPDIR:-/tmp}/dgc-preview.XXXXXX")
trap 'rm -rf "$preview_tmp"' EXIT HUP INT TERM
python3 tools/configure.py --output "$preview_tmp/instrument_config.h"
gfx_dir='.pio/libdeps/vehicle/Adafruit GFX Library'
c++ -std=c++17 -O2 -DARDUINO=100 -I tools/preview -I "$preview_tmp" -I include -I "$gfx_dir" \
    tools/preview/render.cpp src/gauges/EngineGauge.cpp src/gauges/CompassGauge.cpp \
    "$gfx_dir/Adafruit_GFX.cpp" -o "$preview_tmp/render"
"$preview_tmp/render" "$preview_tmp"
python3 - "$preview_tmp" "${1:-diagnostics/gauge-preview.png}" <<'PY'
from pathlib import Path
import sys, struct, zlib
# Lossless PNG export of actual GFX pixels, using only the Python standard library.
def png(path,w,h,pixels):
    def chunk(kind,payload):
        return struct.pack('>I',len(payload))+kind+payload+struct.pack('>I',zlib.crc32(kind+payload)&0xffffffff)
    raw=b''.join(b'\0'+bytes(pixels[y*w*3:(y+1)*w*3]) for y in range(h))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))
def tile(name):
    raw=(Path(sys.argv[1])/(name+'.ppm')).read_bytes()
    header=raw.split(b'\n',3);assert header[:3]==[b'P6',b'240 240',b'255'];return header[3]
names=['cold','normal','high','overheat','iat','battery','boost','unavailable','compass']
preview=bytearray(bytes([22,25,31])*(792*792))
for i,name in enumerate(names):
    pixels=tile(name);x=12+(i%3)*264;y=12+(i//3)*264
    for row in range(240):
        offset=((y+row)*792+x)*3
        preview[offset:offset+720]=pixels[row*720:(row+1)*720]
out=Path(sys.argv[2]);out.parent.mkdir(parents=True,exist_ok=True);png(out,792,792,preview)
for name in ['stale','invalid']:png(out.with_name(name+'.png'),240,240,tile(name))
print(out.resolve())
PY
