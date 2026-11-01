"""Matched 16 pt native captures. No display settings are changed."""
import os, subprocess, time
from pathlib import Path
ROOT=Path(__file__).resolve().parent
out=ROOT/'evidence'
env=dict(os.environ,QT_QPA_PLATFORM='wayland',QT_SCALE_FACTOR='1',QT_FORCE_STDERR_LOGGING='1',QSG_INFO='1',XDG_CACHE_HOME='/tmp/phosphor-002/runtime-cache')
for variant,extra in [('original-current',[]),('refined',['--refine-16'])]:
    image=out/f'{variant}-client.png'
    if image.exists(): raise FileExistsError(image)
    with (out/f'{variant}.log').open('w') as log:
        proc=subprocess.Popen(['/tmp/phosphor-002/clang-build/phosphor-002','--points','16','--screen','DP-1','--width','1280','--height','900','--seconds','14','--capture',str(image),*extra],env=env,stdout=log,stderr=log)
        try:
            time.sleep(3)
            helper=ROOT.parent/'tools/kwin.py'
            subprocess.run(['python3',str(helper),'--width','1288','--height','942','--x','100','--y','80'],check=True,stdout=subprocess.DEVNULL)
            deadline=time.monotonic()+20
            while not image.exists():
                if proc.poll() is not None or time.monotonic()>deadline: raise RuntimeError('capture failed')
                time.sleep(.2)
            (out/f'{variant}-geometry.json').write_bytes(subprocess.check_output(['python3',str(helper)]))
            subprocess.run(['spectacle','-a','-b','-n','-o',str(out/f'{variant}-native.png')],check=True,env=env)
            assert proc.wait(timeout=20)==0
        finally:
            if proc.poll() is None: proc.terminate();proc.wait()
    print(variant,'complete',flush=True)
