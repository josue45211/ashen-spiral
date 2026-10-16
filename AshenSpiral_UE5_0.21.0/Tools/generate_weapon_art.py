"""Original weapon / practice-target meshes. Standard library only; units are cm.
Local +X aims forward, +Z points up. All grips are centered at the origin.
"""
from pathlib import Path
import math
import json
ROOT = Path(__file__).resolve().parents[1] / 'SourceArt'

def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def add(a,b): return tuple(x+y for x,y in zip(a,b))
def mul(a,s): return tuple(x*s for x in a)
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def unit(a):
    length=math.sqrt(sum(x*x for x in a))
    return mul(a,1/length)

class Mesh:
    def __init__(self): self.faces=[]; self.group='body'
    def tri(self,a,b,c):
        n=cross(sub(b,a),sub(c,a))
        if sum(x*x for x in n)<1e-12: return
        self.faces.append((self.group,a,b,c,unit(n)))
    def quad(self,a,b,c,d): self.tri(a,b,c);self.tri(a,c,d)
    def link(self,a,b,r,sides=10,end_radius=None):
        axis=unit(sub(b,a));u=unit(cross(axis,(1,0,0) if abs(axis[0])<.8 else (0,1,0)));v=cross(axis,u)
        ends=[]
        for center,rad in [(a,r),(b,r if end_radius is None else end_radius)]:
            ends.append([add(center,mul(add(mul(u,math.cos(i*2*math.pi/sides)),mul(v,math.sin(i*2*math.pi/sides))),rad)) for i in range(sides)])
        for i in range(sides):
            j=(i+1)%sides
            self.quad(ends[0][i],ends[0][j],ends[1][j],ends[1][i]);self.tri(a,ends[0][j],ends[0][i]);self.tri(b,ends[1][i],ends[1][j])
    def box(self,center,size):
        p=[tuple(center[k]+size[k]*s[k]/2 for k in range(3)) for s in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
        for ids in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:self.quad(*(p[i] for i in ids))
    def blade(self,sections):
        # A tapered diamond cross section gives a ridge, cutting edges and point.
        rings=[]
        for x,z,width,thick in sections:rings.append([(x+thick,0,z),(x,width,z),(x-thick,0,z),(x,-width,z)])
        self.quad(*reversed(rings[0]))
        for a,b in zip(rings,rings[1:]):
            for i in range(4):self.quad(a[i],a[(i+1)%4],b[(i+1)%4],b[i])
        self.quad(*rings[-1])
    def grip(self):
        self.group='grip';self.link((0,0,-13),(0,0,13),2.6,12)
        for z in range(-11,12,3):self.link((0,0,z),(0,0,z+.8),3,12)
    def save(self,name):
        lines=['# Ashen Spiral original mesh; local +X forward, +Z up'];positions=[];last=''
        for group,a,b,c,n in self.faces:
            if group!=last:lines.append('g '+group);last=group
            base=len(positions)+1
            for p in (a,b,c):
                positions.append(p);lines.append('v %.6f %.6f %.6f'%p)
            axes=sorted(range(3),key=lambda k:abs(n[k]))[:2]
            for p in (a,b,c):lines.append('vt %.6f %.6f'%(p[axes[0]]/100,p[axes[1]]/100))
            for _ in range(3):lines.append('vn %.6f %.6f %.6f'%n)
            lines.append('f '+' '.join('%d/%d/%d'%(i,i,i) for i in range(base,base+3)))
        (ROOT/'Meshes'/f'{name}.obj').write_text('\n'.join(lines)+'\n',encoding='ascii')

def generate():
    for weapon in range(9):
        m=Mesh();m.grip()
        if weapon==0:
            m.group='guard';m.link((0,-18,16),(0,18,16),2.4);m.group='pommel';m.link((0,0,-19),(0,0,-13),4)
            m.group='blade';m.blade([(0,19,5.2,1.2),(0,78,4.5,1),(0,102,.08,.08)])
        elif weapon==1:
            m.group='shaft';m.link((0,0,13),(0,0,97),2.8);m.group='hammer_head'
            m.box((0,0,104),(26,52,24));m.box((0,-29,104),(29,7,27));m.box((0,29,104),(29,7,27))
            m.group='pommel';m.link((0,0,-19),(0,0,-13),3.5)
        elif weapon==2:
            m.group='bow_limbs'
            for sign in [-1,1]:
                pts=[(0,0,sign*12),(-2,0,sign*28),(-8,0,sign*46),(-19,0,sign*64),(-24,0,sign*73)]
                for i,(a,b) in enumerate(zip(pts,pts[1:])):m.link(a,b,3.2-i*.55,10,2.8-i*.5)
            m.group='sight';m.box((4,0,15),(8,2,2))
            # Strings and arrow are separate components and follow the drawing hand.
        elif weapon==3:
            m.group='shaft';m.link((0,0,-46),(0,0,92),2.3)
            m.group='glaive_blade';m.blade([(0,90,5,1.1),(6,108,8,1.2),(15,134,.08,.08)])
            m.group='collar';m.link((0,0,85),(0,0,91),4)
        elif weapon==4:
            m.group='shaft';m.link((0,0,-44),(0,0,102),2.7)
            m.group='crown';m.blade([(0,100,5,3),(0,114,10,7),(0,137,.08,.08)])
            for sign in [-1,1]:m.link((0,sign*3,99),(0,sign*14,117),2)
        elif weapon==5:
            m.group='shaft';m.link((0,0,-45),(0,0,106),2.5)
            m.group='scythe_blade';m.blade([(0,103,5,1.2),(17,116,7,1.5),(49,116,5,1),(75,101,.08,.08)])
        elif weapon==6:
            m.group='receiver';m.box((22,0,17),(52,19,24))
            m.group='stock';m.link((-9,0,17),(-33,0,7),7,6);m.box((-36,0,7),(9,17,25))
            m.group='twin_barrels'
            for y in [-6,6]:
                m.link((40,y,20),(108,y,20),5.5,12);m.link((107,y,20),(114,y,20),6.5,12)
            m.group='pump';m.box((64,0,13),(23,22,13))
            for x in [55,61,67,73]:m.box((x,0,13),(2,23,14))
            m.group='sight';m.box((103,0,29),(6,3,7))
        elif weapon==7:
            m.group='receiver';m.box((19,0,18),(45,28,28))
            m.group='rotor';m.link((35,0,19),(47,0,19),18,16)
            m.group='barrel_cluster'
            for j in range(6):
                a=j*math.tau/6;y=math.cos(a)*11;z=19+math.sin(a)*11
                m.link((43,y,z),(112,y,z),3.5,10)
            for x in [62,104]:m.link((x,0,19),(x+6,0,19),17,16)
            m.group='drum';m.link((13,-13,8),(13,-33,8),19,16)
            m.group='stock';m.box((-18,0,18),(27,21,20))
            m.group='handle';m.link((5,0,36),(31,0,36),3);m.link((5,0,28),(5,0,36),3)
        else:
            m.group='launch_tube';m.link((-25,0,25),(103,0,25),14,16)
            m.group='muzzle';m.link((97,0,25),(113,0,25),19,16,16)
            m.group='rear_baffle';m.link((-30,0,25),(-20,0,25),18,12)
            m.group='sight';m.box((44,0,44),(32,7,9));m.link((27,0,46),(62,0,46),5,8)
            m.group='handguard';m.box((31,0,5),(41,18,13))
            for j in [-1,1]:m.box((76,j*16,25),(34,5,18))
        m.save(f'SM_Weapon{weapon}')
    m=Mesh();m.group='route_arrow'
    outline=[(55,0),(-5,32),(-5,12),(-45,12),(-45,-12),(-5,-12),(-5,-32)]
    for i,(x,y) in enumerate(outline):
        nx,ny=outline[(i+1)%len(outline)]
        m.tri((0,0,7),(x,y,7),(nx,ny,7));m.tri((0,0,-7),(nx,ny,-7),(x,y,-7))
        m.quad((x,y,-7),(nx,ny,-7),(nx,ny,7),(x,y,7))
    m.save('SM_RouteArrow')
    m=Mesh();m.group='shaft';m.link((-33,0,0),(82,0,0),.65,8)
    m.group='arrowhead';m.link((82,0,0),(97,0,0),3,4,.05)
    m.group='fletching'
    for axis in range(2):
        size=(17,6,.5) if axis==0 else (17,.5,6);m.box((-22,0,0),size)
    m.save('SM_Arrow')
    m=Mesh();m.group='stand';m.box((0,0,7),(100,90,14));m.link((0,0,14),(0,0,160),7)
    m.group='body';m.link((0,0,75),(0,0,146),31,12);m.link((0,-61,122),(0,61,122),9)
    m.group='head';m.link((0,0,151),(0,0,187),18,12)
    m.group='target';m.link((30,0,114),(34,0,114),21,20)
    m.save('SM_TrainingDummy')
    manifest={}
    for file in (ROOT/'Meshes').glob('*.obj'):
        verts=[list(map(float,line.split()[1:])) for line in file.read_text().splitlines() if line.startswith('v ')]
        low=[min(v[i] for v in verts) for i in range(3)];high=[max(v[i] for v in verts) for i in range(3)]
        manifest[file.stem]={'min':low,'max':high,'extent':[high[i]-low[i] for i in range(3)]}
    (ROOT/'mesh_bounds.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Generated nine forward-facing weapons, thick world-route arrow, bow arrow and training dummy.')

if __name__=='__main__':generate()
