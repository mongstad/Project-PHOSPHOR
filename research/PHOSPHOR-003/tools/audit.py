"""Validate preservation and final evidence; run after analyze.py."""
import hashlib,json,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[1]; E=R/'evidence'; repo=R.parents[1]
for line in (E/'baseline.sha256').read_text().splitlines():
 digest,name=line.split('  ',1);assert hashlib.sha256((repo/name).read_bytes()).hexdigest()==digest,name
for path in ['prototypes/qt-molten-amber','research/PHOSPHOR-001','research/PHOSPHOR-002']:
 assert not subprocess.check_output(['git','diff','54b0260','--',path],cwd=repo),path
before=json.loads((E/'displays-before.json').read_text());after=json.loads((E/'displays-after.json').read_text());assert before==after,'display configuration changed'
a=json.loads((E/'analysis.json').read_text());assert len(a['runs'])==8 and len(a['performance'])==8
for p in a['pixels']:
 if p['label'] in ['accepted vs CPU','refined','off','resize','1080p-r1','1080p-r2','threaded']: assert p['different_pixels']==0,p
 if '4k' in p['label']:assert p['max_channel_delta']<=1,p
for name in ['refined','off','resize','dpr1_5']:
 for path in ['cpu','gpu']:
  m=json.loads((E/f'checks/{name}-{path}.json').read_text()); assert m['cache_builds']==1,(name,path,m)
assert json.loads((E/'checks/ubsan-gpu.json').read_text())['width']==3840
for p in (E/'final-native').glob('*-client.png'):
 assert p.stat().st_size>0
print('PASS preserved artwork/source/evidence hashes; unchanged display state; 8 final runs; matched captures; one cache across resize; native UBSan 4K completion')
