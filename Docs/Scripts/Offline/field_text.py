"""Offline English text evidence. Preserve unresolved controls as tokens.

EBP and dictionary layouts: Karifean/FFXDataParser model/strings.
Charset comes from the extracted game's ffxsjistbl_us.bin, not ASCII guesses.
This decoder does not replace the native production text getter.
"""
from pathlib import Path
import struct


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


class TextEvidence:
    def __init__(self, directory):
        directory = Path(directory)
        self.charset = (directory / "ffxsjistbl_us.bin").read_text(encoding="utf-8")
        data = (directory / "macrodic.dcp").read_bytes()
        offsets = [u32(data, i * 4) for i in range(16)]
        self.tables = []
        for i, start in enumerate(offsets):
            end = next((n for n in offsets[i + 1:] if n > start), len(data))
            chunk = data[start:end] if start and start < end else b""
            self.tables.append(chunk)

    def decode(self, data, offset, depth=0):
        output, extra = [], 0
        while 0 <= offset < len(data):
            b = data[offset]
            offset += 1
            if b == 0:
                break
            if b == 4:
                extra = 0x410
                continue
            if b >= 0x2B:
                idx = b
                if b < 0x30:
                    if offset >= len(data):
                        break
                    idx = (b - 0x2B) * 0xD0 + data[offset]
                    offset += 1
                idx += extra - 0x30
                extra = 0
                output.append(self.charset[idx] if 0 <= idx < len(self.charset) else f"<char:{idx}>")
            elif b == 3:
                output.append("\n")
            elif b in (1, 2, 5, 0xF, 0x11):
                output.append(f"<control:{b:02x}>")
            else:
                if offset >= len(data):
                    break
                arg = data[offset]
                offset += 1
                if 0x13 <= b <= 0x22 and not (b == 0x13 and arg <= 0x43) and depth < 3:
                    output.append(self.macro(b - 0x13, arg - 0x30, depth + 1))
                elif b == 0x10:
                    output.append("<choice-end>" if arg == 255 else f"<choice:{arg - 0x30}>")
                elif b not in (7, 0xA):
                    output.append(f"<control:{b:02x}:{arg:02x}>")
        return "".join(output)

    def macro(self, group, index, depth=0):
        chunk = self.tables[group] if 0 <= group < len(self.tables) else b""
        if not chunk or not 0 <= index < u16(chunk, 0) // 4:
            return f"<macro:{group}:{index}>"
        return self.decode(chunk, u16(chunk, index * 4), depth)

    def field_strings(self, path):
        data = Path(path).read_bytes()
        start, end = u32(data, 20), u32(data, 24)
        if not start or not start < end <= len(data):
            return []
        chunk = data[start:end]
        count = u16(chunk, 0) // 8
        return [self.decode(chunk, u32(chunk, i * 8) & 0xFFFF) for i in range(count)]
