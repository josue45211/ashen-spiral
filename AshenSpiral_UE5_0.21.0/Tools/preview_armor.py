"""Render actual OBJ armor geometry for review; not an Unreal screenshot.
Requires numpy and Pillow outside Unreal only. Setup never imports this module.
"""
from pathlib import Path
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
DARK=(.13,.17,.22);METAL=(.30,.36,.43);CLOTH=(.035,.045,.065);BLUE=(.06,.64,1)

def obj(name):
    v=[];f=[]
    for s in (ROOT/'SourceArt/Meshes'/f'{name}.obj').read_text().splitlines():
        t=s.split()
        if t and t[0]=='v':v.append(list(map(float,t[1:])))
        if t and t[0]=='f':f.append([int(a.split('/')[0])-1 for a in t[1:]])
    return np.array(v)[f]
CACHE={}
def mesh(name):
    if name not in CACHE:CACHE[name]=obj(name)
    return CACHE[name].copy()

def segment(name,a,b,basis):
    a=np.array(a,float);b=np.array(b,float);axis=(a-b)/np.linalg.norm(a-b)
    y=np.cross(axis,[1,0,0]);y/=np.linalg.norm(y);x=np.cross(y,axis)
    rot=np.stack([x,y,axis],axis=1);p=mesh(name);p[:,:,2]*=np.linalg.norm(a-b)/basis
    return p@rot.T+(a+b)/2

def limb(a,b,pole,length=42):
    a=np.array(a,float);b=np.array(b,float);v=b-a;d=np.linalg.norm(v);axis=v/d;d=min(2*length-.001,d)
    side=np.array(pole)-a;side-=axis*np.dot(side,axis);side/=np.linalg.norm(side)
    j=a+axis*d/2+side*math.sqrt(max(0,length*length-d*d/4));return a,j,a+axis*d

def assembly(enemy=False,phase=0,moving=False):
    items=[]
    def add(name,pos,color,glow=False):
        p=mesh(name)
        if name in ('SM_Helmet','SM_EnemyHelm','SM_Visor'):p*=np.array([.95,1,.85])
        items.append((p+np.array(pos),color,glow))
    fire=(1,.09,.03) if enemy else BLUE
    metal=(.24,.27,.32) if enemy else METAL
    add('SM_Torso',(0,0,111),metal);add('SM_EnemyHelm' if enemy else 'SM_Helmet',(0,0,165),metal)
    add('SM_Pelvis',(0,0,78),DARK);add('SM_Visor',(0,0,165),fire,True);add('SM_ArmorCore',(0,0,111),fire,True)
    add('SM_Cape',(-22,0,141),CLOTH);add('SM_EnemyCape' if enemy else 'SM_CapeLower',(-25,0,86),CLOTH)
    for side in [-1,1]:
        add('SM_Pauldron',(0,side*34,135),metal)
        step=math.sin(phase+(math.pi if side>0 else 0)) if moving else 0
        a,j,b=limb((0,side*34,133),(10-step*14,side*38,66),(-8,side*78,96))
        items += [(segment('SM_Arm',a,j,42),DARK,False),(segment('SM_Forearm',j,b,42),metal,False)]
        add('SM_Hand',b,DARK)
        a,j,b=limb((0,side*16,79),(6+step*30,side*16,12+max(0,-step)*22),(70,side*19,47),38)
        items += [(segment('SM_Thigh',a,j,38),DARK,False),(segment('SM_Greave',j,b,38),metal,False)]
        add('SM_Boot',b+np.array([3,0,-2]),DARK)
    return [(p*np.array([.82,.70,1.]),c,g) for p,c,g in items]

def draw_model(canvas,items,center,angle,scale=3.4,elevation=.08):
    cam=np.array([math.cos(angle)*math.cos(elevation),math.sin(angle)*math.cos(elevation),math.sin(elevation)])
    right=np.array([-math.sin(angle),math.cos(angle),0]);up=np.cross(cam,right)
    light=np.array([.55,-.55,.7]);light/=np.linalg.norm(light);tris=[]
    for faces,color,glow in items:
        for f in faces:
            n=np.cross(f[1]-f[0],f[2]-f[0]);norm=np.linalg.norm(n)
            if norm<1e-9:continue
            n/=norm
            if np.dot(n,cam)<=0:continue
            shade=1.0 if glow else .38+.55*max(0,float(np.dot(n,light)))+.2*max(0,float(np.dot(n,cam)))
            rgb=tuple(int(min(255,c*shade*255)) for c in color)
            pts=[(center[0]+np.dot(v,right)*scale,center[1]-np.dot(v,up)*scale) for v in f]
            tris.append((np.dot(f.mean(axis=0),cam),pts,rgb))
    # Per-pixel depth resolves overlapping armor plates correctly in oblique views.
    zbuffer=np.full((canvas.height,canvas.width),-1e20);pixels=np.array(canvas)
    for faces,color,glow in items:
        for f in faces:
            n=np.cross(f[1]-f[0],f[2]-f[0]);norm=np.linalg.norm(n)
            if norm<1e-9:continue
            n/=norm
            if np.dot(n,cam)<=0:continue
            shade=1.0 if glow else .38+.55*max(0,float(np.dot(n,light)))+.2*max(0,float(np.dot(n,cam)))
            rgb=np.array([int(min(255,c*shade*255)) for c in color],dtype=np.uint8)
            p=np.array([[center[0]+np.dot(v,right)*scale,center[1]-np.dot(v,up)*scale,np.dot(v,cam)] for v in f])
            x0=max(0,int(p[:,0].min()));x1=min(canvas.width-1,math.ceil(p[:,0].max()))
            y0=max(0,int(p[:,1].min()));y1=min(canvas.height-1,math.ceil(p[:,1].max()))
            if x1<x0 or y1<y0:continue
            X,Y=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5)
            den=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
            if abs(den)<1e-8:continue
            a=((p[1,1]-p[2,1])*(X-p[2,0])+(p[2,0]-p[1,0])*(Y-p[2,1]))/den
            b=((p[2,1]-p[0,1])*(X-p[2,0])+(p[0,0]-p[2,0])*(Y-p[2,1]))/den;c=1-a-b
            depth=a*p[0,2]+b*p[1,2]+c*p[2,2];view=zbuffer[y0:y1+1,x0:x1+1]
            mask=(a>=0)&(b>=0)&(c>=0)&(depth>view);view[mask]=depth[mask]
            pixels[y0:y1+1,x0:x1+1][mask]=rgb
    canvas.paste(Image.fromarray(pixels))


if __name__=='__main__':
    im=Image.new('RGB',(1800,900),'#101923');d=ImageDraw.Draw(im)
    font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    title=ImageFont.truetype(font,30);small=ImageFont.truetype(font,18)
    d.text((55,32),'ASHEN SPIRAL  /  0.20 ARMOR',font=title,fill='#d4eaff')
    labels=['PLAYER / FRONT','PLAYER / THREE-QUARTER','PLAYER / CAPE','ENEMY / CORRUPTED ARMOR']
    for x,angle,enemy,label in zip([250,700,1120,1550],[0,-.72,math.pi,.60],[False,False,False,True],labels):
        draw_model(im,assembly(enemy),(x,785),angle,3.5)
        d.text((x-180,820),label,font=small,fill='#8ba8bc')
    d.text((55,870),'Actual source mesh geometry. Neutral preview lighting; Unreal materials and gameplay animation require an editor check.',font=small,fill='#8ba8bc')
    im.save(ROOT/'Docs/ARMOR_0.20_PREVIEW.png')
    frames=[]
    for i in range(36):
        frame=Image.new('RGB',(560,720),'#101923');draw_model(frame,assembly(False,i/36*math.tau,True),(280,660),-.6,3.2)
        ImageDraw.Draw(frame).text((22,20),'Joint / stride geometry preview',font=small,fill='#b6cfe0');frames.append(frame)
    frames[0].save(ROOT/'Docs/GAIT_GEOMETRY_PREVIEW.gif',save_all=True,append_images=frames[1:],duration=45,loop=0)
    print('Rendered armor and joint-motion geometry previews.')
