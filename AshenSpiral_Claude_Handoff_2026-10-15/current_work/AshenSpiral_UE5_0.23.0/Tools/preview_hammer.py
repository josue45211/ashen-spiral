"""Render actual runtime hammer pose samples and source OBJ geometry; not Unreal captures.
Run export_hammer.cpp first. Requires numpy + Pillow for the offline review only.
"""
from preview_armor import ROOT, mesh, assembly, limb, segment, draw_model, DARK, METAL, CLOTH
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import json,math
DATA=json.loads((ROOT/'Docs/HAMMER_0.22_SAMPLES.json').read_text())
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
SMALL=ImageFont.truetype(FONT,15);TITLE=ImageFont.truetype(FONT,25)
SCALE=np.array([.82,.70,1.])
def rotation(pitch,yaw,roll=0):
    a,b,c=map(math.radians,(pitch,yaw,roll));ca,sa,cb,sb,cc,sc=math.cos(a),math.sin(a),math.cos(b),math.sin(b),math.cos(c),math.sin(c)
    return np.array([[cb,-sb,0],[sb,cb,0],[0,0,1]])@np.array([[ca,0,sa],[0,1,0],[-sa,0,ca]])@np.array([[1,0,0],[0,cc,-sc],[0,sc,cc]])
def pose(mode,frame):
    sample=DATA[mode*241+frame];p=sample['pose'];rot=np.array(sample['rotation']).T;body=rotation(p[7],p[6]);g=np.array(p[:3]);items=[]
    hand=rotation(p[3],p[4])@rotation(0,p[5])
    crouch=-16*(3*(frame/240)**2-2*(frame/240)**3) if mode==3 else (-16*(1-(3*min(1,frame/240/.28)**2-2*min(1,frame/240/.28)**3)) if mode==4 else 0)
    for i,(f,c,glow) in enumerate(assembly()):
        if i in (8,9,10,15,16,17) or (mode in (2,3,5) and i in (11,12,13,18,19,20)):continue
        f=f/SCALE
        if i in (0,4,7,14): f=(f-[0,0,111])@body.T+[0,0,111]
        if i==2:f=(f-[0,0,78])@rotation(0,p[6]*.35).T+[0,0,78]
        if i in (1,3):f=(f-[0,0,165])@rotation(p[7]*.3,p[6]*.35).T+[0,0,165]
        if i in (5,6):f=(f-[-22,0,141])@rotation(4+abs(p[7])*.45,-p[6]*.45,p[6]*.16).T+[-22,0,141]
        items.append((f,c,glow))
    for side,target in [(-1,g+rot@[0,0,24]),(1,g)]:
        root=body@np.array([0,side*34,22])+[0,0,111]
        a,j,b=limb(root,target,(-8,side*78,96));items.extend([(segment('SM_Arm',a,j,42),DARK,False),(segment('SM_Forearm',j,b,42),METAL,False),(mesh('SM_Hand')@hand.T+b,DARK,False)])
    if mode in (2,3,5):
        for side in [-1,1]:
            a,j,b=limb((0,side*16,79),(24 if side==-1 else -12,side*24,12-crouch),(70,side*19,47),38)
            items.extend([(segment('SM_Thigh',a,j,38),DARK,False),(segment('SM_Greave',j,b,38),METAL,False),(mesh('SM_Boot')+b+[3,0,-2],DARK,False)])
    items.append((mesh('SM_Weapon1')@rot.T+g,(.52,.49,.41),False))
    return [(f*SCALE+[0,0,crouch],c,glow) for f,c,glow in items]
def sheet():
    im=Image.new('RGB',(1800,1000),'#10171f');d=ImageDraw.Draw(im)
    d.text((35,24),'ASHEN SPIRAL 0.22 / GRAV HAMMER',font=TITLE,fill='#e4d4b3')
    d.text((35,62),'Source mesh + runtime pose review. Reference frame mapping below; engine lighting, physics and co-op are untested.',font=SMALL,fill='#9cabb5')
    frames=[(0,60,'1 / LEFT COIL'),(0,134,'2 / LEFT SWING'),(1,60,'3 / RIGHT COIL'),(1,134,'4 / RIGHT SWING'),(1,187,'5 / FOLLOW-THROUGH'),(2,75,'6 / OVERHEAD RAISE'),(2,115,'7 / HEAVY WIND-UP'),(2,182,'8 / PLANTED IMPACT'),(2,207,'9 / WEIGHT TRANSFER'),(2,225,'10 / RECOVERY')]
    for i,(m,f,label) in enumerate(frames):
        x=180+i%5*360;y=500 if i<5 else 950
        draw_model(im,pose(m,f),(x,y-50),-.65,1.45,elevation=.07)
        d.text((x-160,y-8),label,font=SMALL,fill='#c5bdad')
    im.save(ROOT/'Docs/HAMMER_0.22_COMBO_PREVIEW.png')
    im=Image.new('RGB',(1800,650),'#10171f');d=ImageDraw.Draw(im);d.text((35,20),'RMB / LEAP + RADIAL LANDING',font=TITLE,fill='#e4d4b3')
    d.text((35,60),'Pose review only. 0.16s vulnerable startup / airborne protection / 0.22s landing recovery / 7.5s cooldown.',font=SMALL,fill='#9cabb5')
    for i,(m,f,label) in enumerate([(3,240,'CROUCH'),(4,50,'LAUNCH'),(4,120,'APEX'),(4,200,'DESCENT'),(5,0,'LANDING'),(5,200,'RECOVER')]):
        x=150+i*300;draw_model(im,pose(m,f),(x,575),-.65,1.6,elevation=.07);d.text((x-90,610),label,font=SMALL,fill='#c5bdad')
    im.save(ROOT/'Docs/HAMMER_0.22_LEAP_PREVIEW.png')
def animation():
    images=[]
    for mode,count in [(0,14),(1,14),(2,24)]:
        for i in range(count):
            f=round(i/(count-1)*240);im=Image.new('RGB',(600,560),'#10171f');d=ImageDraw.Draw(im)
            d.text((20,18),'0.22 / HAMMER SOURCE ANIMATION',font=SMALL,fill='#d7c6a8')
            draw_model(im,pose(mode,f),(300,515),-.65,1.55,elevation=.07)
            d.text((20,535),f'Hit {mode+1}  /  source geometry; not an Unreal capture',font=SMALL,fill='#9cabb5');images.append(im)
    images[0].save(ROOT/'Docs/HAMMER_0.22_COMBO.gif',save_all=True,append_images=images[1:],duration=50,loop=0)
if __name__=='__main__':sheet();animation();print('Rendered 10-frame combo, leap poses, and source animation GIF.')
