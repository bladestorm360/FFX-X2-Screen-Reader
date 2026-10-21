"""Synthetic VBF format regressions; no game access."""
import hashlib
import random
from pathlib import Path
import struct
import tempfile
import unittest
import zlib
from vbf import Vbf, output_path, BLOCK_SIZE


def fixture(path):
    files = [("event/raw.ebp", random.Random(7).randbytes(BLOCK_SIZE) + b"last block"),
             ("event/compressed.ebp", b"A" * 70000), ("event/empty.ebp", b"")]
    names, rows, blocks, payload = bytearray(), [], [], bytearray()
    for name, content in files:
        start, offset, name_offset = len(blocks), len(payload), len(names)
        names.extend(name.encode() + b"\0")
        for i in range(0, len(content), BLOCK_SIZE):
            raw = content[i:i + BLOCK_SIZE]
            compressed = zlib.compress(raw)
            stored = compressed if len(compressed) < len(raw) else raw
            blocks.append(len(stored) % BLOCK_SIZE)
            payload.extend(stored)
        rows.append((start, 0, len(content), offset, name_offset))
    header_size = 16 + len(files) * 48 + 4 + len(names) + len(blocks) * 2
    header = bytearray(struct.pack("<4sIQ", b"SRYK", header_size, len(files)))
    for name, _ in files:
        header.extend(hashlib.md5(name.lower().encode()).digest())
    for start, unused, size, offset, name_offset in rows:
        # Empty launcher entries can carry unused, out-of-archive offsets.
        header.extend(struct.pack("<IIQQQ", start, unused, size, offset + header_size if size else 2**48, name_offset))
    header.extend(struct.pack("<I", len(names) + 4) + names)
    header.extend(struct.pack(f"<{len(blocks)}H", *blocks))
    path.write_bytes(header + payload + hashlib.md5(header).digest())
    return files


class ReaderTests(unittest.TestCase):
    def test_compressed_raw_partial_and_empty(self):
        with tempfile.TemporaryDirectory() as task_dir:
            path = Path(task_dir) / "test.vbf"
            expected = fixture(path)
            reader = Vbf(path)
            self.assertEqual([(e.name, reader.read(e)) for e in reader.entries], expected)

    def test_corrupt_checksum_rejected(self):
        with tempfile.TemporaryDirectory() as task_dir:
            path = Path(task_dir) / "test.vbf"
            fixture(path)
            data = bytearray(path.read_bytes())
            data[-1] ^= 1
            path.write_bytes(data)
            with self.assertRaisesRegex(ValueError, "checksum"):
                Vbf(path)

    def test_path_traversal_rejected(self):
        with tempfile.TemporaryDirectory() as task_dir:
            for name in ("../outside", "C:/outside", "event/../../outside"):
                with self.assertRaises(ValueError):
                    output_path(task_dir, name)


if __name__ == "__main__":
    unittest.main()
