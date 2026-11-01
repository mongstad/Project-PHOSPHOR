"""Check retained final native evidence; Pillow is validation-only."""
import json
from pathlib import Path
from PIL import Image, ImageChops
E=Path(__file__).resolve().parents[1]/'evidence'
def state(name): return json.loads((E/f'{name}.json').read_text())
assert state('short')['history_lines']==0
assert state('bottom')['scroll_offset']==0
assert 0<state('drag-middle')['scroll_offset']<state('drag-middle')['history_lines']
assert state('track')['scroll_offset']==state('drag-middle')['scroll_offset']+25
assert state('wheel')['scroll_offset']==state('track')['scroll_offset']+3
assert state('keyboard')['scroll_offset']==state('wheel')['scroll_offset']+25
assert (E/'anchor-before.txt').read_bytes()==(E/'anchor-after.txt').read_bytes()
assert state('anchor-after')['scroll_offset']>state('anchor-before')['scroll_offset']>0
assert state('drag-live')['scroll_offset']==0
assert state('evicted')['history_lines']==state('evicted')['scroll_offset']==1000
assert state('resize')['scroll_offset']==1000
assert (state('resize')['columns'],state('resize')['rows'])==(59,16)
assert state('nano')['scroll_offset']==state('nano-wheel')['scroll_offset']==0
assert (E/'nano.txt').read_bytes()==(E/'nano-wheel.txt').read_bytes()
assert state('restored')['scroll_offset']==0
assert 'FOCUS_OK' in (E/'focus.txt').read_text()
a=Image.open(E/'baseline-short.png').convert('RGB')
b=Image.open(E/'current-short.png').convert('RGB')
assert ImageChops.difference(a,b).getbbox()==(1266,20,1276,880)
print('PASS native navigation, anchoring, eviction, resize, alternate screen, shell input, pixel preservation')
for name in ['baseline','current']:
 for phase in ['idle','output']:
  a=state(f'{name}-{phase}-start');b=state(f'{name}-{phase}-end')
  frames=b['render_calls']-a['render_calls'];work=b['render_work_ms']-a['render_work_ms']
  print(name,phase,'duration_ms',b['elapsed_ms']-a['elapsed_ms'],'renders',frames,'render_ms',round(work,4),'ms_per_render',round(work/frames,4) if frames else 0)
  if phase=='idle':assert frames==0
