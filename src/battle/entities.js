// Player / monster / inventory readers for the battle context.

import {
    monsterChrsAddr, playerChrsAddr, saveDataAddr,
    INVENTORY_IDS_OFF, INVENTORY_CNT_OFF,
    CHR_SIZE, CHR_RAM_OFFSET, CHR_IN_BATTLE, CHARACTER_NAMES
} from '../core/addresses.js';
import { decodeFFXSegments } from '../core/charset.js';
import { isValidPtr } from '../core/safeRead.js';

export function readMonsterInfo(monIdx) {
    try {
        const arrayPtr = monsterChrsAddr.readPointer();
        if (!isValidPtr(arrayPtr)) return null;
        const chrBase = arrayPtr.add(monIdx * CHR_SIZE);
        if (chrBase.add(CHR_IN_BATTLE).readU8() === 0) return null;
        const ramBase = chrBase.add(CHR_RAM_OFFSET);
        const nameSegs = decodeFFXSegments(ramBase, 40, 0);
        const name = (nameSegs.length > 0) ? nameSegs[0] : 'Monster ' + monIdx;
        const currentHp = ramBase.add(0x1A4).readS32(), maxHp = ramBase.add(0x54).readS32();
        return {
            name: name,
            hpPct: (maxHp > 0) ? Math.round(currentHp / maxHp * 100) : 0,
            currentHp: currentHp,
            maxHp: maxHp
        };
    } catch (e) { return null; }
}

export function readPlayerInfo(chrIdx) {
    try {
        const arrayPtr = playerChrsAddr.readPointer();
        if (!isValidPtr(arrayPtr)) return null;
        const chrBase = arrayPtr.add(chrIdx * CHR_SIZE);
        if (chrBase.add(CHR_IN_BATTLE).readU8() === 0) return null;
        const ramBase = chrBase.add(CHR_RAM_OFFSET);
        return {
            name: (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : ('Player ' + chrIdx),
            currentHp: ramBase.add(0x1A4).readS32(),
            maxHp: ramBase.add(0x54).readS32(),
            currentMp: ramBase.add(0x1A8).readS32(),
            maxMp: ramBase.add(0x58).readS32()
        };
    } catch (e) { return null; }
}

// Resolves the character's current in-battle display name, falling back to
// CHARACTER_NAMES[chrIdx] if the RAM buffer isn't populated.
export function readPlayerDisplayName(chrIdx) {
    try {
        const pArr = playerChrsAddr.readPointer();
        if (isValidPtr(pArr) && chrIdx < 0x1f) {
            const ns = decodeFFXSegments(pArr.add(chrIdx * CHR_SIZE + CHR_RAM_OFFSET), 40, 0);
            if (ns.length > 0) return ns[0];
        }
    } catch (e) {}
    return (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : 'chr' + chrIdx;
}

export function getItemQuantity(itemId) {
    try {
        const invIds = saveDataAddr.add(INVENTORY_IDS_OFF);
        for (let i = 0; i < 256; i++) {
            const id = invIds.add(i * 2).readU16();
            if (id === itemId) return saveDataAddr.add(INVENTORY_CNT_OFF + i).readU8();
            if (id === 0) break;
        }
    } catch (e) {}
    return 0;
}
