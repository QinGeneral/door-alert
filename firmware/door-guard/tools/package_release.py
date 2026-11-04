"""Package the actual build outputs; never prints or exports pairing key source."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

root=Path(__file__).resolve().parents[1]
release=root/'release';release.mkdir(exist_ok=True)
packages=root.parents[1]/'.platformio/packages'
manifest={
 'version':'door-guard-1.0',
 'boards':{},
 'driver_commit':'248607060e49d18bdf064c6aa411ffe515993d16',
 'default_mode':'DESK, <=1000mm adjacent foreground',
 'unverified':['Installed door coverage and calibration','Battery power sleep/wake and endurance'],
 'acceptance_notes':'See verification/ACCEPTANCE.md for current hardware status; binaries are not a physical installation certification.'
}
for role,mac in [('front','ac:27:6e:7f:d3:20'),('rear','ac:27:6e:7f:c3:b4')]:
 build=root/'.pio/build'/role
 cmd=[str(packages/'tool-esptool/esptool'),'--chip','esp32c3','merge-bin','-o',str(release/(role+'.bin')),'--flash-mode','dio','--flash-freq','80m','--flash-size','4MB','0x0',str(build/'bootloader.bin'),'0x8000',str(build/'partitions.bin'),'0xe000',str(packages/'framework-arduinoespressif32/tools/partitions/boot_app0.bin'),'0x10000',str(build/'firmware.bin')]
 result=subprocess.run(cmd,capture_output=True,text=True)
 if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 image=(release/(role+'.bin')).read_bytes();application=(build/'firmware.bin').read_bytes()
 assert image[0x10000:0x10000+len(application)]==application
 manifest['boards'][role]={'mac':mac,'file':role+'.bin','flash_offset':'0x0','size':len(image),'sha256':hashlib.sha256(image).hexdigest(),'application_sha256':hashlib.sha256(application).hexdigest()}
for name,src in [('rear-upload.log','/private/tmp/door-rear-final-upload.log'),('front-upload.log','/private/tmp/door-front-final-upload.log'),('build.log','/private/tmp/door-final-build.log')]:
 if Path(src).exists():shutil.copyfile(src,release/name)
(release/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Release images match application bytes; manifest saved.')
