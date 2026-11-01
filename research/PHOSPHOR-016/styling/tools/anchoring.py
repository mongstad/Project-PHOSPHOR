"""Generate an extra bounded sequence with output slower than history eviction."""
import json
from pathlib import Path
E=Path(__file__).resolve().parents[1]/'evidence'
a=[dict(ms=300,text="python3 -c 'import time; [(print(i, flush=True), time.sleep(.01)) for i in range(550)]'"),dict(ms=400,key='Return'),dict(ms=1900,key='PageUp',shift=True)]
for ms,name in [(2200,'anchor-before'),(3300,'anchor-after')]:
 a.append(dict(ms=ms,capture=str(E/f'{name}.png'),dump=str(E/f'{name}.txt'),metrics=str(E/f'{name}.json')))
# Grab midway in the small thumb, then drag below track to return live.
a += [dict(ms=3900,mouse='press',thumb=True),dict(ms=4100,mouse='move',x=1271,y=1000),dict(ms=4300,mouse='release',x=1271,y=1000),dict(ms=5000,key='PageDown',shift=True)]
a.append(dict(ms=4500,metrics=str(E/'drag-live.json')))
a.append(dict(ms=6200,text='echo FOCUS_OK'));a.append(dict(ms=6400,key='Return'))
a.append(dict(ms=7000,capture=str(E/'focus.png'),dump=str(E/'focus.txt'),metrics=str(E/'focus.json')))
a.append(dict(ms=7500,close=True))
(E/'anchoring-actions.json').write_text(json.dumps(a,indent=2)+'\n')
