"""1080p physical DP-2: idle and sustained PTY output; two runs.
The prior fixture GPU binary is a contextual baseline, not identical PTY work.
"""
import csv,json,os,subprocess,time
from pathlib import Path
R=Path(__file__).resolve().parents[1];E=R/'evidence/performance-final';E.mkdir(exist_ok=True)
assert not (E/'samples.csv').exists(),'Preserve existing runs'
ENV=dict(os.environ,QT_QPA_PLATFORM='wayland',QT_SCALE_FACTOR='1',QT_FORCE_STDERR_LOGGING='1',QSG_INFO='1',QSG_RENDER_LOOP='basic',XDG_CACHE_HOME='/tmp/phosphor-004/cache',PS1='phosphor$ ')
HZ=os.sysconf('SC_CLK_TCK')
def ticks(pid):
 s=Path(f'/proc/{pid}/stat').read_text();v=s[s.rfind(')')+2:].split();return int(v[11])+int(v[12])
def samples(proc,label,work,n):
 prev=ticks(proc.pid);before=time.monotonic()
 for i in range(n):
  time.sleep(1);now=time.monotonic();current=ticks(proc.pid)
  rss=next(int(s.split()[1]) for s in Path(f'/proc/{proc.pid}/status').read_text().splitlines() if s.startswith('VmRSS:'))
  gpu=[float(x) for x in subprocess.check_output(['nvidia-smi','--query-gpu=utilization.gpu,power.draw,memory.used','--format=csv,noheader,nounits'],text=True).strip().split(',')]
  writer.writerow([label,work,i,now-before,100*(current-prev)/HZ/(now-before),rss,*gpu]);f.flush();prev=current;before=now
# Fixed repeatable ASCII load: 20 lines x 140 columns per 16 ms batch, 10 s.
producer=E/'producer.py';producer.write_text("import sys,time\nend=time.monotonic()+10\nn=0\nwhile time.monotonic()<end:\n sys.stdout.write(''.join((f'[{n+i:06d}] glyph.material /tmp/sample.cpp x=0.125 y=3.750 OK '*3)[:140]+'\\n' for i in range(20)))\n sys.stdout.flush();n+=20;time.sleep(.016)\n")
with (E/'samples.csv').open('w',newline='') as f:
 writer=csv.writer(f);writer.writerow(['run','workload','sample','elapsed_s','cpu_one_core_pct','rss_kib','whole_gpu_pct','power_w','gpu_memory_mib'])
 for rep,path in [(1,'terminal'),(1,'fixture'),(2,'fixture'),(2,'terminal')]:
  label=f'{path}-r{rep}';logpath=E/f'{label}.log'
  if path=='terminal':
   actions=[{'ms':700,'text':"clear; printf 'PHOSPHOR IDLE\\n'"},{'ms':730,'key':'Return'},{'ms':1500,'metrics':str(E/f'{label}-idle-start.json')},{'ms':9000,'text':f"python3 '{producer}'"},{'ms':9030,'key':'Return'},{'ms':10000,'metrics':str(E/f'{label}-update-start.json')},{'ms':20000,'capture':str(E/f'{label}-client.png')},{'ms':18000,'metrics':str(E/f'{label}-update-end.json')},{'ms':21000,'text':'exit'},{'ms':21030,'key':'Return'}]
   script=E/f'{label}-actions.json';script.write_text(json.dumps(actions,indent=2))
   cmd=['/tmp/phosphor-004/clang-build/phosphor-terminal','--clean-bash','--screen','DP-2','--fullscreen','--width','1920','--height','1080','--seconds','25','--test-script',str(script),'--metrics',str(E/f'{label}.json')]
  else:
   cmd=['/tmp/phosphor-003/clang-build/phosphor-003','--path','gpu','--screen','DP-2','--fullscreen','--width','1920','--height','1080','--seconds','23','--update-after-ms','10000','--metrics',str(E/f'{label}.json')]
  with logpath.open('w') as log:
   proc=subprocess.Popen(cmd,env=ENV,stdout=log,stderr=log)
   try:
    deadline=time.monotonic()+15
    while 'FIRST_FRAME_MS' not in logpath.read_text():
     assert proc.poll() is None and time.monotonic()<deadline
     time.sleep(.05)
    first=time.monotonic();time.sleep(2);samples(proc,label,'idle',5)
    # Both workloads are active by 10.5 s after first frame.
    time.sleep(max(0,first+10.5-time.monotonic()));samples(proc,label,'output',6)
    assert proc.wait(timeout=15)==0
    m=json.loads((E/f'{label}.json').read_text());assert (m['width'],m['height'],m['dpr'])==(1920,1080,1),m
   finally:
    if proc.poll() is None:proc.terminate();proc.wait(timeout=5)
  print(label,'complete',flush=True)
