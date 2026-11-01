"""Matched native baseline/current runs; writes only issue evidence, no settings."""
import json, os, subprocess, time
from pathlib import Path
E=Path(__file__).resolve().parents[1]/'evidence'
env=dict(os.environ,QT_QPA_PLATFORM='wayland',QSG_RHI_BACKEND='opengl',QSG_RENDER_LOOP='basic',QT_FORCE_STDERR_LOGGING='1')
for name,binary in [('baseline','/tmp/phosphor-016-baseline/phosphor-terminal'),('current','/tmp/phosphor-016-build/phosphor-terminal')]:
    actions=[dict(ms=300,text="PS1='phosphor$ '; clear; printf 'PHOSPHOR scrollbar check\\n\\033[31mANSI red \\033[92mgreen\\033[0m\\n'"),dict(ms=500,key='Return'),dict(ms=1200,capture=str(E/f'{name}-short.png')),dict(ms=1500,metrics=str(E/f'{name}-idle-start.json')),dict(ms=3500,metrics=str(E/f'{name}-idle-end.json')),dict(ms=4000,text="python3 -c 'import time; [(print(i, flush=True), time.sleep(.002)) for i in range(2000)]'"),dict(ms=4200,key='Return'),dict(ms=4500,metrics=str(E/f'{name}-output-start.json')),dict(ms=7800,metrics=str(E/f'{name}-output-end.json')),dict(ms=9000,close=True)]
    path=E/f'{name}-actions.json';path.write_text(json.dumps(actions,indent=2)+'\n')
    samples=[]
    with (E/f'{name}.log').open('w') as log:
        p=subprocess.Popen([binary,'--clean-bash','--screen','DP-2','--test-script',str(path),'--seconds','15'],env=env,stdout=log,stderr=log)
        start=time.monotonic()
        while p.poll() is None:
            try:
                fields=Path(f'/proc/{p.pid}/stat').read_text().split(') ')[1].split()
                samples.append(dict(seconds=time.monotonic()-start,cpu_ticks=int(fields[11])+int(fields[12])))
            except FileNotFoundError: pass
            time.sleep(.25)
        assert p.returncode==0,p.returncode
    (E/f'{name}-cpu.json').write_text(json.dumps(dict(ticks_per_second=os.sysconf('SC_CLK_TCK'),samples=samples),indent=2)+'\n')
