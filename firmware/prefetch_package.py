"""Download an official PlatformIO mirror into its verified download cache."""
import concurrent.futures
import hashlib
import itertools
from pathlib import Path
import sys
import time
import requests
from platformio.registry.mirror import RegistryFileMirrorIterator

url = sys.argv[1]
cache = Path(__file__).parents[1] / '.platformio/.cache/downloads'
mirrors = list(itertools.islice(RegistryFileMirrorIterator(url), 3))
source, checksum = next(((u,h) for u,h in mirrors if 'github.com' in u), mirrors[0])
dest = cache / hashlib.sha1((mirrors[0][0] + checksum).encode()).hexdigest()
if dest.exists() and hashlib.sha256(dest.read_bytes()).hexdigest() == checksum:
    print('Already verified:', dest, flush=True)
    raise SystemExit(0)
partdir = cache / (dest.name + '.parts')
partdir.mkdir(exist_ok=True)
r = requests.get(source, headers={'Range':'bytes=0-0'}, stream=True, timeout=40)
if r.status_code != 206:
    raise RuntimeError(f'Range unsupported: {r.status_code}')
total = int(r.headers['Content-Range'].split('/')[-1])
resolved = r.url
r.close()
chunk = 2 * 1024 * 1024
count = (total + chunk - 1) // chunk
print(f'Download {total} bytes in {count} chunks from official mirror', flush=True)
def fetch(i):
    start=i*chunk; end=min(total,start+chunk)-1
    p=partdir/str(i)
    if p.exists() and p.stat().st_size == end-start+1:
        return i
    for attempt in range(4):
        try:
            # A query identifies the byte range for intermediate caches.
            sep='&' if '?' in resolved else '?'
            with requests.get(resolved+sep+f'part={i}',headers={'Range':f'bytes={start}-{end}'},timeout=(20,60)) as res:
                res.raise_for_status()
                if res.status_code != 206 or res.headers.get('Content-Range','').split('/')[0] != f'bytes {start}-{end}':
                    raise RuntimeError(f'Unexpected range: {res.status_code} {res.headers.get("Content-Range")}')
                data=res.content
            if len(data)!=end-start+1: raise RuntimeError('short chunk')
            p.write_bytes(data)
            return i
        except Exception:
            if attempt==3: raise
            time.sleep(1)
with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
    for n, f in enumerate(concurrent.futures.as_completed([pool.submit(fetch,i) for i in range(count)]),1):
        f.result()
        if n%8==0 or n==count: print(f'{n}/{count} chunks',flush=True)
tmp=dest.with_suffix('.assembling')
h=hashlib.sha256()
with tmp.open('wb') as out:
    for i in range(count):
        data=(partdir/str(i)).read_bytes(); out.write(data);h.update(data)
if h.hexdigest()!=checksum: raise RuntimeError('SHA256 mismatch')
tmp.replace(dest)
for p in partdir.iterdir(): p.unlink()
partdir.rmdir()
print('SHA256 verified:', checksum, flush=True)
print('Cached:', dest, flush=True)
