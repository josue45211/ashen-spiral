"""Original environment geometry built from natural/architectural reference studies.
No photos or third-party meshes are distributed. Units are centimetres.
"""
from generate_weapon_art import Mesh, ROOT
import math, random, json

def vault(name, rough):
    m=Mesh();rng=random.Random(1600);grid=[]
    for j in range(7):
        x=-300+j*100;row=[]
        for i in range(17):
            a=math.pi*i/16;noise=rng.uniform(-24,24) if rough else 0
            row.append((x,math.cos(a)*(500+noise),420+math.sin(a)*(620+noise)))
        grid.append(row)
    for a,b in zip(grid,grid[1:]):
        for i in range(16):
            # Inward and outward skin, with a closed rim; no invisible lower wall.
            m.quad(a[i],b[i],b[i+1],a[i+1])
            aa=[(x,y,z+45) for x,y,z in (a[i],a[i+1],b[i+1],b[i])];m.quad(*aa)
    boundary=grid[0]+[r[-1] for r in grid[1:]]+list(reversed(grid[-1][:-1]))+[r[0] for r in reversed(grid[1:-1])]
    for i,a in enumerate(boundary):
        b=boundary[(i+1)%len(boundary)];m.quad(a,b,(b[0],b[1],b[2]+45),(a[0],a[1],a[2]+45))
    m.save(name)

def formation(name,down=False):
    m=Mesh();rng=random.Random(1691);rings=[]
    sign=-1 if down else 1
    for j,(z,r) in enumerate([(0,65),(30,62),(95,44),(165,23),(245,2)]):
        rings.append([(math.cos(i*math.tau/11)*r+rng.uniform(-2,2)+j*3,math.sin(i*math.tau/11)*r+rng.uniform(-2,2),sign*z) for i in range(11)])
    for a,b in zip(rings,rings[1:]):
        for i in range(11):m.quad(a[i],a[(i+1)%11],b[(i+1)%11],b[i])
    for i in range(11):
        m.tri((0,0,0),rings[0][(i+1)%11],rings[0][i]);m.tri((12,0,sign*245),rings[-1][i],rings[-1][(i+1)%11])
    if down:
        # Reflection reverses winding; keep the visible faces pointing outward.
        m.faces=[(g,a,c,b,tuple(-v for v in n)) for g,a,b,c,n in m.faces]
    m.save(name)

def generate():
    vault('SM_CaveVault',True);vault('SM_BarrelVault',False)
    formation('SM_Stalagmite');formation('SM_Stalactite',True)
    m=Mesh();m.link((0,0,0),(0,0,210),65,6,63);m.link((0,0,210),(0,0,222),63,6,53);m.save('SM_BasaltColumn')
    m=Mesh();pts=[(-140,0,10),(-85,8,55),(-10,0,85),(70,-12,58),(150,0,12)]
    for i in range(4):m.link(pts[i],pts[i+1],24-i*3,9,20-i*3)
    m.save('SM_RootLog')
    m=Mesh();rng=random.Random(1617);grid=[]
    for x in range(13):grid.append([(-500+x*1000/12,-500+y*1000/12,rng.uniform(-3,3)) for y in range(13)])
    for x in range(12):
        for y in range(12):m.quad(grid[x][y],grid[x+1][y],grid[x+1][y+1],grid[x][y+1])
    m.save('SM_NaturalFloor')
    m=Mesh();m.link((0,0,0),(0,0,14),25,8);m.link((0,0,14),(0,0,84),13,8)
    for a in range(0,360,90):
        x,y=math.cos(math.radians(a))*24,math.sin(math.radians(a))*24;m.link((x,y,10),(x,y,94),3,5)
    m.link((0,0,88),(0,0,102),29,8,7);m.save('SM_Lantern')
    bounds={}
    for p in sorted((ROOT/'Meshes').glob('*.obj')):
        points=[list(map(float,l.split()[1:])) for l in p.read_text().splitlines() if l.startswith('v ')]
        lo=[min(v[i] for v in points) for i in range(3)];hi=[max(v[i] for v in points) for i in range(3)]
        bounds[p.stem]={'min':lo,'max':hi,'extent':[hi[i]-lo[i] for i in range(3)]}
    (ROOT/'mesh_bounds.json').write_text(json.dumps(bounds,indent=2)+'\n');print('Environment kit:',len(bounds),'meshes')
if __name__=='__main__':generate()
