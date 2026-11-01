import os,sys,time,json,tty,termios,fcntl,struct,select,signal,subprocess
from pathlib import Path
root=Path(sys.argv[1]);root.mkdir(exist_ok=True)
sys.path.insert(0,'/home/Nozba/Codex/Project-PHOSPHOR/Project-PHOSPHOR/prototypes/ghostty-molten-amber')
import fixtures
control=root/'control.json';state=root/'state.json'
label=root.name.upper().replace('BASELINE','BASELINE EFFECTS OFF').replace('PROTOTYPE','PROTOTYPE EFFECTS ON')
sys.stdout.write('\x1b]0;PHOSPHOR-001 | '+label+'\x07');sys.stdout.flush()
old=termios.tcgetattr(0);tty.setraw(0)
mode=None;last_query=None;row=0;inputs=0;matched=False;last_size=None;started=time.monotonic()
def size():return struct.unpack('HHHH',fcntl.ioctl(0,termios.TIOCGWINSZ,bytes(8)))
def write_state():
 state.write_text(json.dumps({'pid':os.getpid(),'mode':mode,'tty_size':size(),'rows_emitted':row,'input_bytes':inputs,'paste_marker_received':matched,'seconds':time.monotonic()-started}))
def redraw():
 sys.stdout.write('\x1b[?1049l\x1b[0m\x1b[2J\x1b[H');fixtures.static();sys.stdout.write('\r\nPHOSPHOR native test > ');sys.stdout.flush()
try:
 while True:
  try:params=json.loads(control.read_text())
  except (FileNotFoundError,json.JSONDecodeError):params={'mode':'idle'}
  new=params.get('mode','idle')
  if new=='exit':break
  if new!=mode:
   mode=new
   if mode in ('idle','input'):redraw()
   elif mode=='fullscreen':sys.stdout.write('\x1b[?1049h\x1b[?25l\x1b[2J\x1b[H')
   elif mode=='scroll':sys.stdout.write('\x1b[?25h\x1b[2J\x1b[H')
  cursor=params.get('cursor')
  if cursor is not None:sys.stdout.write(f'\x1b[{int(cursor)} q')
  if mode=='scroll':
   sys.stdout.write(''.join(f'{i:08d} | Molten Amber rapid scroll | αβγ ── 0123456789\r\n' for i in range(row,row+32)));row+=32
  elif mode=='fullscreen':
   h,w,_,_=size();sys.stdout.write(f'\x1b[H\x1b[38;2;255;158;46mPHOSPHOR fullscreen {w}x{h}; hidden cursor; resize\x1b[0m\r\n'+'┌'+'─'*max(0,min(w-3,100))+'┐\r\n')
  if params.get('query')!=last_query:
   last_query=params.get('query');sys.stdout.write('\x1b[6n')
  sys.stdout.flush()
  ready,_,_=select.select([0],[],[],1/60)
  if ready:
   data=os.read(0,4096);inputs+=len(data)
   if b'PHOSPHOR_PASTE_0123' in data:matched=True;sys.stdout.write('\r\nReceived test paste marker.\r\n');sys.stdout.flush()
   if b'\x03' in data:break
  if size()!=last_size or int((time.monotonic()-started)*5)%5==0:
   last_size=size();write_state()
finally:
 sys.stdout.write('\x1b[0m\x1b[?1049l\x1b[?25h\x1b[0 q');sys.stdout.flush();termios.tcsetattr(0,termios.TCSANOW,old);write_state()
