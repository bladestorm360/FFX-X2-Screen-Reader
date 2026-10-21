"""Render extracted original meshes for offline NPC/object visual review.

Actual vertices/UVs, flat texture-sampled faces and painter ordering. These
review images do not reproduce game animation or lighting. Pillow only.
"""
import argparse
import math
from pathlib import Path
from PIL import Image, ImageDraw
from phyre_mesh import mesh_sets, write_obj


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("assets",type=Path)
    parser.add_argument("--codes",default="f011,f016,m250,n080,n122,n126,n234")
    args=parser.parse_args()
    for code in args.codes.split(","):
        meshes=list(args.assets.rglob(code+".dae.phyre"))
        textures=list(args.assets.rglob(code+".dds.png"))
        if len(meshes)!=1 or len(textures)!=1:
            raise ValueError(f"Missing/ambiguous original asset {code}")
        sets=mesh_sets(meshes[0].read_bytes())
        write_obj(meshes[0].with_suffix(".obj"),sets)
        texture=Image.open(textures[0]).convert("RGB")
        polygons,colors=[],[]
        for s in sets:
            for face in s["faces"]:
                polygons.append([s["vertices"][i] for i in face])
                u=sum(s["uvs"][i][0] for i in face)/3%1
                v=sum(s["uvs"][i][1] for i in face)/3%1
                colors.append(texture.getpixel((int(u*texture.width)%texture.width,int(v*texture.height)%texture.height)))
        sheet=Image.new("RGB",(1200,440),"#eeeeee")
        draw=ImageDraw.Draw(sheet);draw.text((20,10),code+" - original extracted mesh",fill="black")
        for view,degrees in enumerate((45,135,225)):
            a=math.radians(degrees);e=math.radians(20)
            def project(p):
                x,y,z=p
                right=x*math.cos(a)+z*math.sin(a)
                depth=-x*math.sin(a)+z*math.cos(a)
                up=-y*math.cos(e)-depth*math.sin(e)
                return right,-up,depth*math.cos(e)+y*math.sin(e)
            triangles=[[project(p) for p in tri] for tri in polygons]
            points=[p for tri in triangles for p in tri]
            lo=[min(p[i] for p in points) for i in range(2)]
            hi=[max(p[i] for p in points) for i in range(2)]
            scale=min(360/max(hi[0]-lo[0],1e-3),360/max(hi[1]-lo[1],1e-3))
            center=[(lo[i]+hi[i])/2 for i in range(2)]
            for index in sorted(range(len(triangles)),key=lambda i:sum(p[2] for p in triangles[i]),reverse=True):
                tri=triangles[index]
                draw.polygon([(view*400+200+(p[0]-center[0])*scale,230+(p[1]-center[1])*scale) for p in tri],fill=colors[index])
        sheet.save(args.assets/(code+"_model.png"))
        print(code,len(polygons),"faces; review image saved")


if __name__=="__main__":main()
