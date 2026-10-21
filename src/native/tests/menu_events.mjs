// Offline fixtures from native input/renderer bodies; never attaches to a game.
import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import vm from 'node:vm';

const baseAddress = 0x400000;
const bytes = Buffer.alloc(0x4000000);
let allocation = 0x3000000;
class Pointer {
    constructor(value) { this.value = Number(value); }
    add(offset) { return new Pointer(this.value + Number(offset)); }
    sub(pointer) { return new Pointer(this.value - pointer.value); }
    toUInt32() { return this.value >>> 0; }
    toString() { return '0x' + this.value.toString(16); }
    isNull() { return this.value === 0; }
    compare(pointer) { return Math.sign(this.value - pointer.value); }
    equals(pointer) { return this.value === pointer.value; }
    readU8() { return bytes.readUInt8(this.value); }
    readS8() { return bytes.readInt8(this.value); }
    readU16() { return bytes.readUInt16LE(this.value); }
    readS16() { return bytes.readInt16LE(this.value); }
    readS32() { return bytes.readInt32LE(this.value); }
    readU32() { return bytes.readUInt32LE(this.value); }
    readPointer() { return new Pointer(bytes.readUInt32LE(this.value)); }
    writePointer(pointer) { bytes.writeUInt32LE(pointer.value, this.value); }
    writeU32(value) { bytes.writeUInt32LE(value, this.value); }
    readUtf8String() { return bytes.toString('utf8',this.value,bytes.indexOf(0,this.value)); }
    writeByteArray(value) { Buffer.from(value).copy(bytes,this.value); }
}
const rva = offset => new Pointer(baseAddress + offset);
const alloc = length => { const pointer = new Pointer(allocation); allocation += length; return pointer; };
const text = value => { const pointer = alloc(512); bytes.write(value + '\0', pointer.value); return pointer; };
const natives = new Map();
const calls = [], speech = [], hooks = new Map();
const state = { sceneId: 2 };
function names(offset, prefix) {
    natives.set(offset, (id, strings) => {
        calls.push([offset, id]);
        const record = alloc(8);
        bytes.writeUInt16LE(3, record.value);
        const stringBase = text('---' + (id === 8 || id === 9 ? 'Same label' : prefix + id));
        strings.writePointer(stringBase);
        return record;
    });
}
names(0x390A40, 'Item ');
names(0x3908F0, 'Key ');
names(0x39A4C0, 'Ability ');
for (const [offset, prefix] of [[0x3ABE10, 'Gear '], [0x473AF0, 'Mode '], [0x4DDFA0, 'Fallback '], [0x385070, 'Character ']]) {
    natives.set(offset, id => { calls.push([offset, id]); return text(prefix + id); });
}
let selectedAeon = 8;
natives.set(0x4A9830, () => selectedAeon);
natives.set(0x4A9AB0, slot => [4, 0, 1, 6, 3, 5, 2][slot]);
natives.set(0x3852B0, () => 0);
natives.set(0x38FD40, (namespace, id) => text('Native stat ' + namespace + ':' + id));
natives.set(0x390500, id => id === 15 ? 7 : 2);
const gearRecord=alloc(32), requirements=alloc(4096);
natives.set(0x3ABBF0, () => gearRecord);
natives.set(0x4C1C00, () => requirements);
natives.set(0x4C1B50, () => requirements);
natives.set(0x4CD2F0, (row,id,item,count) => { item.writeU32(0xffffffff); count.writeU32(0); return 0; });
const context = vm.createContext({
    Map, Set, console,
    ptr: value => new Pointer(value),
    Process: { pointerSize: 4 },
    Memory: { alloc },
    NativeFunction: function(pointer) {
        const native = natives.get(pointer.value - baseAddress);
        assert.ok(native, 'unexpected native call');
        return native;
    }
});
const dependencies = {
    '../core/memory.js': { rva, base: new Pointer(baseAddress) },
    '../core/hooks.js': { attachRva: (offset, handler) => {
        if(!hooks.has(offset)) hooks.set(offset,[]);
        hooks.get(offset).push(handler);
    } },
    '../core/speech.js': { speak: value => speech.push(value), logFile: () => {} },
    '../utils/state.js': { state },
    '../core/safeRead.js': { isValidPtr: pointer => pointer && pointer.value > 0x10000 },
    '../core/charset.js': { decodeFFXSegments: pointer => {
        const end = bytes.indexOf(0, pointer.value);
        return [bytes.toString('utf8', pointer.value, end)];
    } }
};
const modules = new Map();
async function module(name) {
    if (modules.has(name)) return modules.get(name);
    const result = new vm.SourceTextModule(await fs.readFile(new URL('../../menus/' + name, import.meta.url), 'utf8'), { context });
    modules.set(name, result);
    await result.link(async specifier => {
        if (specifier.startsWith('./')) return module(specifier.slice(2));
        const exports = dependencies[specifier];
        assert.ok(exports, specifier);
        const synthetic = new vm.SyntheticModule(Object.keys(exports), function() {
            for (const [key, value] of Object.entries(exports)) this.setExport(key, value);
        }, { context });
        return synthetic;
    });
    return result;
}
for (const name of ['items.js', 'commandBars.js', 'fieldLists.js', 'partyLists.js', 'help.js', 'sphereLists.js', 'repairs.js']) {
    const result = await module(name); await result.evaluate(); result.namespace.install();
}
function u8(pointer, offset, value) { bytes.writeUInt8(value, pointer.value + offset); }
function s8(pointer, offset, value) { bytes.writeInt8(value, pointer.value + offset); }
function u16(pointer, offset, value) { bytes.writeUInt16LE(value, pointer.value + offset); }
function s16(pointer, offset, value) { bytes.writeInt16LE(value, pointer.value + offset); }
function s32(pointer, offset, value) { bytes.writeInt32LE(value, pointer.value + offset); }
function window(renderer = 0x4C7720) {
    const pointer = alloc(1024);
    pointer.add(0x10).writePointer(rva(renderer));
    u16(pointer, 0x30, 4); u8(pointer, 0x42, 2); s8(pointer, 0x45, 0);
    s32(pointer, 0x94, 8); return pointer;
}
function item(slot, id, count = 1, flags = 1) {
    const table = rva(0x1197730).add(slot * 4);
    u16(table, 0, id); u8(table, 2, count); u8(table, 3, flags);
}
function event(offset, pointer, mutate = () => {}) {
    eventArgs(offset, [pointer], mutate);
}
function eventArgs(offset, args, mutate = () => {}) {
    const list=hooks.get(offset), invocations=list.map(() => ({}));
    list.forEach((hook,i) => { if(hook.onEnter) hook.onEnter.call(invocations[i],args); });
    mutate(); list.forEach((hook,i) => { if(hook.onLeave) hook.onLeave.call(invocations[i]); });
}
function expectSpeech(expected, operation) {
    speech.length = 0; operation(); assert.deepEqual(speech, expected);
}

const items = window();
item(0, 12, 3); item(1, 2, 7); item(2, 8, 1); item(3, 9, 1); item(4, 44, 0);
expectSpeech(['Item 12, 3'], () => event(0x4C6DA0, items, () => s32(items, 0x28, 2)));
expectSpeech([], () => event(0x4C6DA0, items));
expectSpeech(['Item 2, 7'], () => event(0x4C6DA0, items, () => s32(items, 0x58, 1)));
expectSpeech(['Same label, 1'], () => event(0x4C6DA0, items, () => { s16(items, 0x48, 1); s32(items, 0x58, 0); }));
expectSpeech(['Same label, 1'], () => event(0x4C6DA0, items, () => s32(items, 0x58, 1)));
expectSpeech([], () => event(0x4C6DA0, items, () => { s16(items, 0x48, 2); s32(items, 0x58, 0); }));
expectSpeech(['Item 2, 6'], () => event(0x4C6DA0, items, () => { s16(items, 0x48, 0); s32(items, 0x58, 1); item(1, 2, 6); }));
// Return to the same item from a target window is a distinct activation.
s32(items, 0x28, 4);
expectSpeech(['Item 2, 6'], () => event(0x4C6DA0, items, () => s32(items, 0x28, 2)));
u16(rva(0x1196F30), 0, 71); u16(rva(0x1196F30), 2, 72);
expectSpeech(['Key 72'], () => event(0x4C6DA0, items, () => u8(items, 0x43, 1)));
assert.deepEqual(calls.at(-1), [0x3908F0, 72]);
expectSpeech([], () => event(0x4C6DA0, items, () => s32(items, 0x94, 1)));
s32(items, 0x94, 8);
expectSpeech(['Ability 2'], () => event(0x4C6DA0, items, () => u8(items, 0x43, 2)));
state.sceneId = 1;
expectSpeech([], () => event(0x4C6DA0, items, () => s32(items, 0x58, 0)));

const bar = window(0x4E3CF0);
bar.add(0x58).writePointer(text('Use')); bar.add(0x64).writePointer(text('Sort'));
expectSpeech(['Use'], () => event(0x4E3880, bar, () => s32(bar, 0x28, 5)));
expectSpeech(['Sort'], () => event(0x4E3880, bar, () => s16(bar, 0x48, 1)));
expectSpeech([], () => event(0x4E3880, bar));

state.sceneId = 0x15;
const overdrive = window(0x4D16C0);
item(0, 31); item(1, 32); item(2, 33, 2); item(3, 34, 3);
expectSpeech(['Ability 31'], () => event(0x4D0E80, overdrive, () => s32(overdrive, 0x28, 2)));
expectSpeech([], () => event(0x4D0E80, overdrive, () => s32(overdrive, 0x28, 3)));
expectSpeech(['Ability 32'], () => event(0x4D0E80, overdrive, () => s16(overdrive, 0x48, 1)));
expectSpeech(['Mode 33'], () => event(0x4D0E80, overdrive, () => s16(overdrive, 0x48, 2)));
expectSpeech(['Fallback 4181'], () => event(0x4D0E80, overdrive, () => s16(overdrive, 0x48, 3)));
expectSpeech([], () => event(0x4D0E80, overdrive, () => item(3, 34, 3, 0)));

state.sceneId = 9;
const gear = window(0x4D6830), gearTable = alloc(32);
rva(0x146A9F8).writePointer(gearTable); u16(gearTable, 0, 107); u16(gearTable, 2, 14);
expectSpeech(['Gear 107'], () => event(0x4B4460, gear, () => s32(gear, 0x28, 2)));
expectSpeech(['Gear 14'], () => event(0x4B4460, gear, () => s16(gear, 0x48, 1)));
gear.add(0x10).writePointer(rva(0x123456));
expectSpeech([], () => event(0x4B4460, gear, () => s16(gear, 0x48, 0)));

const main = alloc(256);
s32(rva(0x147151C), 0, 0); s32(rva(0x1471520), 0, 1);
expectSpeech(['Character 4'], () => event(0x4E0340, main, () => s32(main, 0x1C, 0x0E)));
expectSpeech(['Character 0'], () => event(0x4E0340, main, () => s32(main, 0x1C, 0x14)));
expectSpeech([], () => event(0x4E0340, main));
state.sceneId = 7;
const aeon = window();
expectSpeech(['Character 8'], () => event(0x4CCEA0, aeon, () => s32(aeon, 0x28, 3)));
expectSpeech(['Character 9'], () => event(0x4CCEA0, aeon, () => { selectedAeon = 9; }));
expectSpeech([], () => event(0x4CCEA0, aeon));
const increase = window(0x4CE720);
s32(rva(0x85A60C), 0, 91); s32(rva(0x85A60C), 0x1C, 19);
expectSpeech(['Native stat 12:91'], () => event(0x4CD6A0, increase, () => s32(increase, 0x28, 2)));
expectSpeech(['Native stat 12:19'], () => event(0x4CD6A0, increase, () => s16(increase, 0x48, 1)));

// Help's decoder is called on idle frames. Only initialization / native
// dispatch result 1 may arm it, and page text is spoken together in order.
s32(rva(0x8416B0), 0, 0x0B);
const pages = alloc(32), script = alloc(4096), strings = alloc(2048), table = alloc(32);
rva(0xD2A01C).writePointer(pages); rva(0xD2A03C).writePointer(script);
rva(0xD2A024).writePointer(table); rva(0xD2A040).writePointer(strings);
s32(rva(0xD2A080), 0, 1); s32(rva(0xD2A088), 0, 0);
s32(pages, 0, 128); s16(pages, 4, 2);
const first = script.add(128), second = first.add(0x138);
rva(0xD2A084).writePointer(first);
s16(first, 0x16, 0); s16(first, 0x18, -1);
s16(second, 0x16, 1); s16(second, 0x18, -1);
s32(table, 0, 0); s32(table, 4, 512);
const decoded = alloc(4096), result = alloc(8);
function decodedEntry(source, label) {
    eventArgs(0x376A50, [decoded, source], () => { bytes.fill(0, decoded.value, decoded.value + 4096); bytes.write(label + '\0', decoded.value); });
}
expectSpeech([], () => decodedEntry(strings, 'Idle renderer label'));
eventArgs(0x375EF0, []);
expectSpeech(['Same label Same label'], () => { decodedEntry(strings, 'Same label'); decodedEntry(strings.add(512), 'Same label'); });
expectSpeech([], () => { decodedEntry(strings, 'Same label'); decodedEntry(strings.add(512), 'Same label'); });
expectSpeech(['Focused help label'], () => {
    event(0x376220, result, () => { s32(result, 0, 1); rva(0xD2A084).writePointer(second); });
    decodedEntry(strings, 'Other row'); decodedEntry(strings.add(512), 'Focused help label');
});
expectSpeech([], () => {
    event(0x376220, result, () => s32(result, 0, 0));
    decodedEntry(strings.add(512), 'Idle changed text');
});

const pool = alloc(0x3000), action = pool.add(7 * 0x340), spheres = pool.add(8 * 0x340);
rva(0x1686108).writePointer(pool); s16(pool, 0x2700, -1);
u8(action, 0x23, 1); s16(action, 0x1E, 2);
action.add(0x40).writePointer(text('Move character')); action.add(0x4C).writePointer(text('Use Sphere'));
u8(spheres, 0x23, 1); s16(spheres, 0x1E, 3); s16(spheres, 0x16, 2);
u16(spheres, 0x48, 53); u16(spheres, 0x54, 15); u16(spheres, 0x60, 8);
expectSpeech(['Move character'], () => eventArgs(0x648E40, [], () => {
    s16(pool, 0x2700, 7); pool.add(0x2704).writePointer(rva(0x656330));
}));
expectSpeech(['Use Sphere'], () => eventArgs(0x6583E0, [], () => s16(action, 0x18, 1)));
expectSpeech(['Item 53, 2'], () => eventArgs(0x6583E0, [], () => eventArgs(0x6598A0, [], () => {
    s16(pool, 0x2700, 8); pool.add(0x2704).writePointer(rva(0x6560D0));
})));
expectSpeech([], () => eventArgs(0x6583E0, []));
expectSpeech(['Item 15, 7'], () => eventArgs(0x6583E0, [], () => { s16(spheres, 0x18, 1); s16(spheres, 0x1A, 1); }));
expectSpeech(['Same label, 2'], () => eventArgs(0x6583E0, [], () => s16(spheres, 0x18, 2)));
expectSpeech([], () => eventArgs(0x6583E0, [], () => pool.add(0x2704).writePointer(rva(0x123456))));
console.log('Menu event fixtures passed: native ordering, Items columns/quantities, repeats, field/party/aeon routing, Help idle silence, Sphere Grid callbacks/scrolling/nested ownership.');
