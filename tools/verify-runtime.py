"""Read-only compatibility check of an independently obtained runtime; never loads DLLs."""
from pathlib import Path
import argparse,hashlib,json,sys
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('runtime_root',type=Path,help='Folder containing the h3sb subdirectory')
args=parser.parse_args();root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'docs/runtime-data-manifest.json').read_text())['files']
missing=[];different=[]
for name,expected in manifest.items():
    file=args.runtime_root/name
    if not file.is_file():missing.append(name);continue
    digest=hashlib.sha256()
    with file.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
    if digest.hexdigest().upper()!=expected:different.append(name)
if missing or different:
    print(json.dumps({'missing':missing,'different':different},indent=2));sys.exit(1)
print(f'PASS: {len(manifest)} separately obtained static runtime files match the accepted bundle. No binaries loaded or copied; this is not redistribution clearance.')
