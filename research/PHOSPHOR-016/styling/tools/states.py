import json
from pathlib import Path
E=Path(__file__).resolve().parents[1]/'evidence'
a=[dict(ms=300,text="PS1='phosphor$ '; clear; seq 1 240"),dict(ms=500,key='Return')]
def shot(ms,name):a.append(dict(ms=ms,capture=str(E/f'{name}.png'),metrics=str(E/f'{name}.json')))
shot(1400,'rest')
a.append(dict(ms=1800,mouse='hover',thumb=True));shot(2200,'hover')
a.append(dict(ms=2600,mouse='press',thumb=True));shot(3000,'pressed')
a.append(dict(ms=3400,mouse='move',x=1271,y=450));shot(3800,'dragging')
a.append(dict(ms=4200,mouse='release',x=1271,y=450));a.append(dict(ms=4500,mouse='hover',x=100,y=100));shot(4900,'released')
a.append(dict(ms=5400,close=True))
(E/'states-actions.json').write_text(json.dumps(a,indent=2)+'\n')
