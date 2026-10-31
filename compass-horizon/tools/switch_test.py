#!/usr/bin/env python3
"""Exercise both faces, rapid switches, and saved-mode restart on real hardware.

Keep the button released and MPU level. Finishes in Compass. Does not write
compass calibration. Uses the normal serial command path, not synthetic sensors.
"""
import argparse
from pathlib import Path
import time
import serial
from serial_capture import open_port, parse


def check(ok, message):
    if not ok:
        raise AssertionError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',default='/dev/cu.usbmodem101')
    parser.add_argument('--log',default='diagnostics/switch-test.log')
    args=parser.parse_args()
    Path(args.log).parent.mkdir(parents=True,exist_ok=True)
    connection=open_port(args.port)
    with open(args.log,'w') as log:
        def note(message):
            print(message,flush=True); log.write('# '+message+'\n'); log.flush()

        def record(deadline):
            nonlocal connection
            while time.monotonic()<deadline:
                try:
                    line=connection.readline().decode(errors='replace').strip()
                except (serial.SerialException,OSError):
                    connection.close()
                    while time.monotonic()<deadline:
                        try:
                            connection=open_port(args.port); break
                        except (serial.SerialException,OSError):
                            time.sleep(0.2)
                    else:
                        raise AssertionError('USB did not return')
                    continue
                if line:
                    log.write(line+'\n'); log.flush()
                    value=parse(line)
                    if value: return value
            raise AssertionError('Diagnostics timeout')

        def wait_for(predicate,seconds=5):
            deadline=time.monotonic()+seconds
            while True:
                value=record(deadline)
                if predicate(value): return value

        def healthy(value,baseline):
            for key in ('imu_fresh','mag_fresh','mag_valid','cal','saved','wdt'):
                check(value[key]=='1',f'{key} not healthy: {value}')
            for key in ('offset','imu_err','mag_err','overflow','imu_retry','mag_retry','reset','presses'):
                check(value[key]==baseline[key],f'{key} changed unexpectedly')

        try:
            # Drain old buffered lines and observe current reports before commands.
            end=time.monotonic()+4
            first=record(time.monotonic()+6)
            while time.monotonic()<end:
                first=record(time.monotonic()+4)
            healthy(first,first)
            check(first['btn']=='1','Release the physical button before this test')
            before=first
            note('Baseline: '+str(first))
            for i in range(20):
                expected='horizon' if before['mode']=='compass' else 'compass'
                switches=int(before['switches'])+1
                connection.write(b'm')
                after=wait_for(lambda r:int(r['switches'])>=switches)
                check(int(after['switches'])==switches and after['mode']==expected,'Wrong mode/count')
                healthy(after,first)
                check(int(after['up'])>int(before['up']),'Unexpected restart')
                check(int(after['imu_sample'])>int(before['imu_sample']),'MPU sampling stopped')
                check(int(after['mag_sample'])>int(before['mag_sample']),'Magnetometer sampling stopped')
                before=after
                if (i+1)%5==0: note(f'{i+1}/20 regular switches passed')
            # Interrupt in-progress tile transfers. An even number returns to
            # the same face; the final settled image must be checked visually.
            for _ in range(10):
                connection.write(b'm'); time.sleep(0.06)
            after=wait_for(lambda r:int(r['switches'])>=int(before['switches'])+10)
            check(int(after['switches'])==int(before['switches'])+10,'Rapid switch count mismatch')
            check(after['mode']==before['mode'],'Rapid switches ended on wrong face')
            healthy(after,first)
            check(int(after['heap'])>=int(first['heap'])-1024,'Possible heap loss')
            check(int(after['frame'])>int(first['frame']),'No completed frames')
            elapsed=(int(after['up'])-int(first['up']))/1000
            note('PASS: 30 switches without sensor reinitialization or new errors; '
                 f'MPU={(int(after["imu_sample"])-int(first["imu_sample"]))/elapsed:.1f} Hz, '
                 f'magnetometer={(int(after["mag_sample"])-int(first["mag_sample"]))/elapsed:.1f} Hz; '
                 f'heap={after["heap"]}')
            if after['mode']!='horizon': connection.write(b'm')
            saved=wait_for(lambda r:r['mode']=='horizon' and r['mode_saved']=='1')
            healthy(saved,first)
            note('Restarting with Horizon saved')
            connection.write(b'r')
            reboot=wait_for(lambda r:int(r['up'])<int(saved['up']) and r['imu_fresh']=='1'
                            and r['mag_fresh']=='1',12)
            check(reboot['mode']=='horizon' and reboot['mode_saved']=='1','Last mode not restored')
            check(reboot['reset']=='3','Expected commanded software restart')
            check(reboot['offset']==first['offset'] and reboot['saved']=='1','Calibration changed')
            healthy(reboot,reboot)
            connection.write(b'm')
            final=wait_for(lambda r:r['mode']=='compass' and r['mode_saved']=='1')
            healthy(final,reboot)
            note('PASS: saved Horizon mode restored; returned to Compass and saved; calibration unchanged')
            note('Final: '+str(final))
        finally:
            connection.close()


if __name__=='__main__':
    main()
