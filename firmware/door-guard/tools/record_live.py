"""Capture real USB telemetry from both boards without altering sensor data."""
import argparse
import json
from pathlib import Path
import time
import serial

p=argparse.ArgumentParser()
p.add_argument('--front',default='/dev/cu.usbmodem101')
p.add_argument('--rear',default='/dev/cu.usbmodem2101')
p.add_argument('--seconds',type=float,default=45)
p.add_argument('--name',default='integration')
a=p.parse_args()
root=Path(__file__).resolve().parents[1]/'verification'
root.mkdir(exist_ok=True)
ports={};buffers={};last={};records=[]
try:
 for role,port in [('front',a.front),('rear',a.rear)]:
  if port=='none':continue
  s=serial.Serial(port=None,baudrate=115200,timeout=0)
  s.dtr=False;s.rts=False;s.port=port;s.open()
  ports[role]=s;buffers[role]=bytearray()
 start=time.monotonic()
 with (root/(a.name+'.jsonl')).open('w') as f:
  while time.monotonic()-start<a.seconds:
   for role,s in ports.items():
    data=s.read(s.in_waiting or 1)
    if not data:continue
    buffers[role].extend(data)
    while b'\n' in buffers[role]:
     line,_,rest=buffers[role].partition(b'\n');buffers[role]=bytearray(rest)
     try:r=json.loads(line.decode(errors='replace'))
     except (ValueError,TypeError):continue
     r['source_port']=s.port;r['host_elapsed']=round(time.monotonic()-start,3)
     f.write(json.dumps(r)+'\n');f.flush();records.append(r)
     marker=(r.get('state'),r.get('display'),r.get('remote'),r.get('event'))
     if marker!=last.get(role):print(json.dumps(r),flush=True);last[role]=marker
   time.sleep(.01)
finally:
 for s in ports.values():s.close()
front=[r for r in records if r.get('role')=='front' and 'display' in r]
rear=[r for r in records if r.get('role')=='rear' and 'state' in r]
for r in front:
 assert r['r']+r['y']+r['g']<=1,r
 if r['g']:assert r['fresh'] and r['remote']=='CLEAR',r
 if r['r']:assert r['fresh'] and r['remote']=='OCCUPIED',r
 if not r['fresh']:assert r['g']==0 and r['r']==0,r
summary={
 'front_records':len(front),'rear_records':len(rear),
 'front_displays':sorted({r['display'] for r in front}),
 'rear_states':sorted({r['state'] for r in rear}),
 'fresh_status_received':any(r['fresh'] for r in front),
 'front_rx_max':max((r['rx'] for r in front),default=0),
 'rear_rx_max':max((r['rx'] for r in rear),default=0),
 'front_pir_values':sorted({r['pir'] for r in front}),
 'rear_frame_max':max((r['frames'] for r in rear),default=0),
 'green_safety_invariants_passed':bool(front),
 'boundary':'USB-connected desktop test; physical installation and battery sleep not tested.'
}
(root/(a.name+'-result.json')).write_text(json.dumps(summary,indent=2)+'\n')
print('SUMMARY '+json.dumps(summary),flush=True)
