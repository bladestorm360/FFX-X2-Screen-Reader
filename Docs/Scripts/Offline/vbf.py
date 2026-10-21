"""Read-only VBF reader for offline FFX data extraction.

Format cross-checked against the launcher reader published by Topher:
https://github.com/topher-au/VBFTool/blob/master/VBFTool/VirtuosBigFileReader.cs
Independent implementation; never writes to an archive or game directory.
"""
from dataclasses import dataclass
import hashlib
from pathlib import Path, PurePosixPath
import struct
import zlib

BLOCK_SIZE = 65536


@dataclass(frozen=True)
class Entry:
    name: str
    size: int
    offset: int
    block_start: int


def read_exact(stream, size):
    data = stream.read(size)
    if len(data) != size:
        raise ValueError("Truncated VBF")
    return data


class Vbf:
    def __init__(self, path):
        self.path = Path(path)
        self.entries = []
        with self.path.open("rb") as stream:
            magic, header_size, count = struct.unpack("<4sIQ", read_exact(stream, 16))
            if magic != b"SRYK" or not 0 < count <= 1000000:
                raise ValueError("Unsupported VBF header")
            if not 16 + count * 48 + 4 <= header_size <= self.path.stat().st_size - 16:
                raise ValueError("Invalid VBF header size")
            # Hashes form one array; metadata forms a separate array.
            hashes = [read_exact(stream, 16) for _ in range(count)]
            rows = [struct.unpack("<IIQQQ", read_exact(stream, 32)) for _ in range(count)]
            string_size = struct.unpack("<I", read_exact(stream, 4))[0]
            if not 4 <= string_size <= header_size - stream.tell() + 4:
                raise ValueError("Invalid VBF string table size")
            # The size INCLUDES its four-byte size prefix.
            strings = read_exact(stream, string_size - 4)
            block_count = sum((row[2] + BLOCK_SIZE - 1) // BLOCK_SIZE for row in rows)
            if stream.tell() + block_count * 2 > header_size:
                raise ValueError("Invalid VBF block table")
            self.blocks = struct.unpack(f"<{block_count}H", read_exact(stream, block_count * 2))
            for digest, (start, _, size, offset, name_offset) in zip(hashes, rows):
                if name_offset >= len(strings):
                    raise ValueError("Invalid VBF filename offset")
                end = strings.index(b"\0", name_offset)
                name = strings[name_offset:end].decode("utf-8")
                if hashlib.md5(name.lower().encode("utf-8")).digest() != digest:
                    raise ValueError(f"Filename/hash mismatch: {name}")
                chunks = (size + BLOCK_SIZE - 1) // BLOCK_SIZE
                if start + chunks > block_count or (size and offset < header_size):
                    raise ValueError(f"Invalid VBF file bounds: {name}")
                stored_size = sum(x or BLOCK_SIZE for x in self.blocks[start:start + chunks])
                if size and offset + stored_size > self.path.stat().st_size - 16:
                    raise ValueError(f"VBF file extends past archive: {name}")
                self.entries.append(Entry(name, size, offset, start))
            stream.seek(0)
            header = read_exact(stream, header_size)
            stream.seek(-16, 2)
            if hashlib.md5(header).digest() != read_exact(stream, 16):
                raise ValueError("VBF header checksum mismatch")

    def read(self, entry):
        if entry.size == 0:
            return b""
        output = bytearray()
        with self.path.open("rb") as stream:
            stream.seek(entry.offset)
            for i in range((entry.size + BLOCK_SIZE - 1) // BLOCK_SIZE):
                expected = min(BLOCK_SIZE, entry.size - len(output))
                stored = self.blocks[entry.block_start + i] or BLOCK_SIZE
                block = read_exact(stream, stored)
                if stored == BLOCK_SIZE or stored == expected:
                    decoded = block
                else:
                    decoded = zlib.decompress(block)
                if len(decoded) != expected:
                    raise ValueError(f"Wrong decoded block length: {entry.name}")
                output.extend(decoded)
        if len(output) != entry.size:
            raise ValueError(f"Wrong extracted length: {entry.name}")
        return bytes(output)


def output_path(root, name):
    """Reject archive paths that escape the caller's output directory."""
    path = PurePosixPath(name.replace("\\", "/").lstrip("/"))
    if not path.parts or any(p in (".", "..") or ":" in p for p in path.parts):
        raise ValueError(f"Unsafe archive path: {name}")
    root = Path(root).resolve()
    result = root.joinpath(*path.parts).resolve()
    if not result.is_relative_to(root):
        raise ValueError(f"Archive path outside output root: {name}")
    return result
