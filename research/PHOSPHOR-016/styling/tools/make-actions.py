import json
from pathlib import Path
E=Path(__file__).resolve().parents[1]/'evidence'
a=[]
def cmd(ms,text): a.extend([dict(ms=ms,text=text),dict(ms=ms+30,key='Return')])
def shot(ms,name): a.append(dict(ms=ms,capture=str(E/f'{name}.png'),dump=str(E/f'{name}.txt'),metrics=str(E/f'{name}.json')))
def mouse(ms,kind,x,y):a.append(dict(ms=ms,mouse=kind,x=x,y=y))
cmd(300,"PS1='phosphor$ '; clear; printf 'PHOSPHOR scrollbar check\\n\\033[31mANSI red \\033[92mgreen\\033[0m\\n'")
shot(1100,'short')
cmd(1400,'seq 1 240')
shot(2200,'bottom')
mouse(2500,'press',1271,835);mouse(2600,'move',1271,440);mouse(2700,'release',1271,440)
shot(3000,'drag-middle')
mouse(3300,'press',1271,100);mouse(3400,'release',1271,100)
shot(3700,'track')
a.append(dict(ms=3900,wheel=120));shot(4200,'wheel')
a.append(dict(ms=4400,key='PageUp',shift=True));shot(4700,'keyboard')
cmd(5000,"python3 -c 'import time; [(print(i, flush=True), time.sleep(.002)) for i in range(1600)]'")
a.append(dict(ms=5500,key='PageUp',shift=True));shot(5900,'incoming-before');shot(7600,'incoming-after');shot(9000,'evicted')
a.append(dict(ms=9300,width=800,height=600));shot(10000,'resize')
cmd(10200,'nano --ignorercfiles /tmp/phosphor-scrollbar-sample.txt');shot(11200,'nano');a.append(dict(ms=11500,wheel=120));shot(11800,'nano-wheel');a.append(dict(ms=12100,key='X',ctrl=True));shot(12600,'restored');a.append(dict(ms=13000,close=True))
(E/'actions.json').write_text(json.dumps(a,indent=2)+'\n')
