#!/usr/bin/env python3
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('configure',ROOT/'tools/configure.py')
config=importlib.util.module_from_spec(spec);spec.loader.exec_module(config)
BASE=config.load(ROOT/'test/fixtures/instrument.json')

class ConfigurationTests(unittest.TestCase):
    def reject(self,transform):
        value=copy.deepcopy(BASE);transform(value)
        with self.assertRaises(config.ConfigError):config.validate(value)
    def test_shipped_examples(self):
        for path in (ROOT/'config').rglob('*.json'):config.load(path)
    def test_invalid_root(self):
        for value in [[],None,False,{},'bad']:
            with self.subTest(value=value),self.assertRaises(config.ConfigError):config.validate(value)
    def test_keys_and_version(self):
        self.reject(lambda c:c.update(surprise=1))
        self.reject(lambda c:c.pop('can'))
        self.reject(lambda c:c.update(schema_version=True))
        self.reject(lambda c:c['can'].update(bitratee=500000))
    def test_can(self):
        for field,value in [('bitrate',333333),('bitrate',True),('id',2048),('id',-1),('id',10**400),('mode','normal'),('format','std')]:
            with self.subTest(field=field):self.reject(lambda c:c['can'].update({field:value}))
        self.reject(lambda c:c['can'].update(format='extended',id=0x20000000))
        self.reject(lambda c:c['ecu'].update(profile='link_generic_dash_2'))
        self.reject(lambda c:c['ecu'].update(transmit_channel=0))
        self.reject(lambda c:c['ecu'].update(model='bad\nname'))
    def test_manifests(self):
        for faces in [[],['coolnt'],['fans'],['iat','iat'],[False],['rpm']*11,'rpm']:
            with self.subTest(faces=faces):self.reject(lambda c:c['displays'].update({'1':faces}))
        c=copy.deepcopy(BASE);c['displays']={'1':['compass'],'2':['horizon']};config.validate(c)
    def test_units_ranges(self):
        self.reject(lambda c:c['display'].update(temperature_unit='kelvin'))
        self.reject(lambda c:c['display'].update(stale_ms=0))
        self.reject(lambda c:c['display']['coolant_c'].update(high=60))
        self.reject(lambda c:c['display']['lambda'].update(stoichiometric_afr=0))
        for v in [[0,0],[100,0],[float('nan'),50],[0,float('inf')],[0],['0',2]]:
            self.reject(lambda c:c['scales'].update(rpm=v))
    def test_json_parsing(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'bad.json'
            for raw in ['{"a":1,"a":2}','{"a":NaN}','{"a":Infinity}','{']:
                p.write_text(raw)
                with self.assertRaises(config.ConfigError):config.load(p)
    def test_deterministic_and_optional_sources(self):
        c=copy.deepcopy(BASE)
        self.assertIn('enableFuel=false',config.header(c))
        c['displays']['1'].append('fuel');self.assertIn('enableFuel=true',config.header(c))
        with tempfile.TemporaryDirectory() as tmp:
            out=Path(tmp)/'instrument_config.h';config.generate(ROOT/'test/fixtures/instrument.json',out)
            timestamp=out.stat().st_mtime_ns;config.generate(ROOT/'test/fixtures/instrument.json',out)
            self.assertEqual(out.stat().st_mtime_ns,timestamp)
            c['can']['id']=1001;p=Path(tmp)/'changed.json';p.write_text(json.dumps(c));config.generate(p,out)
            self.assertIn('canId=1001',out.read_text())
    def test_alternate_config_compiles_and_runs(self):
        c=copy.deepcopy(BASE)
        c['can'].update(id=0x18ff1234,format='extended',bitrate=250000,mode='listen_only')
        c['display'].update(temperature_unit='C',pressure_unit='kPa',compass_theme='light',horizon_style='standard')
        c['display']['lambda'].update(unit='afr',stoichiometric_afr=9.8)
        c['displays']={'1':['coolant','oil','fuel','lambda','compass'],'2':['iat','horizon']}
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);(p/'instrument_config.h').write_text(config.header(c))
            for name,sources in [('integration',['test/config_integration_test.cpp','src/display_settings.cpp']),('receiver',['test/can_receiver_test.cpp','src/can/CanReceiver.cpp'])]:
                subprocess.run(['c++','-std=c++17','-O2','-I',tmp,'-I','test/fakes','-I','include',*sources,'-o',str(p/name)],cwd=ROOT,check=True)
                subprocess.run([str(p/name)],check=True)

if __name__=='__main__':unittest.main()
