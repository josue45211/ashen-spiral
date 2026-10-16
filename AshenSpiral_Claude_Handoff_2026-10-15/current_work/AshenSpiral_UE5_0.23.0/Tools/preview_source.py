"""Optional documentation previews; requires matplotlib. Normal setup does not.
Uses the actual exported C++ layouts and original OBJ vertices. Not UE screenshots.
"""
from pathlib import Path
import json
import math
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

ROOT=Path(__file__).resolve().parents[1]
BG='#101a23';INK='#ecede2';GOLD='#efbd70';TEAL='#74d6ca'
plt.rcParams.update({'font.family':'DejaVu Sans','text.color':INK,'axes.titlecolor':INK})
layouts=sorted(json.loads((ROOT/'Docs/layout_samples.json').read_text()),key=lambda item:item['shape'])
fig,axes=plt.subplots(2,3,figsize=(15,10),facecolor=BG)
names=['Courts','Paired galleries','Cross sanctuary','Cloister ring','Winding gallery']
for ax,item,title in zip(axes.flat,layouts,names):
    ax.set_facecolor(BG)
    rooms=item['rooms']
    for a,b,gap in item['edges']:
        ax.plot([rooms[a][0],rooms[b][0]],[rooms[a][1],rooms[b][1]],color=GOLD if gap else '#5d7884',lw=5,ls='--' if gap else '-',zorder=1)
    for i,(x,y,hx,hy,sx,sy) in enumerate(rooms):
        color=TEAL if i==0 else (GOLD if i==item['exit'] else '#658994')
        ax.add_patch(Rectangle((x-hx,y-hy),hx*2,hy*2,facecolor='#243947',edgecolor=color,lw=1.6,zorder=2))
        ax.text(x,y,f'{sx} × {sy}',ha='center',va='center',fontsize=6 if sx==1 else 10,color=color,zorder=3)
        if i==0 or i==item['exit']:ax.text(x,y-hy*.58,'START' if i==0 else 'CHEST',ha='center',fontsize=4.5 if sx==1 else 6,color=color,zorder=3)
    ax.autoscale_view();ax.margins(.10);ax.set_aspect('equal');ax.axis('off')
    ax.set_title(f'{title}\n{len(rooms)} rooms · seed {item["seed"]}',fontsize=14,pad=18,loc='left')
axes.flat[-1].axis('off')
axes.flat[-1].text(.02,.74,'5–10 actual rooms',fontsize=24,transform=axes.flat[-1].transAxes)
axes.flat[-1].text(.02,.58,'1×1 · 2×2 · 2×3 · 3×2 · 3×3\n\nRoom areas are drawn to scale.\nSolid corridors connect every room.\nDashed gold = optional jump shortcut.\n\nSource layout diagrams, not game renders.',fontsize=13,linespacing=1.6,transform=axes.flat[-1].transAxes,color='#a4bac4',va='top')
fig.suptitle('ASHEN SPIRAL  /  0.18 ROOM LAYOUTS',x=.06,y=.99,ha='left',fontsize=23,color=GOLD)
fig.tight_layout(pad=3);fig.savefig(ROOT/'Docs/LAYOUT_OVERVIEW.png',dpi=130,facecolor=BG);plt.close(fig)

fig=plt.figure(figsize=(15,5),facecolor=BG)
for slot,(number,title) in enumerate([(6,'Cinder Scattergun'),(7,'Tempest Rotary'),(8,'Rift Mortar')],1):
    vertices=[];faces=[]
    for line in (ROOT/f'SourceArt/Meshes/SM_Weapon{number}.obj').read_text().splitlines():
        values=line.split()
        if not values:continue
        if values[0]=='v':vertices.append(tuple(map(float,values[1:])))
        if values[0]=='f':faces.append([vertices[int(item.split('/')[0])-1] for item in values[1:]])
    colors=[]
    for a,b,c in faces:
        u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
        n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]];length=math.sqrt(sum(k*k for k in n))
        light=.35+.6*max(0,sum(n[i]*[.2,-.6,.77][i] for i in range(3))/max(length,1e-8))
        colors.append((light*.83,light*.77,light*.63,1))
    ax=fig.add_subplot(1,3,slot,projection='3d');ax.set_facecolor(BG)
    ax.add_collection3d(Poly3DCollection(faces,facecolors=colors,linewidths=0))
    ax.set_xlim(-45,120);ax.set_ylim(-50,50);ax.set_zlim(-25,70);ax.set_box_aspect((165,100,95));ax.view_init(20,-62);ax.axis('off');ax.set_title(title,fontsize=17,y=-.05)
fig.suptitle('ORIGINAL WEAPON GEOMETRY',fontsize=23,color=GOLD,x=.055,ha='left')
fig.text(.055,.04,'Source OBJ preview • neutral lighting • not an Unreal gameplay screenshot',color='#a4bac4',fontsize=12)
fig.savefig(ROOT/'Docs/WEAPON_MODELS.png',dpi=130,facecolor=BG);plt.close(fig)
print('Created floorplan and weapon source previews.')
