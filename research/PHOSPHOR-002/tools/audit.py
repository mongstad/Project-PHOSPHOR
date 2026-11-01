"""Offline audit of committed captures, geometry, measurements and preservation."""
import csv, hashlib, json, re, subprocess
from pathlib import Path
from urllib.parse import unquote
from PIL import Image
root=Path(__file__).resolve().parents[3]
research=root/'research/PHOSPHOR-002'; evidence=research/'evidence'; native=evidence/'native'
for line in (evidence/'artwork.sha256').read_text().splitlines():
    digest,name=line.split('  ',1)
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
assert not subprocess.check_output(['git','diff','3adc5a451cd46ef92423304ff55a995a44ee44db','--','research/PHOSPHOR-001','prototypes/ghostty-molten-amber','Development_Plan.md'],cwd=root), 'historical record changed'
for tag,size in [('1080p',(1920,1080)),('physical-4k',(3840,2160))]:
    for work in ['idle','update']:
        for mode in ['baseline','material']:
            label=f'{tag}-{work}-{mode}'
            im=Image.open(native/f'{label}-client.png');assert im.size==size,(label,im.size)
            geom=json.loads((native/f'{label}-geometry.json').read_text());assert len(geom)==1
            assert (geom[0]['client']['width'],geom[0]['client']['height'])==size
            assert (native/f'{label}-native.png').exists()
            assert 'SUMMARY paints' in (native/f'{label}.log').read_text()
            assert im.getpixel((size[0]-1,0))[:3]==(7,5,3)
for mode in ['baseline','material']:
    parent=Image.open(native/f'1080p-idle-{mode}-client.png')
    crop=Image.open(native/f'14pt-{mode}-crop-100pct.png')
    assert crop.tobytes()==parent.crop((16,160,616,470)).tobytes()
for pt in [12,16]:
    for mode in ['baseline','material']:
        assert Image.open(native/f'{pt}pt-{mode}-client.png').size==(1280,900)
        assert (native/f'{pt}pt-{mode}-native.png').exists()
assert Image.open(native/'resize-dpr1_5-client.png').size==(1920,1350)
assert (native/'no-glow-client.png').exists()
rows=list(csv.DictReader((native/'samples.csv').open()));assert len(rows)==80
assert len({r['run'] for r in rows})==8
for name in {r['run'] for r in rows}:assert len([r for r in rows if r['run']==name])==10
before=json.loads((evidence/'displays-before.json').read_text())
after=json.loads((native/'displays-restored.json').read_text())
assert before['outputs']==after['outputs'],'display state not restored exactly'
for md in [research/'REPORT.md',research/'BUILD.md',research/'PHASE0.md',root/'prototypes/qt-molten-amber/README.md']:
    for target in re.findall(r'\]\(([^)]+)\)',md.read_text()):
        if '://' not in target:assert (md.parent/unquote(target.split('#')[0])).exists(),(md,target)
print('PASS: original artwork/history, 14 native test cases, 80 samples, pixel-exact crops, links, exact output restoration')
