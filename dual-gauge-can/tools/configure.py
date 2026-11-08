#!/usr/bin/env python3
"""Validate JSON and generate build-only C++ constants; no third-party packages."""
import argparse
import hashlib
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FACES = {"horizon":"Horizon", "compass":"Compass", "coolant":"Coolant", "oil":"Oil",
         "rpm":"Rpm", "boost":"Boost", "fuel":"Fuel", "lambda":"Lambda", "battery":"Battery", "iat":"Iat"}
SCALES = {"coolant", "iat", "rpm", "boost", "battery", "oil", "fuel", "lambda"}

class ConfigError(ValueError):
    pass

def keys(value, expected, path):
    if not isinstance(value, dict):
        raise ConfigError(f"{path}: expected an object")
    missing, extra = set(expected)-set(value), set(value)-set(expected)
    if missing or extra:
        raise ConfigError(f"{path}: missing keys {sorted(missing)}; unknown keys {sorted(extra)}")

def number(value, low, high, path, integer=False):
    if type(value) not in (int, float) or (type(value) is float and not math.isfinite(value)) or (integer and type(value) is not int):
        raise ConfigError(f"{path}: expected a finite {'integer' if integer else 'number'}")
    if not low <= value <= high:
        raise ConfigError(f"{path}: must be between {low} and {high}")

def choice(value, allowed, path):
    if not isinstance(value, str) or value not in allowed:
        raise ConfigError(f"{path}: choose one of {sorted(allowed)}")

def validate(c):
    keys(c, {"schema_version","ecu","can","displays","display","scales"}, "config")
    if type(c["schema_version"]) is not int or c["schema_version"] != 1:
        raise ConfigError("schema_version: supported version is 1")
    ecu=c["ecu"];keys(ecu,{"model","firmware","profile","can_bus","transmit_channel","transmit_rate_hz"},"ecu")
    for name in ("model","firmware"):
        value=ecu[name]
        if not isinstance(value,str) or not 1<=len(value)<=80 or not all(32<=ord(x)<=126 for x in value):
            raise ConfigError(f"ecu.{name}: use 1–80 printable ASCII characters")
    choice(ecu["profile"],{"link_generic_dash"},"ecu.profile")
    number(ecu["can_bus"],1,2,"ecu.can_bus",True)
    number(ecu["transmit_channel"],1,10,"ecu.transmit_channel",True)
    number(ecu["transmit_rate_hz"],1,100,"ecu.transmit_rate_hz",True)
    can=c["can"];keys(can,{"bitrate","mode","id","format"},"can")
    number(can["bitrate"],125000,1000000,"can.bitrate",True)
    if can["bitrate"] not in (125000,250000,500000,1000000):
        raise ConfigError("can.bitrate: supported values are 125000, 250000, 500000, 1000000")
    choice(can["mode"],{"listen_only","normal_rx"},"can.mode")
    choice(can["format"],{"standard","extended"},"can.format")
    number(can["id"],0,0x7ff if can["format"]=="standard" else 0x1fffffff,"can.id",True)
    keys(c["displays"],{"1","2"},"displays")
    for screen,faces in c["displays"].items():
        if not isinstance(faces,list) or not 1<=len(faces)<=len(FACES):
            raise ConfigError(f"displays.{screen}: needs 1–{len(FACES)} face names")
        for face in faces:
            if face=="fans":
                raise ConfigError(f"displays.{screen}: fans has no supported CAN mapping yet; remove it")
            choice(face,FACES,f"displays.{screen}")
        if len(set(faces))!=len(faces):
            raise ConfigError(f"displays.{screen}: duplicate faces are not allowed")
    d=c["display"];keys(d,{"temperature_unit","pressure_unit","compass_theme","horizon_style","stale_ms","coolant_c","lambda"},"display")
    choice(d["temperature_unit"],{"C","F"},"display.temperature_unit")
    choice(d["pressure_unit"],{"kPa","psi"},"display.pressure_unit")
    choice(d["compass_theme"],{"dark","light"},"display.compass_theme")
    choice(d["horizon_style"],{"standard","numerical"},"display.horizon_style")
    number(d["stale_ms"],100,60000,"display.stale_ms",True)
    t=d["coolant_c"];keys(t,{"warm","high","overheat"},"display.coolant_c")
    for name,v in t.items():number(v,-50,205,"display.coolant_c."+name)
    if not t["warm"]<t["high"]<t["overheat"]:
        raise ConfigError("display.coolant_c: require warm < high < overheat")
    lam=d["lambda"];keys(lam,{"unit","stoichiometric_afr"},"display.lambda")
    choice(lam["unit"],{"lambda","afr"},"display.lambda.unit")
    number(lam["stoichiometric_afr"],1,30,"display.lambda.stoichiometric_afr")
    keys(c["scales"],SCALES,"scales")
    for name,r in c["scales"].items():
        if not isinstance(r,list) or len(r)!=2:raise ConfigError(f"scales.{name}: expected [minimum, maximum]")
        for v in r:number(v,-100000,100000,"scales."+name)
        if not r[0]<r[1]:raise ConfigError(f"scales.{name}: minimum must be less than maximum")
    return c

def unique_object(pairs):
    result={}
    for key,value in pairs:
        if key in result:raise ConfigError(f"duplicate JSON key: {key}")
        result[key]=value
    return result

def load(path):
    try:
        return validate(json.loads(Path(path).read_text(),object_pairs_hook=unique_object,
                                   parse_constant=lambda s: (_ for _ in ()).throw(ConfigError(f"invalid JSON number: {s}"))))
    except (OSError,json.JSONDecodeError) as e:
        raise ConfigError(f"{path}: {e}") from e

def header(c):
    validate(c)
    digest=hashlib.sha256(json.dumps(c,sort_keys=True,separators=(",",":")).encode()).hexdigest()[:12]
    def text(v):return json.dumps(v,ensure_ascii=True)
    def flag(v):return "true" if v else "false"
    def real(v):return repr(float(v))+"f"
    ecu,can,d=c["ecu"],c["can"],c["display"]
    enabled=set(c["displays"]["1"]+c["displays"]["2"])
    lines=['// Generated from validated JSON. Edit config/instrument.json, not this file.',
           '#pragma once','#include "gauge_types.h"','#include <cstdint>',
           'namespace instrumentConfig {',f'inline constexpr const char *hash={text(digest)};',
           f'inline constexpr const char *ecuModel={text(ecu["model"])}, *ecuFirmware={text(ecu["firmware"])}, *profile={text(ecu["profile"])};',
           f'inline constexpr uint32_t canBitrate={can["bitrate"]}, canId={can["id"]};',
           f'inline constexpr bool canAck={flag(can["mode"]=="normal_rx")}, canExtended={flag(can["format"]=="extended")};',
           f'inline constexpr unsigned ecuBus={ecu["can_bus"]}, ecuChannel={ecu["transmit_channel"]}, ecuRateHz={ecu["transmit_rate_hz"]};']
    for face in ("oil","fuel","lambda"):
        lines.append(f'inline constexpr bool enable{FACES[face]}={flag(face in enabled)};')
    lines+=['}', 'namespace gaugeConfig {']
    for name,screen in (("first","1"),("second","2")):
        lines.append(f'inline constexpr Gauge {name}[]={{'+','.join('Gauge::'+FACES[f] for f in c['displays'][screen])+'};')
    lines += [f'inline constexpr uint32_t staleMs={d["stale_ms"]};',
              f'inline constexpr bool temperatureF={flag(d["temperature_unit"]=="F")}, pressurePsi={flag(d["pressure_unit"]=="psi")};',
              f'inline constexpr bool compassLight={flag(d["compass_theme"]=="light")}, horizonNumerical={flag(d["horizon_style"]=="numerical")};',
              f'inline constexpr bool showAfr={flag(d["lambda"]["unit"]=="afr")};',
              f'inline constexpr float stoichiometricAfr={real(d["lambda"]["stoichiometric_afr"])};',
              f'inline constexpr float coolantWarmC={real(d["coolant_c"]["warm"])}, coolantHighC={real(d["coolant_c"]["high"])}, coolantOverheatC={real(d["coolant_c"]["overheat"])};',
              'constexpr float temperature(float c) {return temperatureF?c*1.8f+32:c;}',
              'constexpr float pressure(float kpa) {return pressurePsi?kpa/6.894757f:kpa;}',
              'constexpr float mixture(float lambda) {return showAfr?lambda*stoichiometricAfr:lambda;}',
              'struct Range {float minimum, maximum;};',
              'constexpr Range scale(Gauge gauge) {switch(gauge) {']
    for face,r in c['scales'].items():
        conversion='temperature' if face in ('coolant','iat') else 'pressure' if face in ('oil','fuel','boost') else 'mixture' if face=='lambda' else ''
        values=[f'{conversion}({real(v)})' if conversion else real(v) for v in r]
        lines.append(f'case Gauge::{FACES[face]}:return {{{values[0]},{values[1]}}};')
    lines+=['default:return {0,1};}}','}','']
    return '\n'.join(lines)

def generate(source,output):
    c=load(source);rendered=header(c);out=Path(output);out.parent.mkdir(parents=True,exist_ok=True)
    if not out.exists() or out.read_text()!=rendered:out.write_text(rendered)
    return c

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config',type=Path,default=ROOT/'config/instrument.json')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    try:
        c=generate(args.config,args.output) if args.output else load(args.config)
    except ConfigError as e:parser.exit(2,f'Configuration error: {e}\n')
    print(f'Config OK: {args.config}; {c["can"]["mode"]}, {c["can"]["bitrate"]} bit/s, ID {c["can"]["id"]}; faces {c["displays"]}')

if __name__=='__main__':main()
