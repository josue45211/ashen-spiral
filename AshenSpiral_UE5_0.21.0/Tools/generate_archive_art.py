"""0.21 original Archives kit. No external images or third-party assets.
Regeneration requires numpy/Pillow; the delivered meshes and maps are ready to import.
Written gothic-archive direction only: the referenced P2 pack was not available.
"""
from generate_weapon_art import Mesh, ROOT
import math
import json
import random
import numpy as np
from PIL import Image, ImageFilter, ImageDraw

def moved_box(m, center, size, angle=0):
    part=Mesh();part.box((0,0,0),size);c,s=math.cos(angle),math.sin(angle)
    def point(p):return (p[0]*c-p[1]*s+center[0],p[0]*s+p[1]*c+center[1],p[2]+center[2])
    for _,a,b,d,n in part.faces:m.tri(point(a),point(b),point(d))

def geometry():
    for broken in (False,True):
        m=Mesh()
        # Front is +X. Vertical planks and a cornice make orientation unmistakable.
        for y in range(-126,127,28):m.box((-40,y,141),(10,26,282 if not broken or y<0 else 151))
        for z in (10,78,146,214,282):
            if broken and z>146:continue
            m.box((0,0,z),(85,280,12));m.box((41,0,z-5),(9,280,15))
        for y in (-139,139):m.box((0,y,141),(85,16,282 if not broken or y<0 else 150))
        if not broken:
            m.box((0,0,292),(100,300,16));m.box((41,0,273),(12,292,10))
            for y in (-139,139):m.box((43,y,146),(12,22,292))
        else:
            moved_box(m,(25,25,9),(75,210,11),.38)
            moved_box(m,(12,35,160),(72,130,11),-.23)
        m.save('SM_ArchiveShelfBroken' if broken else 'SM_ArchiveShelf')
    for broken in (False,True):
        m=Mesh()
        if not broken:
            for y in (-44,44):
                for x in (-73,73):m.box((x,y,48),(13,13,92))
            m.box((0,0,96),(185,116,12));m.box((0,0,23),(165,9,9))
            m.box((0,53,108),(185,9,14))
        else:
            for x in (-73,73):m.box((x,-44,39),(13,13,74))
            moved_box(m,(0,-24,78),(185,61,12),.08)
            moved_box(m,(-9,31,14),(170,55,12),-.32)
            moved_box(m,(48,20,8),(13,83,13),.67)
        m.save('SM_ArchiveDeskBroken' if broken else 'SM_ArchiveDesk')
    m=Mesh()
    # Lancet arch, entirely above head height. Two curved shoulders meet at a point.
    for side in (-1,1):
        pts=[(0,side*450,340),(0,side*450,555)]
        for j in range(1,15):
            t=j/14;pts.append((0,side*450*(1-t*t),555+470*t))
        for a,b in zip(pts,pts[1:]):m.link(a,b,18,8)
        m.box((0,side*450,354),(60,65,28))
        for x in (-23,23):
            for a,b in zip(pts,pts[1:]):m.link((x,a[1],a[2]),(x,b[1],b[2]),5,6)
    m.box((0,0,1020),(65,35,55));m.save('SM_ArchiveRib')
    m=Mesh()
    for side in (-1,1):
        m.link((0,side*45,0),(0,side*45,60),3,6)
        m.link((0,side*45,60),(0,0,85),3,6)
    m.link((0,-50,10),(0,50,10),4,8);m.link((0,-50,52),(0,50,52),3,8)
    m.box((0,0,3),(30,112,6));m.save('SM_ArchiveIronwork')
    m=Mesh();rng=random.Random(2100)
    for j in range(16):
        x,y=rng.uniform(-56,56),rng.uniform(-42,42)
        moved_box(m,(x,y,2+j%3),(rng.uniform(10,27),rng.uniform(9,20),1.2),rng.uniform(-3,3))
    for j in range(5):
        moved_box(m,(rng.uniform(-30,30),rng.uniform(-28,28),5+j),(rng.uniform(25,70),7,5),rng.uniform(-2,2))
    m.save('SM_ArchiveDebris')
    bounds={}
    for p in sorted((ROOT/'Meshes').glob('*.obj')):
        points=[list(map(float,l.split()[1:])) for l in p.read_text().splitlines() if l.startswith('v ')]
        lo=[min(v[i] for v in points) for i in range(3)];hi=[max(v[i] for v in points) for i in range(3)]
        bounds[p.stem]={'min':lo,'max':hi,'extent':[hi[i]-lo[i] for i in range(3)]}
    (ROOT/'mesh_bounds.json').write_text(json.dumps(bounds,indent=2)+'\n')

def textures():
    n=512;rng=np.random.default_rng(2100);Y,X=np.mgrid[:n,:n]
    def blurred(radius):
        raw=Image.fromarray(rng.integers(0,256,(n,n),dtype=np.uint8))
        # Wrap before blurring so the maps tile without a hard border.
        tiled=Image.fromarray(np.tile(np.asarray(raw),(3,3)))
        return np.asarray(tiled.filter(ImageFilter.GaussianBlur(radius)),dtype=float)[n:2*n,n:2*n]/255
    coarse=blurred(24);mid=blurred(4);grain=rng.random((n,n))
    coarse=(coarse-coarse.min())/max(.001,np.ptp(coarse))
    mid=(mid-mid.min())/max(.001,np.ptp(mid))
    for kind in ('Stone','Wood','Rust'):
        if kind=='Stone':
            rows=Y//128;bx=(X+(rows%2)*128)%256;by=Y%128
            edge=np.minimum(np.minimum(bx,255-bx),np.minimum(by,127-by))
            seam=np.clip(edge/7,0,1)
            cracks=Image.new('L',(n,n),0);d=ImageDraw.Draw(cracks)
            for j in range(18):
                x,y=map(int,rng.integers(20,480,2));pts=[(x,y)]
                for k in range(5):x+=int(rng.integers(-13,14));y+=int(rng.integers(7,19));pts.append((x,y))
                d.line(pts,fill=int(rng.integers(70,145)),width=1)
            crack=np.asarray(cracks)/255
            damp=np.clip((.44-coarse)*2.7,0,.65)
            height=.30*seam+.15*mid+.04*grain-.11*crack
            value=(.42+.19*mid+.10*grain)*(.64+.36*seam)*(1-damp*.6)*(1-crack*.65)
            rgb=np.stack([value*.94,value*.98,value],axis=-1)
            rough=.77+.17*mid-damp*.12
        elif kind=='Wood':
            streak=np.sin(X*.32+np.sin(Y*.031)*2+coarse*7)
            plank=np.clip(np.minimum(X%128,127-X%128)/5,0,1)
            height=.12*plank+.055*streak+.03*grain
            value=(.46+.13*streak+.13*mid+.08*grain)*(.58+.42*plank)
            rgb=np.stack([value,value*.78,value*.55],axis=-1)
            rough=.79+.13*mid
        else:
            rust=np.clip((coarse-.28)*2.1,0,1)
            height=.09*mid+.08*grain+.13*rust
            base=.26+.10*mid
            rgb=np.stack([base+rust*.35,base+rust*.055,base-rust*.10],axis=-1)
            rough=.52+.42*rust
        dx=np.roll(height,1,1)-np.roll(height,-1,1);dy=np.roll(height,1,0)-np.roll(height,-1,0)
        normal=np.stack([dx*3,dy*3,np.ones_like(height)],axis=-1);normal/=np.linalg.norm(normal,axis=-1,keepdims=True)
        dest=ROOT/'Textures'/('T_Archive'+kind)
        Image.fromarray((np.clip(rgb,0,1)*255).astype('uint8')).save(str(dest)+'.png')
        Image.fromarray(((normal*.5+.5)*255).astype('uint8')).save(str(dest)+'_Normal.png')
        Image.fromarray((np.clip(rough,0,1)*255).astype('uint8')).save(str(dest)+'_Roughness.png')

if __name__=='__main__':
    geometry();textures();print('Generated 7 Archives meshes and 9 original aged material maps.')
