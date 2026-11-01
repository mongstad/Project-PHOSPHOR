"""Explicit owner-authorized native evidence run; no shell/PTY inside sample.
Run after enabling HDMI-A-1 at 3840x2160. Restores its recorded previous mode.
"""
import argparse, csv, json, os, subprocess, time
from pathlib import Path
parser=argparse.ArgumentParser(); parser.add_argument("--skip-1080",action="store_true"); parser.add_argument("--output-dir",type=Path); options=parser.parse_args()
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=options.output_dir or ROOT/'evidence/native'; OUT.mkdir(parents=True,exist_ok=True)
if (OUT/'samples.csv').exists() and not options.skip_1080:
    raise FileExistsError('Preserve existing evidence; choose a new --output-dir')
BIN='/tmp/phosphor-002/clang-build/phosphor-002'
env=dict(os.environ,QT_QPA_PLATFORM='wayland',QT_FORCE_STDERR_LOGGING='1',QSG_INFO='1',XDG_CACHE_HOME='/tmp/phosphor-002/runtime-cache')
HZ=os.sysconf('SC_CLK_TCK')
def ticks(pid):
    s=Path(f'/proc/{pid}/stat').read_text(); a=s[s.rfind(')')+2:].split(); return int(a[11])+int(a[12])
def run(label,args,bench=False,scale='1'):
    if (OUT/f'{label}-client.png').exists():
        raise FileExistsError(f'Capture already exists: {label}')
    with (OUT/f'{label}.log').open('w') as log:
        proc=subprocess.Popen([BIN,*args,'--seconds','24' if bench else '12','--capture',str(OUT/f'{label}-client.png')],env=dict(env,QT_SCALE_FACTOR=scale),stdout=log,stderr=log)
        try:
            # Wait for actual framebuffer capture, including synchronous cold cache.
            deadline=time.monotonic()+90
            while not (OUT/f'{label}-client.png').exists():
                if proc.poll() is not None: raise RuntimeError(f'{label} exited before capture')
                if time.monotonic()>deadline: raise TimeoutError(label)
                time.sleep(.2)
            geometry=subprocess.check_output(['python3',str(ROOT/'tools/kwin.py')],text=True)
            (OUT/f'{label}-geometry.json').write_text(geometry)
            if bench:
                before=time.monotonic(); previous=ticks(proc.pid)
                for index in range(10):
                    time.sleep(1)
                    now=time.monotonic(); current=ticks(proc.pid)
                    rss=next(int(l.split()[1]) for l in Path(f'/proc/{proc.pid}/status').read_text().splitlines() if l.startswith('VmRSS:'))
                    gpu=subprocess.check_output(['nvidia-smi','--query-gpu=utilization.gpu,power.draw,memory.used','--format=csv,noheader,nounits'],text=True).strip().split(',')
                    writer.writerow([label,index,now-before,100*(current-previous)/HZ/(now-before),rss,*[float(v) for v in gpu]]); samples.flush()
                    before=now; previous=current
            # Focus only this app before a compositor screenshot.
            subprocess.run(['python3',str(ROOT/'tools/kwin.py'),'--width','1288','--height','942'],check=True,stdout=subprocess.DEVNULL) if '--resize-test' in args else None
            # Fullscreen test windows are activated by their launch. The geometry
            # plus client capture is authoritative if active focus is changed.
            subprocess.run(['spectacle','-a','-b','-n','-o',str(OUT/f'{label}-native.png')],check=True,timeout=10,env=env)
            rc=proc.wait(timeout=60)
            if rc: raise RuntimeError(f'{label} exit {rc}')
            print(label,'complete',flush=True)
        finally:
            if proc.poll() is None: proc.terminate();proc.wait(timeout=10)
try:
    with (OUT/'samples.csv').open('a' if options.skip_1080 else 'w',newline='') as samples:
        writer=csv.writer(samples)
        if not options.skip_1080: writer.writerow(['run','sample','elapsed_s','cpu_one_core_pct','rss_kib','whole_gpu_pct','power_w','gpu_memory_mib'])
        for screen,tag in [('DP-2','1080p'),('HDMI-A-1','physical-4k')]:
            if options.skip_1080 and tag=='1080p': continue
            for work in ['idle','update']:
                for mode in ['baseline','material']:
                    args=['--screen',screen,'--fullscreen']
                    if mode=='baseline':args+=['--baseline']
                    if work=='update':args+=['--update-ms','16']
                    run(f'{tag}-{work}-{mode}',args,True)
        for pt in ['12','16']:
            for mode in ['baseline','material']:
                run(f'{pt}pt-{mode}',['--points',pt]+(['--baseline'] if mode=='baseline' else []))
        run('resize-dpr1_5',['--resize-test'],scale='1.5')
        run('no-glow',['--glow','0'])
finally:
    subprocess.run(['kscreen-doctor','output.HDMI-A-1.mirror.DP-1','output.HDMI-A-1.mode.69','output.HDMI-A-1.position.-2560,0','output.HDMI-A-1.disable'],check=True)
    (OUT/'displays-restored.json').write_text(subprocess.check_output(['kscreen-doctor','-j'],text=True))
