"""Original, deterministic synthetic reload cues. No external dependencies."""
from pathlib import Path
import math,random,struct,wave
root=Path(__file__).resolve().parents[1]/'SourceArt/Audio'
for name,complete in [('S_ReloadStart',False),('S_ReloadReady',True)]:
    rng=random.Random(1900+complete);rate=44100;duration=.38 if complete else .26;data=[]
    for i in range(int(rate*duration)):
        t=i/rate;v=0.
        for start,freq in ([(0,720),(.105,1080)] if complete else [(0,260),(.095,190)]):
            d=t-start
            if d>=0:
                v+=.30*math.sin(2*math.pi*freq*d)*math.exp(-d*32)
                v+=.15*rng.uniform(-1,1)*math.exp(-d*110)
        data.append(struct.pack('<h',int(max(-.9,min(.9,v))*32767)))
    with wave.open(str(root/(name+'.wav')),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','not compressed'));w.writeframes(b''.join(data))
print('Generated start click and ascending ready cue.')
