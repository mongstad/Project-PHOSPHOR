"""Native DP-only paired benchmark. Does not configure monitor modes.
Uses exact client geometry via temporary KWin script; output refuses overwrites.
"""
import csv,json,os,subprocess,time,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=ROOT/'evidence/final-native');a=p.parse_args()
OUT=a.out;OUT.mkdir(parents=True,exist_ok=True)
if (OUT/'samples.csv').exists():raise FileExistsError('Choose a fresh --out; preserve evidence')
ENV=dict(os.environ,QT_QPA_PLATFORM='wayland',QT_SCALE_FACTOR='1',QT_FORCE_STDERR_LOGGING='1',QSG_INFO='1',QSG_RENDER_LOOP='basic',XDG_CACHE_HOME='/tmp/phosphor-003/cache')
BIN='/tmp/phosphor-003/clang-build/phosphor-003';HZ=os.sysconf('SC_CLK_TCK')
def ticks(pid):
 s=Path(f'/proc/{pid}/stat').read_text();v=s[s.rfind(')')+2:].split();return int(v[11])+int(v[12])
def geometry(*args):return json.loads(subprocess.check_output(['python3',str(ROOT/'tools/kwin.py'),*args],text=True))
def sample(proc,label,work,count):
 g=geometry('--no-focus'); assert g[0]['client']['width']==width and g[0]['client']['height']==height,g
 before=time.monotonic();prev=ticks(proc.pid)
 for i in range(count):
  time.sleep(1);now=time.monotonic();current=ticks(proc.pid)
  rss=next(int(l.split()[1]) for l in Path(f'/proc/{proc.pid}/status').read_text().splitlines() if l.startswith('VmRSS:'))
  gpu=[float(x) for x in subprocess.check_output(['nvidia-smi','--query-gpu=utilization.gpu,power.draw,memory.used','--format=csv,noheader,nounits'],text=True).strip().split(',')]
  writer.writerow([label,work,i,now-before,100*(current-prev)/HZ/(now-before),rss,*gpu]);csvfile.flush();before=now;prev=current
 g=geometry('--no-focus'); assert g[0]['client']['width']==width and g[0]['client']['height']==height,g
 (OUT/f'{label}-{work}-end-geometry.json').write_text(json.dumps(g,indent=2))
with (OUT/'samples.csv').open('w',newline='') as csvfile:
 writer=csv.writer(csvfile);writer.writerow(['run','workload','sample','elapsed_s','cpu_one_core_pct','rss_kib','whole_gpu_pct','power_w','gpu_memory_mib'])
 for width,height,screen,tag in [(1920,1080,'DP-2','1080p'),(3840,2160,'DP-1','4k-buffer')]:
  for rep in [1,2]:
   for path in (['cpu','gpu'] if rep==1 else ['gpu','cpu']):
    label=f'{tag}-r{rep}-{path}';logpath=OUT/f'{label}.log';capture=OUT/f'{label}-client.png'
    args=[BIN,'--path',path,'--screen',screen,'--width',str(width),'--height',str(height),'--fixed-size','--seconds','30','--update-after-ms','16000','--capture',str(capture),'--metrics',str(OUT/f'{label}.json')]
    if tag=='1080p':args+=['--fullscreen']
    with logpath.open('w') as log:
     proc=subprocess.Popen(args,env=ENV,stdout=log,stderr=log)
     try:
      time.sleep(3)
      if tag=='4k-buffer':geometry('--width',str(width+8),'--height',str(height+42),'--x','100','--y','80')
      deadline=time.monotonic()+20
      while not capture.exists():
       if proc.poll() is not None or time.monotonic()>deadline:raise RuntimeError(f'{label}: no capture')
       time.sleep(.1)
      g=geometry();assert len(g)==1 and g[0]['client']['width']==width and g[0]['client']['height']==height,g
      (OUT/f'{label}-geometry.json').write_text(json.dumps(g,indent=2))
      sample(proc,label,'idle',6)
      subprocess.run(['spectacle','-a','-b','-n','-o',str(OUT/f'{label}-native.png')],env=ENV,check=True,timeout=10)
      deadline=time.monotonic()+10
      while 'WARM_IDLE' not in logpath.read_text():
       if proc.poll() is not None or time.monotonic()>deadline:raise RuntimeError(f'{label}: no update start')
       time.sleep(.1)
      time.sleep(2);sample(proc,label,'update',8)
      assert proc.wait(timeout=15)==0
      final=json.loads((OUT/f'{label}.json').read_text());assert (final['width'],final['height'])==(width,height),final
     finally:
      if proc.poll() is None:proc.terminate();proc.wait(timeout=10)
    print(label,'complete',flush=True)
print('COMPLETE: 8 runs, 48 idle + 64 warm update samples; no monitor modes changed',flush=True)
