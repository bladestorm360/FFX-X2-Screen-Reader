// Offline Frida prototypes for source-verified silent menus. Production owns
// these exclusively in native/{shops,confirmations,field_details,esc_settings}.
// Bodies retained by native_bodies.py, 20261003_154724 through 155718.
import { rva, base } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { speak } from '../core/speech.js';
import { menuWindow, recordName, textAt } from './nativeText.js';
import { state } from '../utils/state.js';

const functions = new Map();
function native(offset, result, args) {
    if (!functions.has(offset)) functions.set(offset, new NativeFunction(rva(offset), result, args, 'mscdecl'));
    return functions.get(offset);
}
const quantity = id => native(0x390500, 'uint', ['uint'])(id);
function shopFocus(pointer, offset) {
    const window = menuWindow(pointer);
    if (!window || window.row < 0 || window.row >= window.count || window.count > 512 || window.depth < 0) return null;
    if (offset === 0x4D0E80 && window.renderer.sub(base).toUInt32() !== 0x4DA440) return null;
    let id;
    if ([0x4D77C0, 0x4D9D70].includes(offset)) {
        const gear = offset === 0x4D77C0;
        const bank = rva(gear ? 0x146AB68 : 0x146ABEC).readPointer();
        if (bank.isNull()) return null;
        const table = native(0x4C1AD0,'pointer',['uint','pointer'])(rva(gear ? 0x146AADC : 0x146ABE0).readU32(),bank);
        if (table.isNull() || window.row >= table.readU16()) return null;
        id = table.add(2 + window.row * 2).readU16();
    } else id = rva(0x1197730).add(window.row * 4).readU16();
    return {...window,id,phase:offset === 0x4D0E80 && window.phase === 3 ? 2 : window.phase};
}
const shopKey = s => s ? [s.phase,s.row,s.depth,s.id].join(':') : '';
function shopName(s,offset) {
    if ([0x4D77C0,0x4D7990].includes(offset)) {
        const strings = Memory.alloc(Process.pointerSize);
        const record = offset === 0x4D77C0 ?
            native(0x4D9140,'pointer',['uint'])(s.id) : native(0x3ABBF0,'pointer',['uint','pointer'])(s.id,strings);
        if (record.isNull()) return '';
        const language = native(0x3852B0,'int',[])();
        const name = textAt(native(0x3A0C70,'pointer',['uint','uint','int','pointer'])(record.readU16(),record.add(4).readU8(),language,ptr(0)));
        const price = native(offset === 0x4D77C0 ? 0x4D9340 : 0x4D93E0,'uint',['pointer'])(record);
        return name ? name + ', ' + price + ' Gil' : '';
    }
    const name = recordName(0x390A40,s.id);
    const price = native(offset === 0x4D9D70 ? 0x4DB390 : 0x4DB3E0,'uint',['uint'])(s.id);
    return name ? name + ', ' + quantity(s.id) + ', ' + price + ' Gil' : '';
}
function requirement(item,needed) {
    if (!item || item === 0xffffffff) return;
    const name = recordName(0x390A40,item);
    if (name) speak(name + ', ' + needed + ' required, ' + quantity(item) + ' available',false);
}
function requirements(pointer) {
    const window = menuWindow(pointer);
    if (!window || ![4,7,9].includes(state.sceneId) || window.row < 0 || window.row >= window.count || window.row >= 512 || window.depth < 0) return;
    const renderer = window.renderer.sub(base).toUInt32();
    if ([0x4CF9C0,0x4D6830].includes(renderer)) {
        const table=renderer === 0x4D6830 ? rva(0x146A9F8).readPointer() : null;
        if(table && table.isNull()) return;
        const id=table ? table.add(window.row*2).readU16() : rva(0x1197730).add(window.row*4).readU16();
        const record=native(0x3ABBF0,'pointer',['uint','pointer'])(id,ptr(0));
        if(record.isNull()) return;
        const slots=record.add(0xB).readU8(); if(slots > 4) return;
        const names=[];
        for(let i=0;i<slots;++i) {
            const ability=record.add(0xE+i*2).readU16();
            if(ability && ability !== 255) { const name=recordName(0x39A4C0,ability); if(name) names.push(name); }
        }
        if(names.length) speak(names.join(', '),false);
    } else if ([0x4CDB70,0x4D5F30].includes(renderer)) {
        const index = rva(0x1197730).add(window.row*4+3).readU8();
        if (index === 255) return;
        const table = native(renderer === 0x4CDB70 ? 0x4C1C00 : 0x4C1B50,'pointer',[])();
        if (!table.isNull()) requirement(table.add(index*8+4).readU16(),table.add(index*8+6).readU8());
    }
}
export function increaseRequirement(row) {
    if(row < 0 || row >= 10) return;
    const item=Memory.alloc(4), needed=Memory.alloc(4);
    item.writeU32(0xffffffff); needed.writeU32(0);
    native(0x4CD2F0,'int',['int','uint','pointer','pointer'])(row,native(0x4A9830,'uint8',[])(),item,needed);
    requirement(item.readU32(),needed.readU32());
}
export function install() {
    attachRva(0x4CCB30,{
        onEnter(args) {
            this.window=args[0]; this.phase=this.window.add(0x28).readS32();
            this.character=native(0x4A9830,'uint8',[])();
        },
        onLeave() {
            if(state.sceneId !== 7 || this.window.add(0x28).readS32() !== 4) return;
            const id=native(0x4A9830,'uint8',[])();
            if(id < 8 || id >= 18 || (this.phase === 4 && id === this.character)) return;
            const save=native(0x3853F0,'pointer',['uint'])(id); if(save.isNull()) return;
            speak(textAt(native(0x385070,'pointer',['uint'])(id)),true);
            const language=native(0x3852B0,'uint',[])(), rows=[];
            for(let row=0;row<10;++row) {
                const stat=rva(0x85A608).add(row*0x1C);
                const label=textAt(native(0x38FD40,'pointer',['int','uint','uint'])(12,stat.add(4).readU32(),language));
                if(label) rows.push(label + ': ' + native(0x4CEC70,'uint',['pointer','pointer','int'])(save,stat,1));
            }
            if(rows.length) speak(rows.join(', '),false);
            const count=rva(0x146A9E4).readU16(); if(count > 16) return;
            const abilities=[];
            for(let row=0;row<count;++row) {
                const name=recordName(0x39A4C0,rva(0x146A9C4).add(row*2).readU16());
                if(name) abilities.push(name);
            }
            if(abilities.length) speak(abilities.join(', '),false);
        }
    },'Native Aeon attributes input prototype');
    for (const offset of [0x4D77C0,0x4D7990,0x4D9D70,0x4D0E80]) attachRva(offset,{
        onEnter(args) { this.window=args[0]; this.before=shopKey(shopFocus(this.window,offset)); },
        onLeave() {
            const after=shopFocus(this.window,offset);
            if (!after || after.phase !== 2 || !after.id || shopKey(after) === this.before) return;
            const text=shopName(after,offset); if(text) speak(text,true);
        }
    },'Native shop input prototype');
    for(const offset of [0x4D9BA0,0x4D9DF0]) attachRva(offset,{
        onEnter(args) { this.window=args[0]; this.phase=this.window.add(0x28).readS32(); this.qty=this.window.add(0x48).readS16(); },
        onLeave() {
            const phase=this.window.add(0x28).readS32(), count=this.window.add(0x48).readS16();
            if(phase !== 3 || count < 1 || (phase === this.phase && count === this.qty)) return;
            speak(recordName(0x390A40,this.window.add(0x5C).readU16()) + ', ' + count + ', ' +
                (this.window.add(0x60).readS32()*count) + ' Gil',true);
        }
    },'Native shop quantity prototype');
    attachRva(0x4E2E20,{
        onEnter(args) { this.window=args[0]; this.phase=this.window.add(0x28).readS32(); this.row=this.window.add(0x48).readS16(); },
        onLeave() {
            const phase=this.window.add(0x28).readS32(), row=this.window.add(0x48).readS16();
            const opening=this.phase === 0 && [1,2,3].includes(phase);
            if(opening) speak(textAt(this.window.add(0x24).readPointer()),true);
            if(phase === 1 && row >= 0 && row < 2 && (opening || row !== this.row))
                speak(textAt(this.window.add(row === 0 ? 0x58 : 0x5C).readPointer()),!opening);
        }
    },'Native confirmation input prototype');
    attachRva(0x4B4460,{
        onEnter(args) { this.window=args[0]; const s=menuWindow(this.window); this.before=s ? [s.phase,s.row,s.depth].join(':') : ''; },
        onLeave() {
            const s=menuWindow(this.window);
            if(s && s.phase === 2 && [s.phase,s.row,s.depth].join(':') !== this.before) requirements(this.window);
        }
    },'Native Learn Customize Increase requirement prototype');
    attachRva(0x2A00,{
        onEnter(args) {
            if(!this.context.ecx.equals(rva(0x1EFB504).readPointer())) return;
            const field=args[0].toUInt32(); if(field > 64) return;
            for(let i=0;i<4;++i) {
                const entry=rva(0x839620).add(i*0x18);
                if(entry.add(0xC).readU32() !== field) continue;
                this.label=entry.add(4).readU32(); this.old=this.context.ecx.add(field*4).readU32(); this.value=args[1].toUInt32(); break;
            }
        },
        onLeave() {
            if(this.label === undefined || this.old === this.value) return;
            const buffer=Memory.alloc(256); buffer.writeByteArray(new Uint8Array(256));
            native(0x2414A0,'void',['uint','pointer','uint'])(this.label,buffer,256);
            const label=buffer.readUtf8String(); if(label) speak(label + ': ' + this.value,true);
        }
    },'Native ESC audio value prototype');
}
