// Renderer-verified text getters. Call only from the owning native input event.
import { rva } from '../core/memory.js';
import { decodeFFXSegments } from '../core/charset.js';
import { isValidPtr } from '../core/safeRead.js';

const getters = new Map();
function getter(offset, result, args) {
    if (!getters.has(offset)) getters.set(offset, new NativeFunction(rva(offset), result, args, 'mscdecl'));
    return getters.get(offset);
}

export function textAt(pointer) {
    if (!isValidPtr(pointer)) return '';
    return decodeFFXSegments(pointer, 256, 0, true)[0] || '';
}

export function recordName(offset, id) {
    const strings = Memory.alloc(Process.pointerSize);
    strings.writePointer(ptr(0));
    const record = getter(offset, 'pointer', ['uint', 'pointer'])(id, strings);
    const base = strings.readPointer();
    if (!isValidPtr(record) || !isValidPtr(base)) return '';
    return textAt(base.add(record.readU16()));
}

export function gearName(id) {
    // FUN_007ABE10 returns the native generated equipment name. Gear IDs are
    // inventory record IDs, not entries in the weapon-name dictionary.
    return textAt(getter(0x3ABE10, 'pointer', ['uint'])(id));
}

export function nativeText(offset, id) {
    return textAt(getter(offset, 'pointer', ['uint'])(id));
}

export function menuText(namespace, id) {
    const language = getter(0x3852B0, 'int', [])();
    return textAt(getter(0x38FD40, 'pointer', ['int', 'uint', 'uint'])(namespace, id, language));
}

export function menuWindow(pointer) {
    if (!isValidPtr(pointer)) return null;
    const phase = pointer.add(0x28).readS32();
    const row = pointer.add(0x48).readS16();
    const count = pointer.add(0x30).readU16();
    const depth = pointer.add(0x45).readS8();
    return { pointer, phase, row, count, depth, renderer: pointer.add(0x10).readPointer() };
}

export function enteredOrMoved(before, after, activePhase = 2) {
    if (!after || after.phase !== activePhase || after.depth < 0 || after.row < 0 || after.row >= after.count) return false;
    return !before || before.phase !== activePhase || before.depth !== after.depth || before.row !== after.row;
}
