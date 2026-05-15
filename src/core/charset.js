// FFX text encoding: decoders for the custom charset used in excel bins,
// dialog buffers, and character names. All memory reads flow through
// safeReadByteArray — unmapped-page reads return null rather than
// throwing into a Frida callback.

import {
    playerChrsAddr, CHR_SIZE, CHR_RAM_OFFSET, CHARACTER_NAMES
} from './addresses.js';
import { isValidPtr, safeReadByteArray } from './safeRead.js';
import { rva } from './memory.js';

export const CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~';

// Byte-length classification for FFX charset opcodes, derived from
// FUN_008DFD70 (the game's own "advance to next char" function at RVA
// 0x4DFD70). Each entry says how many bytes that opcode consumes.
//
// Without this, decodeFFXSegments was treating every byte < 0x2B as a
// 1-byte skip, which let 2-byte opcodes' data byte fall through and get
// decoded as a regular charset char — producing spurious letters like
// "Y" (from 0x13 + 0x68) and "b" (from 0x07 + 0x71) inside popup text.
//
//   0x00         — end of segment (no advance, just close out)
//   0x01, 0x02   — 1-byte
//   0x03         — separator + 1-byte advance
//   0x05         — 1-byte
//   0x04         — 3-byte if next < 0x30, else 2-byte
//   0x06-0x0E    — 2-byte
//   0x0F, 0x11   — 1-byte (fall through default in FUN_008DFD70)
//   0x10         — 2-byte
//   0x12-0x2A    — 2-byte
//   0x2B-0x2F    — multi-byte char prefix (existing logic handles these)
//   0x30+        — regular charset char (existing logic handles these)

const TWO_BYTE_OPCODES = new Set([
    0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x10,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
    0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2A
]);

export function decodeFFXSegments(addr, maxLen, maxOps) {
    const segments = [];
    if (!isValidPtr(addr) || maxLen <= 0 || maxLen > 0x10000) return segments;
    const raw = safeReadByteArray(addr, maxLen);
    if (!raw) return segments;
    let current = '';
    try {
        const bytes = new Uint8Array(raw);
        let i = 0;
        let opCount = 0;
        while (i < maxLen) {
            if (maxOps > 0 && opCount >= maxOps) break;
            let b = bytes[i];
            // Segment terminators / separators.
            if (b === 0x00) { if (current.length > 0) segments.push(current.trim()); current = ''; i++; opCount++; continue; }
            if (b === 0x03) { if (current.length > 0) segments.push(current.trim()); current = ''; i++; opCount++; continue; }
            // 0x0A is a 2-byte opcode in FUN_008DFD70 — line break + 1 data byte.
            if (b === 0x0A) { if (current.length > 0) segments.push(current.trim()); current = ''; i += 2; opCount++; continue; }
            // 0x04 — 3-byte if next < 0x30, else 2-byte.
            if (b === 0x04) {
                if (i + 1 < maxLen && bytes[i + 1] < 0x30) i += 3; else i += 2;
                opCount++;
                continue;
            }
            // All known 2-byte control opcodes (color/style/icon/char-ref/etc.).
            // Dropping their data byte instead of decoding it kills spurious
            // letters like "Y" / "b" leaking into popup text.
            if (TWO_BYTE_OPCODES.has(b)) { i += 2; opCount++; continue; }
            // Remaining < 0x30 bytes (0x01, 0x02, 0x05, 0x0F, 0x11) are 1-byte
            // controls per FUN_008DFD70.
            if (b < 0x2B) { i++; opCount++; continue; }
            // 0x2B-0x2F: multi-byte charset prefix. 0x30+: direct charset index.
            let idx = 0;
            if (b < 0x30) { idx = (b - 0x2B) * 0xD0; i++; if (i >= maxLen) break; b = bytes[i]; }
            idx += (b - 0x30);
            if (idx >= 0 && idx < CHARSET.length) current += CHARSET[idx];
            i++;
            opCount++;
        }
        if (current.length > 0) segments.push(current.trim());
    } catch (e) {}
    return segments.filter(function(s) { return s.length > 0 && s.replace(/\s/g, '').length > 0; });
}

export function decodeFFXString(addr, maxLen) {
    if (!isValidPtr(addr) || maxLen <= 0 || maxLen > 0x10000) return '';
    const raw = safeReadByteArray(addr, maxLen);
    if (!raw) return '';
    let result = '';
    try {
        const bytes = new Uint8Array(raw);
        let i = 0;
        while (i < maxLen) {
            let b = bytes[i];
            if (b === 0x00) break;
            if (b === 0x03 || b === 0x0A) { result += ' '; i++; continue; }
            if (b === 0x13 && i + 1 < maxLen) {
                const chrIdx = bytes[i + 1] - 0x30;
                try {
                    const pArr = playerChrsAddr.readPointer();
                    if (isValidPtr(pArr) && chrIdx >= 0 && chrIdx < 0x1f) {
                        const ns = decodeFFXSegments(pArr.add(chrIdx * CHR_SIZE + CHR_RAM_OFFSET), 40, 0);
                        if (ns.length > 0) result += ns[0];
                    }
                } catch (e2) {
                    if (chrIdx < CHARACTER_NAMES.length) result += CHARACTER_NAMES[chrIdx];
                }
                i += 2; continue;
            }
            if (b === 0x09 || b === 0x10) { i += 2; continue; }
            if (b < 0x2B) { i++; continue; }
            let idx = 0;
            if (b < 0x30) { idx = (b - 0x2B) * 0xD0; i++; if (i >= maxLen) break; b = bytes[i]; }
            idx += (b - 0x30);
            if (idx >= 0 && idx < CHARSET.length) result += CHARSET[idx];
            i++;
        }
    } catch (e) {}
    return result.trim();
}

// ── Native text-resolution helpers: call the game's own text functions ──

const msMenuGetTextFn = new NativeFunction(rva(0x38FD40), 'pointer', ['int', 'int', 'int']);
const getConfigLangFn = new NativeFunction(rva(0x3852B0), 'int', []);
const locStringCopyFn = new NativeFunction(rva(0x241480), 'void', ['int', 'pointer']);

// Upper bound for the excel-bin `dataStart` offset. Real bins are well under
// 1 MB; anything larger almost certainly means the bin pointer is stale or
// uninitialized — reject rather than chase a wild offset.
const EXCEL_MAX_DATA_START = 0x100000;

export function resolveCommandText(commandId, excelBinAddr) {
    try {
        if (!isValidPtr(excelBinAddr)) return null;
        const excelPtr = excelBinAddr.readPointer();
        if (!isValidPtr(excelPtr)) return null;
        const hdr = excelPtr.add(0x8);
        const firstIdx = hdr.readU16(), lastIdx = hdr.add(2).readU16();
        const elementSize = hdr.add(4).readU16(), dataLength = hdr.add(6).readU16();
        const dataStart = hdr.add(8).readU32();
        if (dataStart === 0 || dataStart > EXCEL_MAX_DATA_START) return null;
        if (elementSize === 0 || elementSize > 0x1000) return null;
        const excelIdx = commandId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;
        const elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        const nameOffset = elementAddr.readU16();
        const textAddr = excelPtr.add(dataStart + dataLength + nameOffset);
        const segments = decodeFFXSegments(textAddr, 64, 0);
        return (segments.length > 0) ? segments[0] : null;
    } catch (e) { return null; }
}

export function resolveLocString(textId) {
    try {
        const buf = Memory.alloc(256);
        locStringCopyFn(textId, buf);
        const segs = decodeFFXSegments(buf, 256, 0);
        return (segs.length > 0) ? segs[0] : null;
    } catch (e) { return null; }
}

export function resolveConfigText(textId) {
    try {
        const lang = getConfigLangFn();
        const textPtr = msMenuGetTextFn(7, textId, lang);
        if (!isValidPtr(textPtr)) return null;
        const segs = decodeFFXSegments(textPtr, 256, 0);
        return (segs.length > 0) ? segs[0] : null;
    } catch (e) { return null; }
}
