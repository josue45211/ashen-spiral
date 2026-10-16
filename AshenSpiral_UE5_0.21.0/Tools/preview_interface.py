"""Optional source-layout illustration (Pillow). Not an Unreal screenshot.
Read glyph geometry and tuning exported by Tools/export_interface.cpp.
"""
from pathlib import Path
import json, math, textwrap
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
DATA=json.loads((ROOT/'Docs/interface_source.json').read_text())
S=1.5
im=Image.new('RGB',(1920,1200),'#111b25');d=ImageDraw.Draw(im)
FONT=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
BOLD=FONT.with_name('DejaVuSans-Bold.ttf')
if not FONT.exists():
    import matplotlib.font_manager as fm
    FONT=Path(fm.findfont('DejaVu Sans'));BOLD=Path(fm.findfont(fm.FontProperties(family='DejaVu Sans',weight='bold')))
def xy(box): return tuple(int(v*S) for v in box)
def font(size,bold=False): return ImageFont.truetype(str(BOLD if bold else FONT),int(size*S))
def text(x,y,v,size=14,color='#c6d2db',bold=False): d.text(xy((x,y)),v,font=font(size,bold),fill=color,spacing=int(5*S))
def rect(box,color,width=0):
    if width:d.rectangle(xy(box),outline=color,width=max(1,int(width*S)))
    else:d.rectangle(xy(box),fill=color)
def wrap(v,w,size,bold=False):
    lines=[];line=''
    for word in v.split():
        candidate=(line+' '+word).strip()
        if d.textlength(candidate,font=font(size,bold))>w*S and line:lines.append(line);line=word
        else:line=candidate
    return '\n'.join(lines+[line])
def icon(k,x,y,size,color):
    rgb=tuple(int(color[i:i+2],16) for i in (1,3,5))
    for stroke in DATA['icons'][k]:
        tint=tuple(int(c*stroke['alpha']+24*(1-stroke['alpha'])) for c in rgb)
        points=[xy((x+p[0]*size/100,y+p[1]*size/100)) for p in stroke['points']]
        if len(points)>1:d.line(points,fill=tint,width=max(1,int(stroke['width']*size/100*S)),joint='curve')
# Subdued abstract floor texture; this represents spacing, not world art.
for i in range(0,1280,100):d.line(xy((i,0,i-330,800)),fill='#172431',width=1)
text(80,28,'ASHEN SPIRAL / 0.20 INTERFACE SOURCE PREVIEW',12,'#78909f',True)
text(80,90,'CLAIM YOUR REWARD',29,'#f3f6f8',True)
text(80,132,'Choose one. Shape the next fight.',15,'#ffc267')
text(1090,104,'CLOSE / ESC',12,'#aec0cb',True)
cards=[('Expanding Force','Larger blasts and auras',8,0),('Ability Cooldown','Cast Spiral more often',11,0),('Emberforge Evolution','Evolve an owned trait',8,2),('Tight Grouping','Tighter pellets and bonus shots',12,0),('Forked Flight','Hits split into extra shots',6,1)]
for i,(name,brief,kind,rarity) in enumerate(cards):
    x=80+i*212;y=180;c=['#75bac4','#a17bff','#ffab3d'][rarity]
    rect((x,y,x+200,y+368),c);rect((x+1,y+1,x+199,y+367),'#0a121e' if i!=1 else '#1a2d3c')
    grade=wrap(('CORE STAT' if i==1 else 'TRAIT')+' / '+['COMMON','RARE','LEGENDARY'][rarity],170,14,True)
    text(x+13,y+13,grade,14,c,True)
    icon_y=y+13+len(grade.splitlines())*21+8
    icon(kind,x+59,icon_y,82,c)
    title=wrap(name,168,18,True);title_y=icon_y+86
    text(x+13,title_y,title,18,'#f5f7f9',True)
    text(x+13,title_y+len(title.splitlines())*27+7,wrap(brief,172,14),14)
    text(x+129,y+334,'SELECT',13,c,True)
text(80,570,'Hover a card + hold SHIFT for exact values and conditions.',14,'#a1b5c4')
rect((80,652,242,691),'#3c525f',1);text(99,664,'REROLL / 1',14,'#d6e4eb',True)
rect((274,663,292,681),'#ffc267',1);d.line(xy((278,671,283,676,289,666)),fill='#ffc267',width=3)
icon(13,307,646,44,'#ffc267');text(366,648,'RAISE THE STAKES',17,'#ffc267',True)
text(366,676,'Harder chamber. Better rare rewards. Stays checked.',12,'#afbfca')
text(1040,664,'75 CINDERS',14,'#ffc267',True)
text(80,720,'1/1 STAKES VOTES  /  NEXT CHAMBER: TIER 1  /  Hold SHIFT for stakes details',12,'#91a9b9')
text(80,773,'Original runtime glyphs; layout illustration only. Unreal rendering and input have not been captured here.',11,'#78909f')
im.save(ROOT/'Docs/REWARD_CARDS_PREVIEW.png')
# Same card geometry with skill-specific frame and distinct illustration.
im2=Image.new('RGB',(1600,740),'#111b25');im_old=im;draw_old=d;im=im2;d=ImageDraw.Draw(im)
text(40,25,'ACTIVE SKILLS / SOURCE COMPONENT PREVIEW',22,'#f5f7f9',True)
for i,(name,brief,k) in enumerate([('Rune Lance','Pierce with runic power',5),('Winter Fan','Freeze a cone of foes',4),('War Cry','Shield and melee boost',1),('Cinder Mine','Set an explosive rune',2)]):
    x=40+i*248;y=85;color='#a17bff';rect((x,y,x+232,y+368),color);rect((x+3,y+3,x+229,y+365),'#09202b')
    text(x+14,y+14,'ACTIVE SKILL / RARE',14,color,True);icon(k,x+75,y+49,82,color)
    title=wrap(name,200,18,True);text(x+14,y+135,title,18,'#f5f7f9',True)
    text(x+14,y+135+len(title.splitlines())*27+7,wrap(brief,204,14),14);text(x+103,y+334,'EQUIP + TEST',13,color,True)
text(40,470,'Source layout illustration. These are not Unreal screenshots.',11,'#78909f')
im.save(ROOT/'Docs/SKILL_CARDS_PREVIEW.png')
print('Generated reward and skill source-layout previews; no engine screenshot is implied.')
