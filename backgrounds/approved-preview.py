# Portable copy of the original eight-second study. Production clouds use the approved quarter-speed 32-second loop; stars retain eight seconds.
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math, random

out=Path(__file__).parent
art=Path(__file__).parent
clouds=[Image.open(art/n).convert('RGB').resize((420,420)) for n in ['bright-sky.png','pastel-clouds.png']]
font=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',18)
mask=Image.new('L',(320,320)); ImageDraw.Draw(mask).ellipse((2,2,318,318),fill=255)
def stars(seed,count):
 r=random.Random(seed)
 return [(r.randrange(320),r.randrange(320),r.choice([(125,185,255),(180,235,255),(206,180,255),(175,235,180),(255,230,165)]),r.random()*math.tau,r.choice([1,2,3]),i< count//6) for i in range(count)]
fields=[stars(1999,160),stars(2000,430)]
labels=['Bright sky','Pastel clouds','Quiet twinkle','Retro sparkle']
frames=[]
individual=[[] for _ in range(4)]
for frame in range(96):
 t=math.tau*frame/96
 sheet=Image.new('RGB',(720,760),(19,29,37)); d=ImageDraw.Draw(sheet)
 for idx in range(4):
  if idx<2:
   x=int(50+22*math.sin(t)); y=int(50+12*math.cos(t))
   content=clouds[idx].crop((x,y,x+320,y+320))
  else:
   content=Image.new('RGB',(320,320),(0,1,7)); sd=ImageDraw.Draw(content)
   for x,y,c,phase,freq,large in fields[idx-2]:
    p=((math.sin(t*freq+phase)+1)/2)**(5 if idx==2 else 3)
    strength=(.2+.8*p) if idx==2 else (.42+.58*p)
    col=tuple(int(v*strength) for v in c)
    sd.point((x,y),fill=col)
    if large:
     ext=1+int((5 if idx==2 else 7)*p)
     sd.line((x-ext,y,x+ext,y),fill=col)
     sd.line((x,y-ext,x,y+ext),fill=col)
     if idx==3 and p>.7:
      sd.line((x-2,y-2,x+2,y+2),fill=col)
      sd.line((x-2,y+2,x+2,y-2),fill=col)
      sd.point((x,y),fill=(255,255,255))
  tile=Image.new('RGB',(320,320),(19,29,37));tile.paste(content,(0,0),mask)
  ImageDraw.Draw(tile).ellipse((1,1,319,319),outline=(95,132,151),width=2)
  individual[idx].append(tile)
  px=20+(idx%2)*360;py=14+(idx//2)*380
  sheet.paste(tile,(px,py));d.text((px+160,py+330),labels[idx],fill=(222,235,243),font=font,anchor='mt')
 frames.append(sheet)
duration=[80,80,90]*32
for name,seq in [('four-motion-options',frames)]+list(zip(['bright-sky','pastel-clouds','quiet-twinkle','retro-sparkle'],individual)):
 seq[0].save(out/(name+'.gif'),save_all=True,append_images=seq[1:],duration=duration,loop=0,optimize=False)
 with Image.open(out/(name+'.gif')) as check:
  total=0
  for j in range(check.n_frames):
   check.seek(j);total+=check.info['duration']
  assert total==8000
frames[24].save(out/'four-motion-options-still.png')
print('Verified five animated GIFs, each with an 8-second loop; Session untouched.')
