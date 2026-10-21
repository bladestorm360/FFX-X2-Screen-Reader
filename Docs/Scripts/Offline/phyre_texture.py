"""Read-only DX11 Phyre texture decoder; no guessed image dimensions/offsets.

Independent implementation of the self-describing namespace/instance format.
Format reference: ffgriever, dds-phyre-tool (PhyrePlatformDX11/PhyrePlatform):
https://gitlab.com/ffgriever/dds-phyre-tool
Only DDS reconstruction is supported. No game assets are modified.
"""
import struct


def texture_dds(data):
    def words(offset, count):
        if offset < 0 or offset + count * 4 > len(data):
            raise ValueError("Truncated Phyre structure")
        return struct.unpack_from(f"<{count}I", data, offset)

    h = words(0, 21)
    if h[0] != 0x50485952 or h[1] != 84 or h[3] != 0x44583131:
        raise ValueError("Unsupported Phyre texture platform")
    n = words(84, 8)
    if n[1] != h[2]:
        raise ValueError("Inconsistent Phyre namespace size")
    strings = 84 + n[1] - n[5] - n[6] * n[7]

    def name(offset):
        if not 0 <= offset < n[5]:
            raise ValueError("Invalid namespace string offset")
        start = strings + offset
        end = data.index(b"\0", start, strings + n[5])
        return data[start:end].decode("ascii")

    types = words(116, n[2])
    class_start = 116 + n[2] * 4
    classes = [words(class_start + i * 36, 9) for i in range(n[3])]
    member_start = class_start + n[3] * 36
    members = {}
    member_id = 0
    for c in classes:
        fields = {}
        for i in range(c[3]):
            m = words(member_start + (member_id + i) * 24, 6)
            fields[name(m[0])] = m[2]
        members[name(c[2])] = fields
        member_id += c[3]
    if member_id != n[4]:
        raise ValueError("Invalid member count")
    instances = 84 + n[1]
    texture_start = None
    relative = 0
    for i in range(h[4]):
        instance = words(instances + i * 36, 9)
        if not 1 <= instance[0] <= len(classes):
            raise ValueError("Invalid instance class")
        if name(classes[instance[0] - 1][2]) == "PTexture2D":
            texture_start = instances + h[4] * 36 + relative
        relative += instance[2]
    if texture_start is None:
        raise ValueError("No PTexture2D instance")
    width = words(texture_start + members["PTexture2DBase"]["m_width"], 1)[0]
    height = words(texture_start + members["PTexture2DBase"]["m_height"], 1)[0]
    mips = words(texture_start + members["PTextureCommonBase"]["m_mipmapCount"], 1)[0] + 1
    fixup_data = instances + h[4] * 36 + h[14]
    fixup_start = fixup_data + h[13]
    fmt_type, fmt_size, fmt_offset = words(fixup_start + 12, 3)
    if fmt_type >= len(types) or name(types[fmt_type]) != "PTextureFormatBase" or fmt_offset + fmt_size > h[13]:
        raise ValueError("Invalid texture format fixup")
    fmt = data[fixup_data + fmt_offset:fixup_data + fmt_offset + fmt_size].rstrip(b"\0").decode("ascii")
    payload_start = fixup_start + h[12] * 12 + h[5] + h[7] + h[9]
    if not 1 <= width <= 16384 or not 1 <= height <= 16384 or not 1 <= mips <= 15:
        raise ValueError("Invalid texture dimensions/mip count")
    fourcc = {"DXT1": b"DXT1", "DXT3": b"DXT3", "DXT5": b"DXT5", "BC5": b"ATI2"}
    if fmt not in fourcc and fmt != "ARGB8":
        raise ValueError(f"Unsupported texture format {fmt}")
    block = 8 if fmt == "DXT1" else 16
    size = sum(max(1, width >> i) * max(1, height >> i) * 4 if fmt == "ARGB8" else
               max(1, (max(1, width >> i) + 3) // 4) * max(1, (max(1, height >> i) + 3) // 4) * block for i in range(mips))
    if payload_start + size != len(data):
        raise ValueError("Texture payload size does not match metadata")
    header = [124, (0x100f if fmt == "ARGB8" else 0x81007) | (0x20000 if mips > 1 else 0), height, width,
              width * 4 if fmt == "ARGB8" else max(1, (width + 3) // 4) * max(1, (height + 3) // 4) * block, 0, mips]
    header += [0] * 11
    header += ([32, 0x41, 0, 32, 0xff0000, 0xff00, 0xff, 0xff000000] if fmt == "ARGB8" else
               [32, 4, int.from_bytes(fourcc[fmt], "little"), 0, 0, 0, 0, 0])
    header += [0x1000 | (0x400008 if mips > 1 else 0), 0, 0, 0, 0]
    return b"DDS " + struct.pack("<31I", *header) + data[payload_start:], dict(width=width, height=height, format=fmt, mipmaps=mips)
