"""
VBF (Virtuos Big File) extractor for FFX/FFX-2 HD Remaster.
Extracts files matching a path filter from .vbf archives.

VBF format:
  - Header: magic (4 bytes), header_length (4 bytes), num_files (8 bytes)
  - Per-file: md5_hash (16), block_list_start (4), unknown (4),
              original_size (8), start_offset (8), filename_offset (8)
  - String table: 4-byte length prefix, then concatenated null-terminated paths
  - File data: split into 64KB chunks, each optionally DEFLATE compressed
  - Block list: array of uint16 compressed sizes (0 = uncompressed full 64KB)

Usage:
  python vbf_extract.py <vbf_path> <output_dir> [--filter flash]
  python vbf_extract.py "D:\\Games\\...\\FFX_Data.vbf" extracted_ffx --filter flash
"""

import struct
import zlib
import os
import sys

CHUNK_SIZE = 65536  # 64KB per chunk


def read_vbf_header(f):
    """Read VBF header and return (num_files, header_length)."""
    magic = f.read(4)
    # Known magic values: VBFK (0x4B464256) or SRYK (0x5352594B)
    if magic not in (b'VBFK', b'KFBV', b'SRYK', b'KYRS'):
        # Try as little-endian
        magic_int = struct.unpack('<I', magic)[0]
        print(f"Warning: Unknown magic 0x{magic_int:08X} ({magic!r}), attempting anyway...")

    header_length = struct.unpack('<I', f.read(4))[0]
    num_files = struct.unpack('<Q', f.read(8))[0]

    print(f"Header length: {header_length}")
    print(f"Number of files: {num_files}")

    return num_files, header_length


def read_file_entries(f, num_files):
    """Read per-file metadata entries."""
    entries = []
    # Each entry: 16 (md5) + 4 (block_list_start) + 4 (unknown) +
    #             8 (original_size) + 8 (start_offset) + 8 (filename_offset) = 48 bytes
    for i in range(num_files):
        md5_hash = f.read(16)
        block_list_start = struct.unpack('<I', f.read(4))[0]
        unknown = struct.unpack('<I', f.read(4))[0]
        original_size = struct.unpack('<Q', f.read(8))[0]
        start_offset = struct.unpack('<Q', f.read(8))[0]
        filename_offset = struct.unpack('<Q', f.read(8))[0]

        entries.append({
            'md5': md5_hash,
            'block_list_start': block_list_start,
            'unknown': unknown,
            'original_size': original_size,
            'start_offset': start_offset,
            'filename_offset': filename_offset,
        })

    return entries


def read_string_table(f, entries):
    """Read string table and assign filenames to entries."""
    # String table starts with 4-byte length
    str_table_pos = f.tell()
    str_table_length = struct.unpack('<I', f.read(4))[0]
    str_table_data = f.read(str_table_length)

    for entry in entries:
        # filename_offset is relative to the start of the string table data
        offset = entry['filename_offset']
        if offset < len(str_table_data):
            # Read null-terminated string
            end = str_table_data.index(b'\x00', offset) if b'\x00' in str_table_data[offset:] else len(str_table_data)
            entry['filename'] = str_table_data[offset:end].decode('utf-8', errors='replace')
        else:
            entry['filename'] = f"unknown_{offset:08x}"

    return str_table_pos + 4 + str_table_length


def read_block_list(f, block_list_pos):
    """Read block list (array of uint16 compressed sizes).
    Returns the full block list array."""
    f.seek(block_list_pos)
    # Read a reasonable amount — block list is bounded by the data section
    # We'll read individual entries as needed
    return block_list_pos


def extract_file(f, entry, block_list_base, output_path):
    """Extract a single file from the VBF archive."""
    original_size = entry['original_size']
    if original_size == 0:
        # Empty file
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        with open(output_path, 'wb') as out:
            pass
        return True

    num_chunks = (original_size + CHUNK_SIZE - 1) // CHUNK_SIZE

    # Read block sizes for this file
    # Try both interpretations: block_list_start as element index (*2) or byte offset
    block_offset = block_list_base + entry['block_list_start'] * 2
    f.seek(block_offset)
    # Read extra to diagnose
    diag_raw = f.read(max(num_chunks * 4, 32))

    # Interpret as uint16
    block_sizes_u16 = list(struct.unpack(f'<{min(num_chunks, len(diag_raw)//2)}H', diag_raw[:num_chunks*2]))
    # Interpret as uint32
    n32 = min(num_chunks, len(diag_raw)//4)
    block_sizes_u32 = list(struct.unpack(f'<{n32}I', diag_raw[:n32*4])) if n32 > 0 else []

    print(f"  entry: block_list_start={entry['block_list_start']}, offset=0x{block_offset:X}, orig_size={original_size}, chunks={num_chunks}")
    print(f"  u16: {block_sizes_u16[:8]}{'...' if len(block_sizes_u16) > 8 else ''}")
    print(f"  u32: {block_sizes_u32[:8]}{'...' if len(block_sizes_u32) > 8 else ''}")
    print(f"  raw hex: {diag_raw[:32].hex()}")

    # Simple approach: read original_size bytes straight from start_offset.
    # The VBF may store SWF files without chunk compression since SWFs are
    # already internally compressed (CWS format = zlib-compressed SWF).
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    f.seek(entry['start_offset'])

    with open(output_path, 'wb') as out:
        out.write(f.read(original_size))

    return True


def main():
    if len(sys.argv) < 3:
        print("Usage: python vbf_extract.py <vbf_path> <output_dir> [--filter <keyword>]")
        print("Example: python vbf_extract.py FFX_Data.vbf extracted --filter flash")
        sys.exit(1)

    vbf_path = sys.argv[1]
    output_dir = sys.argv[2]

    path_filter = None
    if '--filter' in sys.argv:
        filter_idx = sys.argv.index('--filter')
        if filter_idx + 1 < len(sys.argv):
            path_filter = sys.argv[filter_idx + 1].lower()

    if not os.path.exists(vbf_path):
        print(f"Error: VBF file not found: {vbf_path}")
        sys.exit(1)

    print(f"Opening: {vbf_path}")
    print(f"Output:  {output_dir}")
    if path_filter:
        print(f"Filter:  *{path_filter}*")
    print()

    with open(vbf_path, 'rb') as f:
        # Step 1: Read header
        num_files, header_length = read_vbf_header(f)

        # Step 2: Read file entries
        print(f"Reading {num_files} file entries...")
        entries = read_file_entries(f, num_files)

        # Step 3: Read string table
        print("Reading string table...")
        block_list_base = read_string_table(f, entries)
        print(f"Block list base: 0x{block_list_base:X}")

        # Step 4: List/filter files
        if path_filter:
            filtered = [e for e in entries if path_filter in e['filename'].lower()]
            print(f"\nMatched {len(filtered)} files (filter: {path_filter}):")
        else:
            filtered = entries
            print(f"\nExtracting all {len(filtered)} files:")

        for e in filtered:
            print(f"  {e['filename']} ({e['original_size']} bytes)")

        if not filtered:
            print("No files matched the filter.")
            # List some filenames to help debug
            print("\nFirst 20 filenames in archive:")
            for e in entries[:20]:
                print(f"  {e['filename']}")
            sys.exit(0)

        # Step 5: Extract
        print(f"\nExtracting {len(filtered)} files...")
        success = 0
        failed = 0

        for i, entry in enumerate(filtered):
            filename = entry['filename']
            # Normalize path separators
            rel_path = filename.replace('/', os.sep).replace('\\', os.sep)
            # Remove leading separator if present
            if rel_path.startswith(os.sep):
                rel_path = rel_path[1:]
            output_path = os.path.join(output_dir, rel_path)

            if extract_file(f, entry, block_list_base, output_path):
                success += 1
                print(f"  [{i+1}/{len(filtered)}] OK: {filename}")
            else:
                failed += 1
                print(f"  [{i+1}/{len(filtered)}] FAILED: {filename}")

        print(f"\nDone. Extracted: {success}, Failed: {failed}")
        print(f"Output: {output_dir}")


if __name__ == '__main__':
    main()
