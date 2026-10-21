// Help uses the event-message engine. Speech is armed by native initialization
// or a dispatch result of 1, then consumes the game's next decoded text.
// Idle renderer/decoder calls never arm an announcement or execute a getter.
import { rva } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { decodeFFXSegments } from '../core/charset.js';
import { isValidPtr } from '../core/safeRead.js';

let pending = [];
let copied = [];
function isHelp() { return rva(0x8416B0).readS32() === 0x0B; }
function source(entry) {
    if (!isValidPtr(entry) || entry.readU8() !== 0) return null;
    let index = entry.add(0x16).readS16();
    const variable = entry.add(0x18).readS16();
    if (variable >= 0) {
        if (variable >= 4096) return null;
        const variables = rva(0xD2A0A4).readPointer();
        if (!isValidPtr(variables)) return null;
        index += variables.add(variable * 4).readS32();
    }
    if (index < 0 || index >= 0x10000) return null;
    const table = rva(0xD2A024).readPointer();
    const strings = rva(0xD2A040).readPointer();
    if (!isValidPtr(table) || !isValidPtr(strings)) return null;
    return strings.add(table.add(index * 4).readS32());
}

function arm(pageChanged) {
    pending = [];
    copied = [];
    if (!isHelp()) return;
    const focus = rva(0xD2A084).readPointer();
    if (!pageChanged) {
        const selected = source(focus);
        if (selected) { pending.push(selected.toString()); return; }
    }
    const pages = rva(0xD2A01C).readPointer();
    const script = rva(0xD2A03C).readPointer();
    const page = rva(0xD2A088).readS32();
    const count = rva(0xD2A080).readS32();
    if (!isValidPtr(pages) || !isValidPtr(script) || page < 0 || page >= count || count > 4096) return;
    const header = pages.add(page * 8);
    const entries = header.add(4).readS16();
    if (entries < 0 || entries > 512) return;
    const start = script.add(header.readS32());
    for (let i = 0; i < entries; ++i) {
        const text = source(start.add(i * 0x138));
        if (text) pending.push(text.toString());
    }
    logFile('[HELP] native page event=' + page + ' text entries=' + pending.length);
}

export function install() {
    for (const offset of [0x375DF0, 0x375EF0]) {
        attachRva(offset, { onLeave() { arm(true); } }, 'Help native focus initialization ' + offset.toString(16));
    }
    attachRva(0x376220, {
        onEnter(args) { this.result = args[0]; this.page = rva(0xD2A088).readS32(); },
        onLeave() {
            if (!isHelp()) { pending = []; return; }
            if (this.result.readS32() === 1) arm(this.page !== rva(0xD2A088).readS32());
            else if (this.result.readS32() === 2) { pending = []; copied = []; }
        }
    }, 'Help native input dispatch');
    attachRva(0x376A50, {
        onEnter(args) { this.output = args[0]; this.source = args[1].toString(); },
        onLeave() {
            if (!pending.length || !isHelp() || pending[0] !== this.source) return;
            pending.shift(); // consume an armed entry once, including an empty conditional result
            const text = decodeFFXSegments(this.output, 4096, 0, true).join(' ');
            if (text) copied.push(text);
            if (!pending.length && copied.length) {
                speak(copied.join(' '), true);
                copied = [];
            }
        }
    }, 'Help event-owned native decoded text');
}
