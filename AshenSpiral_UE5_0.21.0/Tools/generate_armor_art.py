"""Original modular gunmetal armor and sinister variants, modeled in centimeters.
Standard library only. Run last when regenerating SourceArt. No downloaded assets.
"""
from generate_weapon_art import Mesh, ROOT
import math
import json

def loft(m, rings, sides=20):
    """Closed elliptical shell; rings are z, x-radius, y-radius, x-center."""
    rows=[[(cx+rx*math.cos(i*2*math.pi/sides),ry*math.sin(i*2*math.pi/sides),z) for i in range(sides)] for z,rx,ry,cx in rings]
    for a,b in zip(rows,rows[1:]):
        for i in range(sides): j=(i+1)%sides;m.quad(a[i],a[j],b[j],b[i])
    for row,ring,flip in [(rows[0],rings[0],True),(rows[-1],rings[-1],False)]:
        center=(ring[3],0,ring[0])
        for i in range(sides):
            a,b=row[i],row[(i+1)%sides]
            m.tri(center,b,a) if flip else m.tri(center,a,b)

def panel(m, x, yz, depth=2):
    # Convex raised front-facing plate with its own seam and beveled surround.
    area=sum(yz[i][0]*yz[(i+1)%len(yz)][1]-yz[(i+1)%len(yz)][0]*yz[i][1] for i in range(len(yz)))
    if (area>0)!=(depth>0): yz=list(reversed(yz))
    cy=sum(y for y,z in yz)/len(yz);cz=sum(z for y,z in yz)/len(yz)
    inner=[(cy+(y-cy)*.89,cz+(z-cz)*.89) for y,z in yz]
    for i in range(len(yz)):
        j=(i+1)%len(yz);a=(x,*yz[i]);b=(x,*yz[j]);c=(x+depth,*inner[j]);d=(x+depth,*inner[i])
        m.quad(a,b,c,d);m.tri((x+depth,cy,cz),d,c);m.tri((x,cy,cz),b,a)

def torso():
    m=Mesh();loft(m,[(-32,13,20,-2),(-20,15,21,0),(0,18,28,-1),(21,18,30,-2),(30,13,23,-1)],24)
    loft(m,[(29,12,14,0),(37,11,13,0),(40,12,15,0)],20)
    for side in [-1,1]:
        for y,z,wide in [(17,20,11),(17,7,10),(14,-8,9)]:
            panel(m,18,[(side*(y-wide),z+8),(side*(y+wide),z+5),(side*(y+wide-2),z-5),(side*(y-2),z-10)],3)
        for z in [-20,-10,0,10]:panel(m,-16,[(side*7,z+5),(side*20,z+5),(side*18,z-3),(side*8,z-5)],-2)
    for z,w in [(-24,10),(-14,12),(-4,11),(6,8)]:panel(m,19,[(-w,z+5),(w,z+5),(w*.7,z-4),(0,z-7),(-w*.7,z-4)],2)
    panel(m,20,[(-8,29),(8,29),(10,18),(0,5),(-10,18)],3)
    return m

def helmet(sinister=False):
    m=Mesh();loft(m,[(-17,9,10,2),(-12,13,12,1),(0,16,15,0),(10,14,14,-1),(17,10,11,-2),(20,2,3,-2)],28)
    for side in [-1,1]:
        panel(m,14,[(side*2,14),(side*12,10),(side*15,0),(side*6,-12),(side*2,-16)],2)
        m.link((0,side*14,0),(0,side*18,0),7,16)
        m.link((-2,side*18,0),(-2,side*19,0),4,16)
        if sinister:m.link((-4,side*10,12),(-15,side*23,36),4,10,.3)
    panel(m,17,[(-4,13),(4,13),(4,-5),(0,-15),(-4,-5)],2)
    return m

def limb(forearm=False,leg=False,greave=False):
    m=Mesh();length=38 if leg else 42
    r=11 if leg else 9;loft(m,[(-length/2,r*.72,r*.82,0),(-length*.28,r*.9,r*.95,0),(length*.30,r,r,0),(length/2,r*.88,r*.9,0)],16)
    for z in [-length*.25,0,length*.25]:
        panel(m,r+.3,[(-r*.8,z+7),(r*.8,z+7),(r*.72,z-3),(0,z-9),(-r*.72,z-3)],2.4)
    if forearm:panel(m,r,[(-7,19),(7,19),(9,4),(4,-18),(-4,-18),(-9,4)],3)
    if greave:panel(m,r,[(-8,17),(8,17),(9,6),(4,-19),(-4,-19),(-9,6)],3.5)
    return m

def cape(upper=True,torn=False):
    m=Mesh();n=20;steps=12;length=55 if upper else 72
    rows=[]
    for j in range(steps+1):
        t=j/steps;width=(27+9*t) if upper else (36+10*t)
        row=[]
        for i in range(n+1):
            u=i/n*2-1;z=-t*length
            if torn and j==steps:z+=8*(i%3)
            x=-3*t-2.7*math.cos(u*math.pi*5)*(0.3+t*.7)
            row.append((x,u*width,z))
        rows.append(row)
    # Thin closed cloth shell, with thickness rather than coincident duplicate faces.
    for a,b in zip(rows,rows[1:]):
        for i in range(n):
            q=[a[i],b[i],b[i+1],a[i+1]];m.quad(*q);m.quad(*[(x-.55,y,z) for x,y,z in reversed(q)])
    edge=rows[0]+[r[-1] for r in rows[1:]]+list(reversed(rows[-1][:-1]))+[r[0] for r in reversed(rows[1:-1])]
    for a,b in zip(edge,edge[1:]+edge[:1]):m.quad(a,b,(b[0]-.55,b[1],b[2]),(a[0]-.55,a[1],a[2]))
    return m

def generate():
    torso().save('SM_Torso');helmet().save('SM_Helmet');helmet(True).save('SM_EnemyHelm')
    m=Mesh();loft(m,[(-13,12,18,0),(-6,16,25,0),(10,14,25,0),(15,13,21,0)],20)
    for side in [-1,1]:panel(m,13,[(side*4,8),(side*23,6),(side*19,-12),(side*8,-19)],3)
    m.save('SM_Pelvis')
    limb().save('SM_Arm');limb(forearm=True).save('SM_Forearm');limb(leg=True).save('SM_Thigh');limb(leg=True,greave=True).save('SM_Greave')
    m=Mesh();loft(m,[(-9,15,10,4),(-5,17,10,5),(5,14,9,3),(10,9,8,-3)],16)
    for x,z in [(8,6),(17,0)]:m.link((x,-8,z),(x,8,z),1.2,8)
    m.save('SM_Boot')
    m=Mesh();loft(m,[(-14,10,8,0),(-4,17,12,0),(7,18,13,-1),(14,13,10,-2)],20)
    for z in [-6,3,11]:panel(m,13,[(-10,z+4),(10,z+4),(11,z-3),(0,z-7),(-11,z-3)],2)
    m.save('SM_Pauldron')
    m=Mesh();loft(m,[(-8,5,6,0),(-2,6,8,0),(5,5,8,0),(9,4,6,0)],12)
    for y in [-6,-2,2,6]:m.link((3,y,0),(5,y,-9),1.8,8);m.link((5,y,-9),(8,y,-8),1.7,8)
    m.link((0,-7,3),(5,-11,-2),2.2,8);m.link((5,-11,-2),(8,-9,-6),2,8);m.save('SM_Hand')
    m=Mesh()
    for side in [-1,1]:panel(m,17.2,[(side*2,1.5),(side*13,6),(side*12,3),(side*3,-.3)],.7)
    m.save('SM_Visor')
    m=Mesh();panel(m,26,[(-5,24),(5,24),(0,11)],.8)
    for side in [-1,1]:panel(m,19,[(side*18,8),(side*24,13),(side*24,11),(side*18,6)],.65)
    m.save('SM_ArmorCore')
    cape().save('SM_Cape');cape(False).save('SM_CapeLower');cape(False,True).save('SM_EnemyCape')
    m=Mesh()
    for side in [-1,1]:
        for x in [-12,18]:
            m.link((x,side*16,68),(x-23,side*43,36),9,12,6)
            m.link((x-23,side*43,36),(x+14,side*55,5),5,10,2)
            m.link((x+14,side*55,5),(x+28,side*55,2),2,8,.1)
    loft(m,[(45,27,19,-8),(67,38,27,-3),(91,24,18,0)],20)
    for x in [-20,0,18]:m.link((x,-20,78),(x,20,78),5,12)
    m.link((24,0,72),(45,0,72),13,16,8)
    for side in [-1,1]:m.link((40,side*8,69),(54,side*13,56),4,8,.3)
    m.save('SM_RiftLeaper')
    m=Mesh();loft(m,[(24,21,27,-5),(66,28,38,-9),(110,26,36,-9),(131,14,18,0)],24)
    for side in [-1,1]:
        m.link((0,side*18,55),(13,side*32,24),10,12,8);m.link((13,side*32,24),(4,side*39,5),7,10,3)
        m.link((0,side*28,114),(17,side*55,67),10,12,5)
        m.link((17,side*55,67),(29,side*63,42),5,10,1)
        m.link((-15,side*24,116),(-27,side*39,161),7,10,.5)
    loft(m,[(126,12,13,10),(144,17,16,9),(162,8,10,4)],20);m.save('SM_Broodcaller')
    m=Mesh();panel(m,0,[(-24,44),(24,44),(27,15),(19,-32),(0,-49),(-19,-32),(-27,15)],5);m.link((6,0,-38),(6,0,34),2,8);m.save('SM_EnemyShield')
    manifest={}
    for file in sorted((ROOT/'Meshes').glob('*.obj')):
        v=[list(map(float,s.split()[1:])) for s in file.read_text().splitlines() if s.startswith('v ')]
        lo=[min(p[i] for p in v) for i in range(3)];hi=[max(p[i] for p in v) for i in range(3)]
        manifest[file.stem]={'min':lo,'max':hi,'extent':[hi[i]-lo[i] for i in range(3)]}
    (ROOT/'mesh_bounds.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Generated articulated armor kit and sinister variants:',len(manifest),'total source meshes')

if __name__=='__main__':generate()
