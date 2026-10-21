// @desc Naming prototype: native letter/control identity and copied name cells (file-only; user-run)
// Never run with the production naming reader enabled. No polling or game writes.
function paletteSlot(group,column) { return group>=0 && column>=0 && column<5?group*5+column:-1; }
function focusIdentity(phase,palette,cursor,control,mode,character) {
    return phase===4?[palette,mode,mode===0?cursor:control,character].join(':'):'';
}
if(typeof module!=='undefined') module.exports={paletteSlot,focusIdentity};
else {
    const root='D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/';
    const main=Process.enumerateModules()[0],base=main.base;
    if(main.name.toLowerCase()!=='ffx.exe') throw Error('FFX only');
    Process.enumerateModules().forEach(m=>m.enumerateExports().forEach(x=>{
        if(x.name==='FfxNative_Features' && new NativeFunction(x.address,'uint',[],'mscdecl')())
            throw Error('Disable the native reader before this offline prototype');
    }));
    const specs=JSON.parse(File.readAllText(root+'Frida/FFX/native_extensions.json'));
    const stamp=new Date().toISOString().replace(/[:.]/g,'-');
    const file=new File(root+'Frida/output/FFX/naming_events_'+stamp+'.jsonl','w');
    function emit(event) { file.write(JSON.stringify(event)+'\n'); file.flush(); }
    const resolved={};
    function resolve(rva) {
        if(resolved[rva]) return resolved[rva];
        const s=specs.find(x=>x.rva===rva);
        const pattern=s.tokens.map(x=>x===null?'??':x.toString(16).padStart(2,'0')).join(' ');
        const anchor=s.anchor;
        const owner=anchor?resolve(anchor.owner):null;
        const start=anchor?owner.add(anchor.end).add(owner.add(anchor.offset).readS32()):base.add(rva-4096);
        const matches=Memory.scanSync(start,anchor?s.tokens.length:8192,pattern);
        if(matches.length!==1) throw Error('Naming event signature ambiguous '+rva.toString(16));
        return resolved[rva]=matches[0].address;
    }
    const input=resolve(0x4c3560),s=specs.find(x=>x.rva===0x4c3560),data={};
    s.refs.forEach(x=>data[x.reference]=input.add(x.offset).readPointer());
    let group=-1,column=0,grid=false;
    Interceptor.attach(input,{onEnter(args) { this.window=args[0]; },onLeave() {
        const phase=this.window.add(0x28).readS32();
        const values=[0x146a21c,0x146a228,0x146a22c,0x146a230,0x146a218].map(x=>data[x].readS32());
        const cells=Array.from(new Uint8Array(data[0x146a240].readByteArray(32*4)));
        emit({event:'input',phase,values,identity:focusIdentity(phase,...values),cells});
    }});
    Interceptor.attach(resolve(0x4c4530),{onEnter() { grid=true; group=-1; column=0; },onLeave() { grid=false; }});
    Interceptor.attach(resolve(0x4beda0),{onLeave(value) { if(grid) { ++group; column=0; emit({event:'row',group,text:value.toString()}); } }});
    Interceptor.attach(resolve(0x505ab0),{onEnter(args) {
        if(grid) emit({event:'glyph',slot:paletteSlot(group,column++),bytes:Array.from(new Uint8Array(args[0].readByteArray(4)))});
    }});
    emit({event:'ready',producer:'file-only naming prototype'});
}
