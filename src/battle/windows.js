// Battle window state readers + command name resolution.

import {
    btlWinArrayAddr, btlCommandBinAddr, btlItemBinAddr,
    BTL_WIN_SIZE, playerChrsAddr, CHR_SIZE, CHR_RAM_OFFSET, CHARACTER_NAMES
} from '../core/addresses.js';
import { decodeFFXSegments, decodeFFXString, resolveCommandText } from '../core/charset.js';
import { rva } from '../core/memory.js';
import { isValidPtr } from '../core/safeRead.js';
import { getItemQuantity } from './entities.js';

// The battle window array is a fixed-size in-engine pool; the specific size
// isn't documented here, but an idx beyond ~15 is never legitimate and
// almost always means the cur-win-idx register holds a stale/garbage value.
const BTL_WIN_MAX_IDX = 15;

export function readBtlWindow(idx) {
    if (idx > BTL_WIN_MAX_IDX) return null;
    return readBtlWindowFromPtr(btlWinArrayAddr.add(idx * BTL_WIN_SIZE));
}

// Same as readBtlWindow but takes the window pointer directly. Used by
// the cursor-change event hooks where the pointer arrives as args[0].
export function readBtlWindowFromPtr(winBase) {
    if (!isValidPtr(winBase)) return null;
    try {
        return {
            state: winBase.add(0x01).readU8(),
            type: winBase.add(0x06).readU16(),
            chrId: winBase.add(0x08).readU8(),
            submenuDepth: winBase.add(0x0B).readU8(),
            winCmdId: winBase.add(0x0C).readU16(),
            selCmdIdRender: winBase.add(0x16).readU16(),
            selCmdId: winBase.add(0x1E).readU16(),
            cmdListPtr: winBase.add(0x20).readPointer(),
            cmdListLen: winBase.add(0x24).readU32(),
            scrollPos: winBase.add(0x40).readU8(),
            selIdx: winBase.add(0x42).readU8(),
            validSelAmt: winBase.add(0x44).readU32()
        };
    } catch (e) { return null; }
}

// Equipment name cache, populated by the MsWeaponName onLeave hook in battle/index.js.
export const equipListNames = [];
export function resetEquipNames() { equipListNames.length = 0; }

export function cacheEquipName(retval) {
    if (retval.isNull()) return;
    try {
        const name = decodeFFXString(retval, 64);
        if (name.length > 0) equipListNames.push(name);
    } catch (e) {}
}

export function readCommandName(win) {
    if (!isValidPtr(win.cmdListPtr) || win.cmdListLen === 0) return null;
    // cmdListLen is a U32 read from a struct that may be mid-teardown during
    // battle transitions. Reject values larger than the in-engine pool size.
    if (win.cmdListLen > 0x1000) return null;
    if (win.selIdx >= win.cmdListLen) return null;
    try {
        const cmdId = win.cmdListPtr.add(win.selIdx * 2).readU16();
        const prefix = cmdId & 0xF000;
        if (win.type === 0x07 || win.type === 0x0F) {
            return (win.selIdx < equipListNames.length) ? equipListNames[win.selIdx] : 'equipment ' + win.selIdx;
        }
        if (win.type === 0x0A || win.type === 0x05) {
            const chrIdx = cmdId & 0xFF;
            const pArr = playerChrsAddr.readPointer();
            if (isValidPtr(pArr) && chrIdx < 0x1f) {
                const ns = decodeFFXSegments(pArr.add(chrIdx * CHR_SIZE + CHR_RAM_OFFSET), 40, 0);
                if (ns.length > 0) return ns[0];
            }
            return (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : 'chr' + chrIdx;
        }
        if (prefix === 0x2000) {
            const name = resolveCommandText(cmdId, btlItemBinAddr);
            if (name) return name + ' (' + getItemQuantity(cmdId) + ')';
        } else if (prefix === 0x3000) {
            return resolveCommandText(cmdId, btlCommandBinAddr);
        }
        let name2 = resolveCommandText(cmdId, btlCommandBinAddr);
        if (!name2) name2 = resolveCommandText(cmdId, btlItemBinAddr);
        return name2 || ('cmd:0x' + cmdId.toString(16));
    } catch (e) { return null; }
}

export function getCommandPartyPreview(cmdId) {
    try {
        const prefix = cmdId & 0xF000;
        const binAddr = (prefix === 0x2000) ? btlItemBinAddr : btlCommandBinAddr;
        const excelPtr = binAddr.readPointer();
        if (!isValidPtr(excelPtr)) return null;
        const hdr = excelPtr.add(0x8);
        const firstIdx = hdr.readU16(), lastIdx = hdr.add(2).readU16();
        const elementSize = hdr.add(4).readU16(), dataStart = hdr.add(8).readU32();
        const excelIdx = cmdId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;
        const elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        const pp = elementAddr.add(0x22).readU8();
        return { showHp: (pp & (1 << 6)) !== 0, showMp: (pp & (1 << 1)) !== 0 };
    } catch (e) { return null; }
}

// Native fn handle used to detect our own call-through when we invoke it
// (lets MsWeaponName hook skip its own recursion).
export const msWeaponNameFn = new NativeFunction(rva(0x3A0C70), 'pointer', ['uint', 'uint', 'int', 'pointer']);
