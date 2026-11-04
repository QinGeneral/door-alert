"""Check the running board's color order and interval via USB serial."""
import json
from pathlib import Path
import re
import sys
import time
import serial

port = sys.argv[1]
output = Path(__file__).parent / 'verification'
output.mkdir(exist_ok=True)
s = serial.Serial(port=None, baudrate=115200, timeout=0.4)
s.dtr = False
s.rts = False
s.port = port
s.open()
events=[]
raw=[]
deadline=time.monotonic()+14
pattern=re.compile(r'LIGHT=(RED|YELLOW|GREEN) GPIO=(\d+) UPTIME_MS=(\d+) CYCLE=(\d+)')
try:
    while time.monotonic()<deadline and len(events)<10:
        line=s.readline().decode('utf-8',errors='replace').strip()
        if not line: continue
        raw.append(line)
        m=pattern.search(line)
        if m:
            color,pin,ms,cycle=m.groups()
            events.append(dict(color=color,gpio=int(pin),uptime_ms=int(ms),cycle=int(cycle)))
            print(line,flush=True)
finally:
    s.close()
(output/'serial.log').write_text('\n'.join(raw)+'\n')
assert len(events)>=9, f'Not enough events: {len(events)}'
order=['RED','YELLOW','GREEN']
pins={'RED':21,'YELLOW':20,'GREEN':10}
for e in events:
    assert e['gpio']==pins[e['color']], e
for a,b in zip(events,events[1:]):
    assert order[(order.index(a['color'])+1)%3]==b['color'], (a,b)
    assert 950<=b['uptime_ms']-a['uptime_ms']<=1100, (a,b)
report=dict(port=port,passed=True,events=events,checks=['color_order','pin_mapping','1000ms_interval'],boundary='Serial output verified; emitted light not observed by host.')
(output/'result.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: color order, GPIO mapping and 1-second interval',flush=True)
