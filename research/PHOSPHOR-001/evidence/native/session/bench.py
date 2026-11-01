import subprocess,os,time,json,csv,signal,statistics
from pathlib import Path
ROOT=Path('/tmp/phosphor-001-native');SESSION=ROOT/'session';OUT=ROOT/'results';REPO=Path('/home/Nozba/Codex/Project-PHOSPHOR/Project-PHOSPHOR');HZ=os.sysconf('SC_CLK_TCK')
env=dict(os.environ,GDK_BACKEND='wayland',GHOSTTY=str(ROOT/'install/bin/ghostty'),LD_LIBRARY_PATH=str(ROOT/'sysroot/usr/lib64'))
def ticks(pid):
 text=Path(f'/proc/{pid}/stat').read_text();parts=text[text.rfind(')')+2:].split();return int(parts[11])+int(parts[12])
def write(p,mode):p.write_text(json.dumps({'mode':mode}))
def geometry(width=None,height=None):
 cmd=['python3',str(SESSION/'kwin.py')]
 if width:cmd+=['--width',str(width),'--height',str(height)]
 return json.loads(subprocess.check_output(cmd,text=True))
progress=OUT/'progress.jsonl'
with (OUT/'samples.csv').open('w',newline='') as f:
 writer=csv.DictWriter(f,fieldnames=['surface','rep','mode','workload','sample','elapsed_s','cpu_one_core_pct','rss_kib','gpu_pct','power_w','gpu_memory_mib']);writer.writeheader();f.flush()
 for width,height in [(1920,1080),(3840,2160)]:
  for rep in range(1,4):
   for mode in (['baseline','prototype'] if rep%2 else ['prototype','baseline']):
    label=f'{width}x{height}-r{rep}-{mode}';folder=OUT/label;folder.mkdir(exist_ok=True);control=folder/'control.json';write(control,'idle')
    log=(folder/'ghostty.log').open('w')
    proc=subprocess.Popen(['bash',str(REPO/'prototypes/ghostty-molten-amber/run.sh'),mode,'--','python3','-B',str(SESSION/'workload.py'),str(folder)],env=env,stdout=log,stderr=log,start_new_session=True)
    try:
     deadline=time.monotonic()+15
     while not (folder/'state.json').exists():
      if proc.poll() is not None:raise RuntimeError(f'{label} exited {proc.returncode}')
      if time.monotonic()>deadline:raise RuntimeError('window startup timeout')
      time.sleep(.1)
     geometry(width+8,height+42);time.sleep(1.2);g=geometry();assert len(g)==1,g
     gp=int(g[0]['pid']);assert g[0]['client']['width']==width and g[0]['client']['height']==height,g
     (folder/'geometry.json').write_text(json.dumps(g,indent=2));time.sleep(8.8)
     with progress.open('a') as p:p.write(json.dumps({'event':'start','run':label,'pid':gp})+'\n')
     for work in ['idle','scroll']:
      write(control,work);time.sleep(.5)
      prev_t=time.monotonic();prev_ticks=ticks(gp)
      for sample in range(30):
       due=prev_t+1;time.sleep(max(0,due-time.monotonic()))
       now=time.monotonic();current=ticks(gp)
       stats=Path(f'/proc/{gp}/status').read_text().splitlines();rss=next(int(x.split()[1]) for x in stats if x.startswith('VmRSS:'))
       raw=subprocess.check_output(['nvidia-smi','--query-gpu=utilization.gpu,power.draw,memory.used','--format=csv,noheader,nounits'],text=True).strip();gpu,power,memory=[float(x.strip()) for x in raw.split(',')]
       writer.writerow(dict(surface=f'{width}x{height}',rep=rep,mode=mode,workload=work,sample=sample,elapsed_s=now-prev_t,cpu_one_core_pct=(current-prev_ticks)/HZ/(now-prev_t)*100,rss_kib=rss,gpu_pct=gpu,power_w=power,gpu_memory_mib=memory));f.flush();prev_t=now;prev_ticks=current
      with progress.open('a') as p:p.write(json.dumps({'event':'workload_done','run':label,'workload':work})+'\n')
     (folder/'final-state.json').write_text((folder/'state.json').read_text())
    finally:
     write(control,'exit')
     try:proc.wait(timeout=5)
     except subprocess.TimeoutExpired:os.killpg(proc.pid,signal.SIGTERM);proc.wait(timeout=5)
     log.close()
     with progress.open('a') as p:p.write(json.dumps({'event':'exit','run':label,'returncode':proc.returncode})+'\n')
print('COMPLETE: 12 paired native-window runs, 720 one-second samples',flush=True)
