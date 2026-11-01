"""Offline preservation, measured-data and screenshot checks."""
import csv,hashlib,json,statistics,subprocess
from pathlib import Path
import numpy as np
from PIL import Image
R=Path(__file__).resolve().parents[1];E=R/'evidence';repo=R.parents[1];P=E/'performance-final'
for line in (repo/'research/PHOSPHOR-003/evidence/baseline.sha256').read_text().splitlines():
 digest,path=line.split('  ',1);assert hashlib.sha256((repo/path).read_bytes()).hexdigest()==digest,path
for path in ['research/PHOSPHOR-001','research/PHOSPHOR-002','research/PHOSPHOR-003','prototypes/qt-molten-amber','prototypes/qt-gpu-atlas']:
 assert not subprocess.check_output(['git','diff','2109428','--',path],cwd=repo),path
rows=list(csv.DictReader((P/'samples.csv').open()));assert len(rows)==44
results={'performance':[],'warm_deltas':[],'visual':[]}
for path in ['terminal','fixture']:
 for workload,n in [('idle',10),('output',12)]:
  selected=[r for r in rows if r['run'].startswith(path) and r['workload']==workload];assert len(selected)==n
  results['performance'].append(dict(path=path,workload=workload,samples=n,**{k:statistics.mean(float(r[k]) for r in selected) for k in ['cpu_one_core_pct','rss_kib','whole_gpu_pct','power_w']}))
 for rep in [1,2]:
  m=json.loads((P/f'{path}-r{rep}.json').read_text());assert (m['width'],m['height'],m['dpr'])==(1920,1080,1)
for rep in [1,2]:
 a=json.loads((P/f'terminal-r{rep}-update-start.json').read_text());b=json.loads((P/f'terminal-r{rep}-update-end.json').read_text());d={k:b[k]-a[k] for k in ['elapsed_ms','read_bytes','parse_ms','render_calls','render_work_ms','texture_bytes']};assert d['texture_bytes']==0;results['warm_deltas'].append(dict(rep=rep,**d))
ref=repo/'research/PHOSPHOR-002/refinement-16pt/evidence/refined-client.png'
for name in ['matched','final-matched']:
 a=np.array(Image.open(ref)).astype(np.int16);b=np.array(Image.open(E/f'{name}-client.png')).astype(np.int16);assert a.shape==b.shape;delta=np.abs(a-b);n=int(np.any(delta,axis=2).sum());assert n==0
 results['visual'].append(dict(name=name,different_pixels=n,max_channel_delta=int(delta.max()),size=[a.shape[1],a.shape[0]]))
Image.open(ref).crop((16,180,656,515)).save(E/'accepted-crop-100pct.png')
Image.open(E/'final-matched-client.png').crop((16,180,656,515)).save(E/'terminal-crop-100pct.png')
assert 'INTERRUPT_OK\n' in (E/'editing-screen.txt').read_text().replace(' ','')
assert '\nhello\n' in (E/'editing-screen.txt').read_text().replace(' ','')
assert '16 59' in (E/'resize-screen.txt').read_text()
assert json.loads((E/'scrollback-snapshot.json').read_text())['history_lines']==1000
assert json.loads((E/'scrollback-snapshot.json').read_text())['scroll_offset']==25
assert '/tmp/phosphor-004/completion-only' in (E/'final-keys-screen.txt').read_text()
assert '\nfinal\n' in (E/'final-keys-screen.txt').read_text().replace(' ','')
assert 'SHELL_EXIT 0' in (E/'ctrl-d.log').read_text()
for name in ['final-keys','final-ubsan','final-matched']:
 log=(E/f'{name}.log').read_text();assert 'Failed to build graphics' not in log and 'No GLSL' not in log
before=json.loads((E/'displays.json').read_text());after=json.loads((E/'displays-after.json').read_text());assert before==after
(E/'analysis.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS preservation hashes/history; 44 final samples; matched 1080p metadata; zero warm texture upload; accepted full image exact twice; real command/readline/resize/history/Tab/Backspace/Ctrl+D checks; unchanged displays')
