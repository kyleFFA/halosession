"""Measure cyan inner-edge pixels, excluding the cloud silhouette. Analysis only."""
from pathlib import Path
import struct,zlib,math,json,hashlib,sys
root=Path(__file__).resolve().parents[1]
path=Path(sys.argv[1]) if len(sys.argv)>1 else root/'HCMInternal/Assets/session-ringworld-round.png'
data=path.read_bytes();offset=8;compressed=bytearray()
while offset<len(data):
 size=int.from_bytes(data[offset:offset+4],'big');kind=data[offset+4:offset+8];payload=data[offset+8:offset+8+size]
 if kind==b'IHDR':
  width,height,depth,color,_,_,interlace=struct.unpack('>IIBBBBB',payload);assert depth==8 and color==6 and interlace==0
 if kind==b'IDAT':compressed.extend(payload)
 offset+=size+12
raw=zlib.decompress(compressed);stride=width*4;previous=bytearray(stride);rows=[]
for y in range(height):
 start=y*(stride+1);f=raw[start];row=bytearray(raw[start+1:start+1+stride])
 for x in range(stride):
  a=row[x-4] if x>=4 else 0;b=previous[x];c=previous[x-4] if x>=4 else 0
  if f==1:row[x]=(row[x]+a)&255
  elif f==2:row[x]=(row[x]+b)&255
  elif f==3:row[x]=(row[x]+((a+b)//2))&255
  elif f==4:
   p=a+b-c;dist=(abs(p-a),abs(p-b),abs(p-c));row[x]=(row[x]+(a,b,c)[dist.index(min(dist))])&255
  else:assert f==0
 rows.append(row);previous=row
def solve(matrix,values):
 a=[list(row)+[value] for row,value in zip(matrix,values)]
 for i in range(3):
  pivot=max(range(i,3),key=lambda j:abs(a[j][i]));a[i],a[pivot]=a[pivot],a[i]
  divisor=a[i][i];a[i]=[v/divisor for v in a[i]]
  for j in range(3):
   if j!=i:
    factor=a[j][i];a[j]=[x-factor*y for x,y in zip(a[j],a[i])]
 return [row[3] for row in a]
def fit(points):
 sx=sy=sxx=syy=sxy=sz=sxz=syz=0.
 for x,y in points:
  z=-(x*x+y*y);sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;sz+=z;sxz+=x*z;syz+=y*z
 a,b,c=solve([[sxx,sxy,sx],[sxy,syy,sy],[sx,sy,len(points)]],[sxz,syz,sz])
 return -a/2,-b/2,math.sqrt((a*a+b*b)/4-c)
points=[]
for degree in range(360):
 angle=degree*math.pi/180;best=None
 for radius in range(int(width*.265),int(width*.365)):
  x=int(width*.5+math.cos(angle)*radius);y=int(height*.48+math.sin(angle)*radius)
  red,green,blue,alpha=rows[y][x*4:x*4+4]
  if alpha<180 or green<145 or blue<170:continue
  score=(blue-red)+.8*(green-red)-.2*abs(blue-green)
  if best is None or score>best[0]:best=(score,x,y)
 if best and best[0]>80:points.append((best[1],best[2]))
raw_count=len(points)
for _ in range(4):
 cx,cy,radius=fit(points)
 residuals=sorted(abs(math.hypot(x-cx,y-cy)-radius) for x,y in points)
 threshold=max(width*.006,residuals[len(residuals)//2]*3)
 points=[(x,y) for x,y in points if abs(math.hypot(x-cx,y-cy)-radius)<=threshold]
cx,cy,radius=fit(points);distances=sorted(math.hypot(x-cx,y-cy) for x,y in points)
result={'asset':path.name,'sha256':hashlib.sha256(data).hexdigest().upper(),'size':[width,height],
 'method':'Cyan inner-edge saturation maxima along radial probes; robust least-squares circle; no cloud alpha bounds',
 'raw_samples':raw_count,'retained_samples':len(points),'center_pixels':[cx,cy],'center_normalized':[cx/width,cy/height],
 'radius_pixels':radius,'radius_normalized':radius/width,'radius_p05_normalized':distances[len(distances)*5//100]/width,'radius_p95_normalized':distances[len(distances)*95//100]/width,
 'inner_bounds_normalized':[(cx-radius)/width,(cy-radius)/height,(cx+radius)/width,(cy+radius)/height]}
(Path(sys.argv[2]) if len(sys.argv)>2 else root/'out/ring-opening-measurement.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
