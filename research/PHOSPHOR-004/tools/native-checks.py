"""Real Qt/Wayland PTY validation. No monitor modes or installed settings change."""
import json,os,subprocess,time
from pathlib import Path
R=Path(__file__).resolve().parents[1]; E=R/'evidence'; E.mkdir(exist_ok=True)
ENV=dict(os.environ,QT_QPA_PLATFORM='wayland',QT_SCALE_FACTOR='1',QSG_RENDER_LOOP='basic',QT_FORCE_STDERR_LOGGING='1',QSG_INFO='1',XDG_CACHE_HOME='/tmp/phosphor-004/cache',PS1='phosphor$ ')

def command(ms,text):return [{'ms':ms,'text':text},{'ms':ms+30,'key':'Return'}]
def shot(ms,name):return {'ms':ms,'capture':str(E/f'{name}-client.png'),'dump':str(E/f'{name}-screen.txt'),'metrics':str(E/f'{name}-snapshot.json')}
def run(name,actions,seconds=20,build='clang',clean=True):
 path=E/f'{name}-actions.json'; assert not path.exists(),path;path.write_text(json.dumps(actions,indent=2)+'\n')
 captures=[Path(a['capture']) for a in actions if 'capture' in a];done=set()
 with (E/f'{name}.log').open('w') as log:
  proc=subprocess.Popen([f'/tmp/phosphor-004/{build}-build/phosphor-terminal',*(['--clean-bash'] if clean else []),'--seconds',str(seconds),'--test-script',str(path),'--metrics',str(E/f'{name}-metrics.json')],env=ENV,stdout=log,stderr=log)
  try:
   while proc.poll() is None:
    for image in captures:
     if image.exists() and image not in done:
      stem=image.name.removesuffix('-client.png');geometry=subprocess.check_output(['python3',str(R/'tools/kwin.py')]);(E/f'{stem}-geometry.json').write_bytes(geometry)
      subprocess.run(['spectacle','-a','-b','-n','-o',str(E/f'{stem}-native.png')],env=ENV,check=True,timeout=10);done.add(image)
    time.sleep(.1)
   assert proc.returncode==0,(name,proc.returncode)
   assert all(p.exists() for p in captures)
  finally:
   if proc.poll() is None:proc.terminate();proc.wait(timeout=5)
 print(name,'complete',flush=True)

# Exactly reproduce the accepted fixture via actual shell cat -> PTY -> libvterm.
sample=['PHOSPHOR  /  MOLTEN AMBER','','astra@phosphor:~$ ./visual-sample --no-shell','/home/astra/research/PHOSPHOR-002/render.cpp','','ABCDEFGHIJKLMNOPQRSTUVWXYZ  0123456789','abcdefghijklmnopqrstuvwxyz  0O 1Il | ! : ; . ,','!@#$%^&*()_+-=[]{};:\'"<>,.?/\\|~`','┌──────────────────────────────┬───────────────┐','│ GLYPH MATERIAL               │ STATIC DEMO   │','├──────────────────────────────┼───────────────┤','│ Mask → bevel → light → bloom │ 12 / 14 / 16  │','└──────────────────────────────┴───────────────┘','Unicode: α β γ Δ Ω λ π ± × ÷ ≤ ≥ ← ↑ → ↓ é ø å','Solid glyphs: █ ▓ ▒ ░ ■ ◆   Thin glyphs: .,:;!|i','','No command execution. Synthetic text updates only.','']
for n in range(18,26):
 line=f'[{n:06d}] material.sample  x=0.125 y=-3.750  /src/glyph_{n%97:02d}.cpp  {{a+=1;}}  OK '
 sample.append((line*3)[:97])
fixture='\x1b[0m\x1b[2J\x1b[?25l'+''.join(f'\x1b[{row+1};1H'+line[:97] for row,line in enumerate(sample))
(E/'fixture.txt').write_text(fixture)
run('matched',command(500,f"cat '{E/'fixture.txt'}'; sleep 8")+[shot(2000,'matched'),{'ms':3500,'close':True}],seconds=8)

# Keyboard events follow the same handler as human input; no bytes go straight
# to the screen model. Commands are confined to inspection and /tmp test data.
a=command(500,"PS1='phosphor$ '; clear; pwd; ls -d prototypes research; echo REAL_SHELL; printf 'ANSI: \\033[31mred \\033[92mgreen \\033[38;2;80;160;255mtruecolor \\033[44;97mbackground\\033[0m\\n'; printf 'Unicode: α β ┌─┐ é ø å\\n'; uname -a; ls /nonexistent-phosphor-test")
a+=[shot(1800,'commands')]+command(2600,'sleep 30')+[{'ms':3300,'key':'C','ctrl':True}]
a+=command(3800,'echo INTERRUPT_OK')
a+=[{'ms':4300,'text':'echo helo'},{'ms':4400,'key':'Left'},{'ms':4500,'text':'l'},{'ms':4600,'key':'Return'},shot(5100,'editing')]
a+=command(5800,'seq 1 1200')+[{'ms':6500,'key':'PageUp','shift':True},shot(7000,'scrollback'),{'ms':7700,'key':'PageDown','shift':True}]
a+=[{'ms':8100,'width':800,'height':600}]+command(8500,'stty size')+[shot(9200,'resize'),{'ms':9900,'width':1280,'height':900}]
a+=[{'ms':10300,'key':'L','ctrl':True}]+command(10600,"echo RESIZE_AND_CLEAR_OK; printf '\\033[3;5HPOSITIONED\\n'")+[shot(11500,'positioned')]
a+=command(12400,'exit')
run('interactive',a,seconds=18)

Path('/tmp/phosphor-004/nano-sample.txt').write_text('PHOSPHOR nano exploratory check\nRead-only sample; exit without saving.\n')
a=command(500,'nano --ignorercfiles /tmp/phosphor-004/nano-sample.txt')+[shot(1800,'nano'),{'ms':2600,'key':'X','ctrl':True}]
a+=command(3300,'fastfetch --logo none')+[shot(5000,'fastfetch')]+command(6000,'exit')
run('applications',a,seconds=10)
run('configured-shell',command(800,'pwd; echo CONFIGURED_SHELL_OK')+[shot(2100,'configured-shell')]+command(3000,'exit'),seconds=7,clean=False)
run('ubsan-native',command(500,"printf '\\033[2J\\033[HUBSan real shell\\n'; seq 1 1000")+[{'ms':1600,'width':800,'height':600},shot(2400,'ubsan-native')]+command(3200,'exit'),seconds=8,build='ubsan')
# Ctrl+D on an empty readline prompt exits the real shell.
run('ctrl-d',[{'ms':600,'key':'D','ctrl':True}],seconds=6)
