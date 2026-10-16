"""Original low-poly encounter meshes, centimeters, +X forward / +Z up.
Run after generate_source_art.py and generate_weapon_art.py. No external assets.
"""
from pathlib import Path
import json
import math
from generate_weapon_art import Mesh, ROOT

def legs(m, hips, feet, radius=7):
    m.group='legs'
    for a,b in zip(hips,feet):
        knee=((a[0]+b[0])/2-9,(a[1]+b[1])/2,(a[2]+b[2])/2)
        m.link(a,knee,radius,6);m.link(knee,b,radius*.8,6)
        m.box((b[0]+7,b[1],6),(26,15,12))

def sigil(m,p,r=8):
    x,y,z=p;m.group='rune';m.blade([(x,z-r,r*.5,3),(x,z,r,5),(x,z+r,.1,.1)])

def generate():
    m=Mesh();m.box((0,0,74),(62,47,38));m.link((-27,0,75),(-58,0,66),10,6,2)
    m.box((29,0,85),(28,31,26));m.link((43,-10,80),(58,-12,62),4,5,1);m.link((43,10,80),(58,12,62),4,5,1)
    for side in [-1,1]:
        for x in [-19,18]:
            m.link((x,side*20,79),(x-25,side*52,36),7,6);m.link((x-25,side*52,36),(x+25,side*62,8),5,6,2)
        m.link((-20,side*16,87),(-50,side*35,115),5,5,1)
    m.save('SM_RiftLeaper')
    m=Mesh();m.box((0,0,97),(45,61,52));m.box((1,0,146),(33,31,29));legs(m,[(0,-20,77),(0,20,77)],[(0,-25,9),(0,25,9)],10)
    for side in [-1,1]:
        m.box((0,side*47,116),(37,30,34));m.link((0,side*47,112),(18,side*49,68),11,7)
    m.group='shield';m.box((31,-42,97),(12,51,87));m.box((39,-42,97),(7,7,72));m.box((39,-42,108),(7,44,7))
    m.group='mace';m.link((25,48,59),(25,48,116),4,8);m.box((25,48,121),(29,30,25));m.save('SM_Bulwark')
    m=Mesh();m.link((0,0,11),(0,0,124),34,8,18);m.box((0,0,148),(30,29,30));
    for side in [-1,1]:
        m.link((0,side*17,117),(16,side*43,91),7,6);m.link((16,side*43,91),(34,side*48,106),6,6)
        m.link((0,side*11,159),(-10,side*27,187),4,6,1)
    m.group='staff';m.link((35,48,3),(35,48,195),3.5,8);m.link((35,48,183),(35,48,219),11,6,1)
    m.save('SM_EmberShaman')
    m=Mesh();m.box((0,0,112),(24,31,51));m.box((7,0,153),(35,22,24));legs(m,[(0,-10,90),(0,10,90)],[(-10,-20,7),(8,17,7)],5)
    for side in [-1,1]:
        m.link((0,side*18,127),(25,side*27,110),5,6);m.link((25,side*27,110),(47,side*10,124),5,6)
    m.group='rifle';m.box((55,3,126),(64,14,15));m.link((82,3,126),(107,3,126),4,8);m.box((52,3,142),(23,8,7));m.link((-10,0,152),(-40,0,108),9,6,2)
    m.save('SM_RiftSniper')
    m=Mesh();m.link((0,0,43),(0,0,125),40,10,31);m.box((23,0,145),(31,31,26));legs(m,[(0,-23,52),(0,23,52)],[(15,-35,7),(15,35,7)],8)
    for side in [-1,1]:
        m.link((0,side*32,113),(10,side*61,60),9,6)
        m.link((-20,side*20,116),(-43,side*30,166),8,6,3)
        m.box((-32,side*20,94),(23,28,40))
    m.save('SM_Broodcaller')
    m=Mesh();m.box((0,0,0),(7,13,38));m.box((0,0,0),(7,38,13));m.save('SM_HealthPickup')
    m=Mesh();outline=[(-18,17),(18,17),(17,-4),(0,-24),(-17,-4)]
    for i,(y,z) in enumerate(outline):
        yy,zz=outline[(i+1)%len(outline)]
        m.tri((4,0,0),(4,y,z),(4,yy,zz));m.tri((-4,0,0),(-4,yy,zz),(-4,y,z));m.quad((-4,y,z),(-4,yy,zz),(4,yy,zz),(4,y,z))
    m.save('SM_ShieldPickup')
    m=Mesh();m.link((0,0,-4),(0,0,4),48,24)
    for i in range(16):
        a=i*2*math.pi/16;b=a+.18;c=a+.36
        p=(math.cos(a)*46,math.sin(a)*46);q=(math.cos(b)*63,math.sin(b)*63);r=(math.cos(c)*46,math.sin(c)*46)
        m.tri((p[0],p[1],4),(q[0],q[1],4),(r[0],r[1],4));m.tri((r[0],r[1],-4),(q[0],q[1],-4),(p[0],p[1],-4))
        m.quad((p[0],p[1],-4),(q[0],q[1],-4),(q[0],q[1],4),(p[0],p[1],4));m.quad((q[0],q[1],-4),(r[0],r[1],-4),(r[0],r[1],4),(q[0],q[1],4))
    m.save('SM_SawDisc')
    manifest={}
    for file in sorted((ROOT/'Meshes').glob('*.obj')):
        v=[list(map(float,line.split()[1:])) for line in file.read_text().splitlines() if line.startswith('v ')]
        lo=[min(a[i] for a in v) for i in range(3)];hi=[max(a[i] for a in v) for i in range(3)]
        manifest[file.stem]={'min':lo,'max':hi,'extent':[hi[i]-lo[i] for i in range(3)]}
    (ROOT/'mesh_bounds.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Generated five enemies, health cross, shield and saw disc. Total meshes:',len(manifest))

if __name__=='__main__':generate()
