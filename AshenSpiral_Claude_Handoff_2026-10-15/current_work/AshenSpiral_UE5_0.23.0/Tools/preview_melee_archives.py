"""Source geometry review images, not Unreal captures. numpy + Pillow required.
First compile export_melee.cpp and write Docs/MELEE_ARCHIVES_SAMPLES.json.
"""
from preview_armor import ROOT, mesh, assembly, limb, segment, draw_model, DARK, METAL
import json
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
DATA=json.loads((ROOT/'Docs/MELEE_ARCHIVES_SAMPLES.json').read_text())
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
TITLE=ImageFont.truetype(FONT,28);SMALL=ImageFont.truetype(FONT,17);LABEL=ImageFont.truetype(FONT,21)
SCALE=np.array([.82,.70,1.])
def rotation(pitch,yaw):
    a,b=map(math.radians,(pitch,yaw));c,s=math.cos(a),math.sin(a);d,e=math.cos(b),math.sin(b)
    return np.array([[d,-e,0],[e,d,0],[0,0,1]])@np.array([[c,0,s],[0,1,0],[-s,0,c]])
def pose(w,step,frame):
    p=next(p for p in DATA['poses'] if (p['weapon'],p['step'],p['frame'])==(w,step,frame))
    x,y,z,pitch,yaw,roll,bodyyaw,bodypitch=p['pose'];rot=rotation(pitch,yaw)
    items=[]
    for i,(f,c,g) in enumerate(assembly()):
        if i in (8,9,10,15,16,17):continue
        f=f/SCALE
        if i in (0,4):f=(f-[0,0,111])@rotation(bodypitch,bodyyaw).T+[0,0,111]
        if i==2:f=(f-[0,0,78])@rotation(0,bodyyaw*.35).T+[0,0,78]
        items.append((f,c,g))
    grip=np.array([x,y,z]);support=np.array([12,-39,86]) if w==0 else grip+rot@np.array([0,0,p['support']])
    for side,target in [(-1,support),(1,grip)]:
        a,j,b=limb((0,side*34,133),target,(-8,side*78,96))
        items.extend([(segment('SM_Arm',a,j,42),DARK,False),(segment('SM_Forearm',j,b,42),METAL,False)])
        handrot=rot if w!=0 or side==1 else np.eye(3)
        items.append((mesh('SM_Hand')@handrot.T+b,DARK,False))
    items.append((mesh('SM_Weapon'+str(w))@rot.T+grip,(.46,.47,.49),False))
    return [(f*SCALE,c,g) for f,c,g in items]

def melee():
    im=Image.new('RGB',(1800,1120),'#0d151e');d=ImageDraw.Draw(im)
    d.text((40,24),'ASHEN SPIRAL 0.21  /  MELEE FOUNDATION',font=TITLE,fill='#dceaf0')
    d.text((40,67),'Actual armor + weapon meshes and runtime pose curves. No Unreal rendering or movement simulation.',font=SMALL,fill='#90a7b8')
    panels=[(0,0,0,'BLADE / READY'),(0,0,48,'1 / CUT'),(0,1,48,'2 / RETURN CUT'),(0,2,60,'3 / LUNGE IMPACT'),
            (1,0,0,'HAMMER / TWO HANDS'),(1,0,56,'1 / LEFT SWING'),(1,1,56,'2 / RIGHT SWING'),(1,2,42,'3 / CHARGED WIND-UP')]
    for i,(w,s,f,label) in enumerate(panels):
        x=225+(i%4)*450;y=570 if i<4 else 1070
        draw_model(im,pose(w,s,f),(x,y-42),-.65,1.85,elevation=.04)
        d.text((x-185,y-2),label,font=SMALL,fill='#d9c298' if w else '#9fcfdf')
    im.save(ROOT/'Docs/MELEE_0.21_PREVIEW.png')
    im=Image.new('RGB',(1400,730),'#0d151e');d=ImageDraw.Draw(im)
    d.text((35,24),'HAMMER FINISHER + SCYTHE GRIP',font=TITLE,fill='#dceaf0')
    for x,w,frame,label in [(235,1,42,'LIFT / 42%'),(710,1,76,'IMPACT / 76%'),(1150,5,48,'SCYTHE / IMPACT')]:
        draw_model(im,pose(w,2 if w==1 else 0,frame),(x,650),-.65,2.15,elevation=.03)
        d.text((x-150,670),label,font=LABEL,fill='#bfcdd7')
    im.save(ROOT/'Docs/FINISHER_0.21_PREVIEW.png')

def archives():
    im=Image.new('RGB',(1600,1120),'#10171d');d=ImageDraw.Draw(im)
    d.text((36,22),'FORGOTTEN ARCHIVES  /  SOURCE ART REVIEW',font=TITLE,fill='#e2d5bb')
    d.text((36,65),'Written direction: ancient gothic archive. P2 reference pack unavailable. Layout and meshes, not a game capture.',font=SMALL,fill='#9dabad')
    names=['SM_ArchiveShelf','SM_ArchiveShelfBroken','SM_ArchiveDesk','SM_ArchiveDeskBroken','SM_Pillar','SM_ArchiveDebris']
    colors=[(.32,.23,.15),(.28,.20,.13),(.32,.23,.15),(.27,.19,.12),(.30,.32,.34),(.56,.48,.32)]
    for i,plan in enumerate(DATA['archives']):
        left=36+i*390;top=125;size=350;k=size/2400
        d.rectangle((left,top,left+size,top+size),fill='#202a31',outline='#63727a',width=2)
        for rect in [(left+size/2-360*k,top,left+size/2+360*k,top+size),(left,top+size/2-360*k,left+size,top+size/2+360*k)]:d.rectangle(rect,fill='#293841')
        for typ,x,y,yaw,sc,zs,r in plan['props']:
            f=mesh(names[typ]);f[:,:,0:2]*=sc;f[:,:,2]*=zs;f=f@rotation(0,yaw).T+[x,y,0]
            for tri in f:
                rgb=tuple(int(v*255) for v in colors[typ]);d.polygon([(left+size/2+v[0]*k,top+size/2-v[1]*k) for v in tri],fill=rgb)
            if typ in (0,1):
                a=math.radians(yaw);cx=left+size/2+x*k;cy=top+size/2-y*k
                d.line((cx,cy,cx+math.cos(a)*19,cy-math.sin(a)*19),fill='#8cb8c4',width=2)
        d.text((left,490),f'MOTIF {plan["motif"]+1}  /  OPEN CENTRAL PATHS',font=SMALL,fill='#9fb8bf')
    props=[('SM_ArchiveShelf',colors[0],235,.85),('SM_ArchiveShelfBroken',colors[1],520,.85),('SM_ArchiveDeskBroken',colors[3],795,1.3),('SM_ArchiveRib',colors[4],1160,.31)]
    for name,c,x,sc in props:draw_model(im,[(mesh(name),c,False)],(x,840),.58,sc,elevation=.16)
    for x,label in [(95,'INWARD-FACING STACK'),(390,'BROKEN STACK'),(675,'COLLAPSED TABLE'),(1070,'POINTED RIB')]:d.text((x,865),label,font=SMALL,fill='#c4c9c6')
    for i,(name,label) in enumerate([('Stone','DAMP / CRACKED STONE'),('Wood','WORN TIMBER'),('Rust','CORRODED IRON')]):
        patch=Image.open(ROOT/'SourceArt/Textures'/('T_Archive'+name+'.png')).convert('RGB').resize((360,140))
        x=70+i*510;im.paste(patch,(x,918));d.text((x,1073),label,font=SMALL,fill='#b8c2bf')
    im.save(ROOT/'Docs/ARCHIVES_0.21_PREVIEW.png')

if __name__=='__main__':melee();archives();print('Rendered melee grip, finisher and Archives source previews.')
