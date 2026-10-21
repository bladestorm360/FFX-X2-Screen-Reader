"""Read-only field mesh/UV extraction for visual identity review.

Independent implementation; descriptor layouts cross-checked with the original
Xentax mesh reader archived by tinkerbeast/ffx-ai/thirdparty_xentax/phyre.py.
No arbitrary face/vertex offsets or manual model dimensions are supplied.
"""
import math
import struct


def mesh_sets(data):
    h = struct.unpack_from("<21I", data)
    if h[0] != 0x50485952 or h[3] != 0x44583131 or h[1] != 84:
        raise ValueError("Unsupported Phyre mesh platform")
    index_start = len(data) - h[18] - h[19] - h[20]
    vertex_start = index_start + h[18]
    if index_start < 84 + h[2] or h[20] != 0:
        raise ValueError("Invalid mesh buffer sizes")
    candidates = []
    for offset in range(84 + h[2], index_start - 108, 4):
        b = struct.unpack_from("<27I", data, offset)
        if b[0] == 0xffffffff and b[13] and b[13] % 3 == 0 and 3 <= b[12]+1 < 65536 and b[22] == 0 and b[24] == b[13]*2:
            candidates.append(offset)
    valid = []
    for candidate in candidates:
        sets = []
        ipos = index_start
        pos = candidate
        try:
            while pos + 108 <= index_start:
                b = struct.unpack_from("<27I", data, pos)
                if b[0] != 0xffffffff:
                    break
                count, nverts = b[13], b[12]+1
                if count % 3 or b[24] != count*2 or ipos+count*2 > vertex_start:
                    raise ValueError("Invalid face descriptor")
                faces = list(struct.iter_unpack("<3H", data[ipos:ipos+count*2]))
                if not faces or max(max(f) for f in faces) >= nverts:
                    raise ValueError("Face outside vertex range")
                sets.append(dict(faces=faces, count=nverts))
                ipos = index_start + ((ipos-index_start+count*2+3) & ~3)
                pos += 108
            if ipos != vertex_start:
                continue
            first = sets[0]["count"]
            marker = struct.pack("<2I", 12, first)
            headers = []
            p = 84+h[2]
            while True:
                p = data.find(marker, p, index_start)
                if p < 0:
                    break
                b = struct.unpack_from("<16I", data, p)
                if b[14] == first*12:
                    headers.append(p)
                p += 1
            for vheader in headers:
                vp, hp = vertex_start, vheader
                parsed = []
                for s in sets:
                    size = 0
                    attrs = []
                    while hp+64 <= index_start:
                        b = struct.unpack_from("<16I", data, hp)
                        if b[1] != s["count"]:
                            break
                        attrs.append(b[0])
                        size += b[0]*b[1]
                        hp += 64
                    if attrs[:3] != [12,12,8] or vp+size > len(data):
                        break
                    vertices = list(struct.iter_unpack("<3f",data[vp:vp+s["count"]*12]))
                    uv_start = vp+s["count"]*24
                    uvs = list(struct.iter_unpack("<2f",data[uv_start:uv_start+s["count"]*8]))
                    if not all(math.isfinite(x) and abs(x) < 1e6 for v in vertices for x in v):
                        break
                    parsed.append(dict(vertices=vertices, uvs=uvs, faces=s["faces"]))
                    vp += size
                if len(parsed) == len(sets) and vp == len(data):
                    valid.append(parsed)
        except (ValueError, struct.error):
            continue
    if len(valid) != 1:
        raise ValueError(f"Expected one validated mesh layout, found {len(valid)}")
    return valid[0]


def write_obj(path, sets):
    lines = ["# Extracted original Phyre mesh for offline visual review"]
    base = 1
    for i, s in enumerate(sets):
        lines.append(f"g mesh_{i}")
        lines += ["v %g %g %g" % v for v in s["vertices"]]
        lines += ["vt %g %g" % (uv[0],1-uv[1]) for uv in s["uvs"]]
        lines += ["f " + " ".join(f"{base+j}/{base+j}" for j in face) for face in s["faces"]]
        base += len(s["vertices"])
    path.write_text("\n".join(lines)+"\n", encoding="ascii")
