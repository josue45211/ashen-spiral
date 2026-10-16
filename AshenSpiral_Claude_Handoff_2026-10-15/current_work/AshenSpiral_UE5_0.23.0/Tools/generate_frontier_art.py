"""Original faceted cave formations and interactable meshes. No external assets."""
from generate_weapon_art import Mesh, ROOT
import math, random, json

def crystal(m, at, radius, height, lean=(0,0)):
    x,y,z=at;n=6
    lower=[(x+radius*math.cos(i*math.tau/n),y+radius*math.sin(i*math.tau/n),z) for i in range(n)]
    upper=[(a+lean[0]*.7,b+lean[1]*.7,z+height*.7) for a,b,_ in lower]
    tip=(x+lean[0],y+lean[1],z+height)
    for i in range(n):
        j=(i+1)%n;m.quad(lower[i],lower[j],upper[j],upper[i]);m.tri(upper[i],upper[j],tip);m.tri((x,y,z),lower[j],lower[i])

def generate():
    rng=random.Random(1500);m=Mesh();rings=[]
    for layer,z in enumerate([-100,-60,55,100]):
        rings.append([(math.cos(i*math.tau/9)*rng.uniform(83,110)*(0.7 if layer in (0,3) else 1),math.sin(i*math.tau/9)*rng.uniform(80,106)*(0.7 if layer in (0,3) else 1),z+rng.uniform(-7,7)) for i in range(9)])
    for a,b in zip(rings,rings[1:]):
        for i in range(9):m.quad(a[i],a[(i+1)%9],b[(i+1)%9],b[i])
    for i in range(9):m.tri((0,0,-110),rings[0][(i+1)%9],rings[0][i]);m.tri((0,0,110),rings[-1][i],rings[-1][(i+1)%9])
    m.save('SM_CaveRock')
    m=Mesh();crystal(m,(0,0,0),28,185,(12,-10))
    for i in range(7):
        a=i*math.tau/7;crystal(m,(math.cos(a)*42,math.sin(a)*42,0),rng.uniform(12,23),rng.uniform(70,140),(math.cos(a)*25,math.sin(a)*25))
    m.save('SM_CrystalCluster')
    m=Mesh();crystal(m,(0,0,0),28,113,(4,-3));crystal(m,(-15,9,0),15,61,(-20,10));m.save('SM_LootCrystal')
    m=Mesh();m.link((0,0,5),(0,0,92),32,12);m.link((0,0,18),(0,0,25),34,12);m.link((0,0,72),(0,0,79),34,12)
    for a in range(0,360,90):
        x,y=math.cos(math.radians(a))*33,math.sin(math.radians(a))*33
        m.box((x,y,49),(9,9,30))
    m.save('SM_EmberCask')
    m=Mesh();m.box((-20,0,0),(18,75,75));m.link((-13,0,0),(36,0,0),22,10,16);m.link((28,0,0),(38,0,0),19,10);m.save('SM_WallNozzle')
    bounds={}
    for p in sorted((ROOT/'Meshes').glob('*.obj')):
        points=[list(map(float,l.split()[1:])) for l in p.read_text().splitlines() if l.startswith('v ')]
        lo=[min(v[i] for v in points) for i in range(3)];hi=[max(v[i] for v in points) for i in range(3)]
        bounds[p.stem]={'min':lo,'max':hi,'extent':[hi[i]-lo[i] for i in range(3)]}
    (ROOT/'mesh_bounds.json').write_text(json.dumps(bounds,indent=2)+'\n')
    print('Frontier meshes generated:',len(bounds))
if __name__=='__main__':generate()
