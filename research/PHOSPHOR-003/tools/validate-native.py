"""Supplemental real-window captures after the benchmark (no monitor changes)."""
import json,os,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'evidence/checks';OUT.mkdir(parents=True,exist_ok=True)
ENV=dict(os.environ,QT_QPA_PLATFORM='wayland',QT_SCALE_FACTOR='1',QT_FORCE_STDERR_LOGGING='1',QSG_INFO='1',QSG_RENDER_LOOP='basic',XDG_CACHE_HOME='/tmp/phosphor-003/cache')
CASES=[('refined-cpu','cpu',[],{}),('refined-gpu','gpu',[],{}),('off-cpu','cpu',['--effects-off'],{}),('off-gpu','gpu',['--effects-off'],{}),('resize-cpu','cpu',['--resize-test'],{}),('resize-gpu','gpu',['--resize-test'],{}),('dpr1_5-cpu','cpu',[],{'QT_SCALE_FACTOR':'1.5'}),('dpr1_5-gpu','gpu',[],{'QT_SCALE_FACTOR':'1.5'}),('ubsan-gpu','gpu',['--width','3840','--height','2160','--fixed-size'],{}),('threaded-gpu','gpu',[],{'QSG_RENDER_LOOP':'threaded'}),('profile-cpu','cpu',['--update-after-ms','1000'],{'QSG_RENDER_TIMING':'1','QT_LOGGING_RULES':'qt.scenegraph.time.*=true;qt.rhi.general=true'}),('profile-gpu','gpu',['--update-after-ms','1000'],{'QSG_RENDER_TIMING':'1','QT_LOGGING_RULES':'qt.scenegraph.time.*=true;qt.rhi.general=true'})]
for name,path,extra,variables in CASES:
 image=OUT/f'{name}-client.png'
 if image.exists():raise FileExistsError(image)
 build='ubsan-build' if name.startswith('ubsan') else 'clang-build'
 with (OUT/f'{name}.log').open('w') as log:
  proc=subprocess.Popen([f'/tmp/phosphor-003/{build}/phosphor-003','--path',path,'--seconds','12','--capture',str(image),'--metrics',str(OUT/f'{name}.json'),*extra],env=dict(ENV,**variables),stdout=log,stderr=log)
  try:
   # At DPR 1 set exact matching geometry before the seven-second capture.
   # Fractional scale keeps the same 1280x900 logical size; avoid forcing
   # compositor geometry in physical pixels on that scale.
   time.sleep(5)
   if not name.startswith('dpr'):
    targetWidth,targetHeight=('3848','2202') if name=='ubsan-gpu' else ('1288','942')
    subprocess.run(['python3',str(ROOT/'tools/kwin.py'),'--width',targetWidth,'--height',targetHeight],check=True,stdout=subprocess.DEVNULL)
   deadline=time.monotonic()+40
   while not image.exists():
    if proc.poll() is not None or time.monotonic()>deadline:raise RuntimeError(f'capture failed: {name}')
    time.sleep(.1)
   (OUT/f'{name}-geometry.json').write_bytes(subprocess.check_output(['python3',str(ROOT/'tools/kwin.py')]))
   subprocess.run(['spectacle','-a','-b','-n','-o',str(OUT/f'{name}-native.png')],check=True,env=ENV,timeout=10)
   assert proc.wait(timeout=40)==0
  finally:
   if proc.poll() is None:proc.terminate();proc.wait(timeout=10)
 print(name,'complete',flush=True)
