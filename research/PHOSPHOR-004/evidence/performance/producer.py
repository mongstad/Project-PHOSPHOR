import sys,time
end=time.monotonic()+10
n=0
while time.monotonic()<end:
 sys.stdout.write(''.join((f'[{n+i:06d}] glyph.material /tmp/sample.cpp x=0.125 y=3.750 OK '*3)[:140]+'\n' for i in range(20)))
 sys.stdout.flush();n+=20;time.sleep(.016)
