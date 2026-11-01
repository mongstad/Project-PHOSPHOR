"""Offline aggregation of preserved raw measurements and unmodified captures."""
import csv,json,statistics
from pathlib import Path
import numpy as np
from PIL import Image
R=Path(__file__).resolve().parents[1]; E=R/'evidence'; F=E/'final-native'; C=E/'checks'
rows=list(csv.DictReader((F/'samples.csv').open())); assert len(rows)==112
result={'performance':[], 'runs':[], 'pixels':[]}
for res,size in [('1080p',(1920,1080)),('4k-buffer',(3840,2160))]:
 for path in ['cpu','gpu']:
  for workload,n in [('idle',12),('update',16)]:
   selected=[r for r in rows if r['run'].startswith(res) and r['run'].endswith(path) and r['workload']==workload];assert len(selected)==n
   result['performance'].append(dict(resolution=res,path=path,workload=workload,samples=n,**{k:statistics.mean(float(r[k]) for r in selected) for k in ['cpu_one_core_pct','rss_kib','whole_gpu_pct','power_w']}))
  for f in sorted(F.glob(res+'-r*-'+path+'.json')):
   m=json.loads(f.read_text()); assert (m['width'],m['height'])==size and m['dpr']==1 and m['cache_builds']==1
   w=json.loads(next(l[10:] for l in f.with_suffix('.log').read_text().splitlines() if l.startswith('WARM_IDLE ')))
   n=m['work_calls']-w['work_calls']
   result['runs'].append(dict(run=f.stem,cold_ms=m['cache_wall_ms'],first_frame_ms=m['first_frame_ms'],warm_calls=n,warm_work_ms=(m['work_wall_ms']-w['work_wall_ms'])/n,texture_bytes_per_update=(m['requested_texture_bytes']-w['requested_texture_bytes'])/n,geometry_bytes_per_update=(m['geometry_bytes']-w['geometry_bytes'])/n))
def compare(label,a,b):
 x=np.array(Image.open(a).convert('RGBA')).astype(np.int16);y=np.array(Image.open(b).convert('RGBA')).astype(np.int16);assert x.shape==y.shape,(a,b,x.shape,y.shape)
 d=np.abs(x-y);n=int(np.any(d,axis=2).sum());result['pixels'].append(dict(label=label,size=[x.shape[1],x.shape[0]],different_pixels=n,percent=100*n/(x.shape[0]*x.shape[1]),max_channel_delta=int(d.max()),mean_rgb_absolute_delta=d[:,:,:3].mean(axis=(0,1)).tolist()))
compare('accepted vs CPU',R.parent/'PHOSPHOR-002/refinement-16pt/evidence/refined-client.png',C/'refined-cpu-client.png')
for name in ['refined','off','resize','dpr1_5']:
 compare(name,C/f'{name}-cpu-client.png',C/f'{name}-gpu-client.png')
for res in ['1080p','4k-buffer']:
 for rep in [1,2]:compare(f'{res}-r{rep}',F/f'{res}-r{rep}-cpu-client.png',F/f'{res}-r{rep}-gpu-client.png')
compare('threaded',C/'refined-gpu-client.png',C/'threaded-gpu-client.png')
compare('UBSan 4k',F/'4k-buffer-r1-cpu-client.png',C/'ubsan-gpu-client.png')
for path in ['cpu','gpu']:Image.open(C/f'refined-{path}-client.png').crop((16,180,656,515)).save(C/f'refined-{path}-crop-100pct.png')
(E/'analysis.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
