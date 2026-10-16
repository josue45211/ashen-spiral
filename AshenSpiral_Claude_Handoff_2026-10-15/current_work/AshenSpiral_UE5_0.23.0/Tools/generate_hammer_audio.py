"""Original synthesized hammer cues. No external samples; mono 44.1kHz PCM."""
from pathlib import Path
import wave
import numpy as np
ROOT=Path(__file__).resolve().parents[1]/'SourceArt/Audio'
rng=np.random.default_rng(220022)
for name,duration in [('HammerSwing',.27),('HammerHit',.34),('HammerSmash',.78),('HammerLeap',.48)]:
    t=np.arange(int(44100*duration))/44100
    noise=rng.normal(0,1,len(t));low=np.convolve(noise,np.ones(18)/18,'same')
    if name=='HammerSwing': data=(noise*.10+low*.6)*np.sin(np.pi*t/duration)**2
    elif name=='HammerLeap':data=(low*.7+np.sin(2*np.pi*(70*t+190*t*t))*.2)*np.sin(np.pi*t/duration)**2
    else:
        heavy=name=='HammerSmash';decay=5 if heavy else 13
        data=(np.sin(2*np.pi*((45 if heavy else 110)*t+16*(1-np.exp(-t*30))))*.65+low*.8)*np.exp(-t*decay)
        data+=noise*.23*np.exp(-t*45)
        if heavy:data+=rng.normal(0,1,len(t))*.1*np.exp(-t*4)*(np.sin(t*77)**12)
    data*=np.minimum(1,t/.002);data/=max(1,float(abs(data).max())*1.04)
    with wave.open(str(ROOT/f'S_{name}.wav'),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(44100);f.writeframes((data*27000).astype('<i2').tobytes())
print('Generated four original hammer cues.')
