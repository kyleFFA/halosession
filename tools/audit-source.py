"""Inspect the staged source payload. Does not build, launch, inject or contact GitHub."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
import xml.etree.ElementTree as ET

root=Path(__file__).resolve().parents[1]
def git(*args):
    return subprocess.run(['git','-C',str(root),*args],capture_output=True,check=True).stdout
names=[n.decode('utf-8') for n in git('ls-files','-z').split(b'\0') if n]
if not names:raise SystemExit('No staged/tracked files; review and add the source first.')
staged={}
requests=[]
for entry in git('ls-files','--stage','-z').split(b'\0'):
    if not entry:continue
    metadata,name=entry.split(b'\t',1);mode,oid,stage=metadata.split()
    if mode==b'160000':raise SystemExit('Unmaterialized Git submodule in staged payload.')
    staged[name.decode('utf-8')]=oid.decode();requests.append(oid)
batch=subprocess.run(['git','-C',str(root),'cat-file','--batch'],input=b'\n'.join(requests)+b'\n',capture_output=True,check=True).stdout
blobs={};offset=0
for oid in requests:
    end=batch.index(b'\n',offset);header=batch[offset:end].split();size=int(header[2]);offset=end+1
    if header[1]!=b'blob':raise SystemExit('Unexpected non-blob staged object.')
    blobs[oid.decode()]=batch[offset:offset+size];offset+=size+1
if git('diff','--name-only').strip():raise SystemExit('Unstaged edits exist; review and stage them before auditing the upload set.')
forbidden={'.exe','.dll','.pyd','.lib','.obj','.o','.a','.pdb','.exp','.idb','.ilk','.iobj','.ipdb','.pch','.res','.zip','.7z','.msi','.bin','.dmp','.mdmp','.log','.etl','.pyc','.lnk','.pem','.pfx','.p12','.key'}
runtime_names={'HCMInternalConfig.xml','HCMHotkeyConfig.xml','SessionQuickMenu.xml','h3sb_helpers.stop'}
assets={'HCMInternal/Assets/session-ring.png','HCMInternal/Assets/session-clouds.png',
        'HCMInternal/Assets/session-ring-circular.png','HCMInternal/Assets/session-ringworld-round.png',
        *{f'skins/{skin}/{layer}.png' for skin in ['garden','aurora','autumn'] for layer in ['ring','clouds']}}
patterns={
    'personal_path':re.compile(rb'(?i)[a-z]:[\\/](?:users|steamlibrary)[\\/]|/(?:Users|home)/[^\s"\']+'),
    'github_token':re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{40,})'),
    'openai_key':re.compile(rb'sk-(?:proj-)?[A-Za-z0-9_-]{30,}'),
    'aws_access_key':re.compile(rb'\bAKIA[0-9A-Z]{16}\b'),
    'private_key':re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----')}
failures=[];manifest={};total=0
for name in names:
    path=root/name
    if path.is_symlink():failures.append([name,'symlink']);continue
    data=blobs[staged[name]];total+=len(data)
    if path.suffix.lower() in forbidden or path.name in runtime_names:failures.append([name,'excluded file type/runtime state'])
    if data[:2]==b'MZ' or data[:4] in [b'\x7fELF',b'PK\x03\x04']:failures.append([name,'compiled/archive signature'])
    if name not in assets:
        for label,pattern in patterns.items():
            if pattern.search(data):failures.append([name,label])
        if b'\0' in data:failures.append([name,'unexpected binary data'])
    elif not data.startswith(b'\x89PNG\r\n\x1a\n'):failures.append([name,'invalid source asset'])
    manifest[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest().upper()}
for entry in git('ls-files','--stage').decode().splitlines():
    if entry.startswith('160000 '):failures.append([entry.split('\t')[-1],'unmaterialized submodule'])
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'};compile_units={};project_inputs=[]
for project in ['HCMInternal','HCMInterproc','HCMSpeedhack']:
    file=root/project/(project+'.vcxproj');tree=ET.parse(file)
    units=[n.attrib['Include'] for n in tree.findall('.//m:ClCompile[@Include]',ns)];compile_units[project]=len(units)
    for kind in ['ClCompile','ClInclude','ResourceCompile']:
        for n in tree.findall('.//m:'+kind+'[@Include]',ns):
            value=n.attrib['Include']
            if '$(' not in value and not (file.parent/value).is_file():failures.append([project+'/'+value,'missing '+kind])
            else:project_inputs.append(project+'/'+value)
for file in [root/'HCMInternal/HCMInternal.rc',root/'tools/ui-test.rc']:
    for value in re.findall(r'^\d+\s+RCDATA\s+"([^"]+)"',file.read_text(),re.M):
        value=value.replace('\\\\','\\');base=file.parent if file.parent.name=='HCMInternal' else root
        if not (base/value).is_file():failures.append([str(file.relative_to(root)),'missing source asset '+value])
for required in ['LICENSE','THIRD-PARTY-LICENSES.md','NOTICE','licenses/ArticleEnumClass-BSD-2-Clause.txt','HCMInternal/includes/safetyhook/LICENSE','HCMSpeedhack/includes/safetyhook/LICENSE','HCMInternal/includes/type_safe/LICENSE','HCMInternal/includes/ImGuiColorTextEdit/LICENSE']:
    if required not in names:failures.append([required,'missing required notice'])
report={'files':len(names),'bytes':total,'compile_units':compile_units,'failures':failures,'manifest':manifest,
    'hash_scope':'Exact staged Git blobs after text line-ending normalization',
    'scope':'Static file/signature/pattern and project/resource/notice checks; not proof that every possible secret is absent, a full rebuild or a runtime/license clearance.'}
out=root/'out';out.mkdir(exist_ok=True);(out/'source-audit.json').write_text(json.dumps(report,indent=2))
if failures:
    print(json.dumps({'failures':failures},indent=2));sys.exit(1)
print(f'PASS: {len(names)} source files, {total:,} bytes; no excluded payload/signatures or credential/personal-path pattern matches; project/resource/license closure checked.')
