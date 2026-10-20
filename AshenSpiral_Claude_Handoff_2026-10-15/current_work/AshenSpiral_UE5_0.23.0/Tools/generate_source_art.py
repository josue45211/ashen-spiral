"""Regenerate the original stylized mesh/texture/audio kit. Optional developer tool.
The delivered SourceArt already contains its output; players need no Python install.
Requires numpy, Pillow and trimesh only for REGENERATION outside Unreal.
"""
from pathlib import Path
import math, json, wave
import numpy as np
import trimesh as tm
from PIL import Image, ImageFilter
ROOT=Path(__file__).resolve().parents[1]/'SourceArt'
for d in ('Meshes','Textures','Audio'): (ROOT/d).mkdir(parents=True,exist_ok=True)
rng=np.random.default_rng(1103)

def box(size,at=(0,0,0)):
 m=tm.creation.box(extents=size);m.apply_translation(at);return m

def ico(radius,at=(0,0,0),scale=(1,1,1),sub=1):
 m=tm.creation.icosphere(subdivisions=sub,radius=radius);m.vertices*=np.array(scale);m.apply_translation(at);return m

def cyl(radius,height,at=(0,0,0),sides=12):
 m=tm.creation.cylinder(radius=radius,height=height,sections=sides);m.apply_translation(at);return m

def link(a,b,r,sides=8):
 a=np.array(a,dtype=float);b=np.array(b,dtype=float);v=b-a
 m=tm.creation.cylinder(radius=r,height=np.linalg.norm(v),sections=sides)
 m.apply_transform(tm.geometry.align_vectors([0,0,1],v/np.linalg.norm(v)));m.apply_translation((a+b)/2);return m

def ring(r=50,th=3,height=3,n=64,arc=2*math.pi):
 vs=[];faces=[]
 for i in range(n+1):
  t=arc*i/n
  for rr,z in [(r-th,-height/2),(r+th,-height/2),(r+th,height/2),(r-th,height/2)]:vs.append((rr*math.cos(t),rr*math.sin(t),z))
 for i in range(n):
  for k in range(4):
   a=i*4+k;b=i*4+(k+1)%4;c=(i+1)*4+(k+1)%4;d=(i+1)*4+k
   faces.extend([[a,c,b],[a,d,c]]) # 0.24.2: wound outward (it was inside out: SM_Ring and SM_Slash drew their inner faces)
 return tm.Trimesh(vs,faces,process=False)

def beveled(size,bevel=8):
 x,y,z=np.array(size)/2
 xy=[(-x+bevel,-y),(x-bevel,-y),(x,-y+bevel),(x,y-bevel),(x-bevel,y),(-x+bevel,y),(-x,y-bevel),(-x,-y+bevel)]
 vs=[(a,b,h) for h in [-z,z] for a,b in xy];faces=[]
 for i in range(8):j=(i+1)%8;faces += [[i,j,j+8],[i,j+8,i+8]]
 for i in range(1,7):faces.extend([[0,i+1,i],[8,8+i,8+i+1]])
 return tm.Trimesh(vs,faces,process=True)

def save(name,parts):
 m=tm.util.concatenate(parts if isinstance(parts,list) else [parts]);m.remove_unreferenced_vertices();m.fix_normals()
 # Duplicate vertices per face to create stable planar UVs and deliberately faceted normals.
 v=m.vertices[m.faces].reshape(-1,3);f=np.arange(len(v)).reshape(-1,3);n=np.repeat(m.face_normals,3,axis=0)
 uv=np.zeros((len(v),2))
 for i in range(len(m.faces)):
  axes=np.argsort(np.abs(m.face_normals[i]))[:2];uv[i*3:i*3+3]=v[i*3:i*3+3][:,axes]/100.0
 out=tm.Trimesh(vertices=v,faces=f,vertex_normals=n,process=False)
 out.visual=tm.visual.texture.TextureVisuals(uv=uv)
 text=tm.exchange.obj.export_obj(out,include_normals=True,include_texture=True,write_texture=False)
 (ROOT/'Meshes'/f'{name}.obj').write_text(text,encoding='ascii')

save('SM_Tile',beveled((298,298,18),14))
pillar=[box((130,130,25),(0,0,12.5)),cyl(62,25,(0,0,37.5)),cyl(43,260,(0,0,180)),cyl(57,24,(0,0,320)),box((130,130,28),(0,0,346))]
for i in range(8):
 a=i*math.pi/4;pillar.append(link((43*math.cos(a),43*math.sin(a),60),(43*math.cos(a),43*math.sin(a),300),5))
save('SM_Pillar',pillar)
arch=[]
for i in range(18):
 a=(i+.5)*math.pi/18
 m=box((80,49,80));m.apply_transform(tm.transformations.rotation_matrix(a,(1,0,0)));m.apply_translation((0,220*math.cos(a),270+220*math.sin(a)));arch.append(m)
arch.extend([box((80,80,280),(0,-220,140)),box((80,80,280),(0,220,140))]);save('SM_Arch',arch)
save('SM_Book',beveled((35,15,54),2))
shelf=[box((85,280,18),(0,0,z)) for z in [10,78,146,214,282]]+[box((85,18,282),(0,y,141)) for y in [-139,139]]+[box((12,280,282),(-40,0,141))]
save('SM_Shelf',shelf)
save('SM_Obelisk',[cyl(115,35,(0,0,17.5),6),cyl(85,25,(0,0,47.5),6),tm.creation.cone(radius=58,height=350,sections=4),ring(90,5,7,32)])
save('SM_Crystal',[ico(70,(0,0,100),(.6,.6,2.3),0),ico(40,(45,10,55),(.7,.65,1.9),0),ico(37,(-27,30,47),(.6,.6,1.8),0)])
save('SM_Rock',[ico(90,scale=(1.1,.8,.75),sub=1)])
save('SM_Bone',[link((-55,0,0),(55,0,0),12),ico(20,(-55,0,0),(1,1.3,1)),ico(20,(55,0,0),(1,1.3,1))])
skull=[ico(47,(0,0,55),(.8,.95,1),2),box((35,57,18),(13,0,15))]
for y in [-20,-10,0,10,20]:skull.append(box((14,6,20),(31,y,23)))
save('SM_Skull',skull)
tree=[]
for i in range(5):
 tree.append(link((18*math.sin(i),8*math.cos(i),i*95),(18*math.sin(i+1),8*math.cos(i+1),(i+1)*95),55-i*7,9))
for i in range(7):
 a=i*2.4;p=(0,0,210+i*28);q=(170*math.cos(a),170*math.sin(a),420+i*12);tree.append(link(p,q,19-i,7))
save('SM_Tree',tree)
save('SM_Canopy',[ico(170,(180*math.cos(i*2.4),180*math.sin(i*2.4),460+i*12),(1.05,.9,.5),1) for i in range(7)])
save('SM_Leaf',[ico(85,scale=(.3,1.1,.12),sub=1)])
# SM_Chest and SM_ChestTrim: Tools/make_chest_meshes.py since 0.24.2 (this line's lid cylinder stood upright through the box: the chest drew as a stepped octagon)
save('SM_Ring',ring());save('SM_Slash',ring(50,3,4,32,math.pi*1.4))
save('SM_Brazier',[cyl(65,24,(0,0,12),10),cyl(25,100,(0,0,60),8),cyl(65,25,(0,0,113),12)])
save('SM_Torso',beveled((42,62,64),12))
save('SM_Pelvis',beveled((36,52,28),8))
save('SM_Hood',[ico(30,(0,0,0),(1,1,1.18),2),box((18,47,28),(15,0,-12))])
save('SM_Pauldron',beveled((52,38,25),9))
save('SM_Arm',beveled((23,25,42),6))
save('SM_Leg',beveled((28,26,40),6))
save('SM_Boot',beveled((48,28,22),5))
save('SM_Hand',ico(13,scale=(1,.9,1.1),sub=1))
vs=[];fs=[]
for iz in range(7):
 for iy in range(9):
  z=-iz*16-(8 if iz==6 and iy%2 else 0);y=(iy-4)*(9+iz*.9);x=-abs(iy-4)*1.4-iz*4
  vs.append((x,y,z))
for iz in range(6):
 for iy in range(8):
  a=iz*9+iy;fs.extend([[a,a+9,a+1],[a+1,a+9,a+10]])
cape=tm.Trimesh(vs,fs,process=True);save('SM_Cape',[cape,tm.Trimesh(vs,np.array(fs)[:,::-1],process=False)])
from generate_weapon_art import generate as generate_weapons
save('SM_Wraith',[ico(40,(0,0,118),(.8,1,1.2),1),tm.creation.cone(radius=65,height=130,sections=9),ico(20,(0,-65,90),(.6,.7,1.3),1),ico(20,(0,65,90),(.6,.7,1.3),1)])
save('SM_Brute',[beveled((85,115,125),20),ico(40,(0,0,85),(1,1,1),1),beveled((125,38,55),10)])
generate_weapons() # Also writes dimensions for the complete mesh kit.
# Seamless layered-noise stone: original synthesized maps, not external images.
n=512
noise=rng.random((n,n))
low=Image.fromarray((noise*255).astype('uint8')).filter(ImageFilter.GaussianBlur(18))
mid=Image.fromarray((noise*255).astype('uint8')).filter(ImageFilter.GaussianBlur(2))
h=.62*np.array(low)/255+.28*np.array(mid)/255+.10*noise
h=(h-h.min())/(h.max()-h.min())
a=np.clip(.28+.6*h,0,1)
Image.fromarray((np.stack([a,a*.965,a*.9],axis=-1)*255).astype('uint8')).save(ROOT/'Textures'/'T_Stone.png')
dx=np.roll(h,1,axis=1)-np.roll(h,-1,axis=1);dy=np.roll(h,1,axis=0)-np.roll(h,-1,axis=0)
v=np.stack([dx*2,dy*2,np.ones_like(h)],axis=-1);v/=np.linalg.norm(v,axis=-1,keepdims=True)
Image.fromarray(((v*.5+.5)*255).astype('uint8')).save(ROOT/'Textures'/'T_Normal.png')
Image.fromarray((np.clip(.7+h*.25,0,1)*255).astype('uint8')).save(ROOT/'Textures'/'T_Roughness.png')
# Compact original PCM cues; no voice cloning, external samples or dependencies at runtime.
sr=44100
for name,duration,f0,f1 in [('Attack',.16,380,90),('Dash',.23,190,850),('Spiral',.6,160,520),('Hit',.12,110,45),('Reward',.65,440,880),('Collapse',.45,180,38)]:
 t=np.arange(int(sr*duration))/sr
 phase=2*np.pi*(f0*t+(f1-f0)*t*t/(2*duration));env=np.sin(np.pi*np.minimum(t/.018,1)/2)*np.exp(-t/(duration*.33))
 data=(.17*np.sin(phase)+.04*rng.normal(size=len(t)))*env
 if name=='Reward':data=.13*(np.sin(2*np.pi*440*t)+np.sin(2*np.pi*554.37*t)+np.sin(2*np.pi*659.25*t))*np.exp(-t*5)*np.minimum(1,t/.02)
 with wave.open(str(ROOT/'Audio'/f'S_{name}.wav'),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(sr);w.writeframes((np.clip(data,-1,1)*32767).astype('<i2').tobytes())
print('Generated',len(list((ROOT/'Meshes').glob('*.obj'))),'original mesh assets, 3 textures and 6 audio cues.')
