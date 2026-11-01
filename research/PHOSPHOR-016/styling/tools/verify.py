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
old=E.parents[1]/'evidence'
a=Image.open(old/'bottom.png').convert('RGB')
b=Image.open(E/'bottom.png').convert('RGB')
assert ImageChops.difference(a.crop((0,0,1256,900)),b.crop((0,0,1256,900))).getbbox() is None
images=[Image.open(E/f'{name}.png').convert('RGB') for name in ['rest','hover','pressed']]
for a,b in zip(images,images[1:]):
 bounds=ImageChops.difference(a,b).getbbox()
 assert bounds is not None and bounds[0]>=1264, bounds
assert state('pressed')['scroll_offset']==state('rest')['scroll_offset']==0
assert state('dragging')['scroll_offset']>0
assert state('released')['scroll_offset']==state('dragging')['scroll_offset']
print('PASS native navigation, anchoring, resize, eviction, nano, focus, text pixel preservation, hover/pressed states and release')
