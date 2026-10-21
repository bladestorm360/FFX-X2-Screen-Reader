// @desc Read-only field targeting, equipment slot and visible status-page events (file-only; user-run)
// Never enabled alongside the native producer. No speech, timers or game writes.
function statusPage(phase) {
    return phase===4?0:phase>=6 && phase<=14 && !(phase&1)?phase/2-2:-1;
}
function targetText(name,mode,current,maximum) {
    return name+': '+(mode===1?'MP ':'HP ')+current+'/'+maximum;
}
function nextDirection(dx,dy) {
    const labels=['right','down-right','down','down-left','left','up-left','up','up-right'];
    return !dx && !dy?'same grid coordinates':labels[(Math.round(Math.atan2(dy,dx)*4/Math.PI)+8)%8];
}
if(typeof module!=='undefined') module.exports={statusPage,targetText,nextDirection};
else {
    const main=Process.enumerateModules()[0],base=main.base;
    if(main.name.toLowerCase()!=='ffx.exe') throw Error('FFX only');
    Process.enumerateModules().forEach(m=>m.enumerateExports().forEach(x=>{
        if(x.name==='FfxNative_Features' && new NativeFunction(x.address,'uint',[],'mscdecl')())
            throw Error('Disable the native reader before this offline prototype');
    }));
    const root='D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/';
    const specs=JSON.parse(File.readAllText(root+'Frida/FFX/native_extensions.json'));
    const file=new File(root+'Frida/output/FFX/field_ui_events_'+new Date().toISOString().replace(/[:.]/g,'-')+'.jsonl','w');
    const resolved={};
    function resolve(rva) {
        if(resolved[rva]) return resolved[rva];
        const s=specs.find(x=>x.rva===rva),a=s.anchor,owner=a?resolve(a.owner):null;
        const start=a?owner.add(a.end).add(owner.add(a.offset).readS32()):base.add(rva-4096);
        const pattern=s.tokens.map(x=>x===null?'??':x.toString(16).padStart(2,'0')).join(' ');
        const matches=Memory.scanSync(start,a?s.tokens.length:8192,pattern);
        if(matches.length!==1) throw Error('Ambiguous native field event '+rva.toString(16));
        return resolved[rva]=matches[0].address;
    }
    function emit(event) { file.write(JSON.stringify(event)+'\n');file.flush(); }
    function snapshot(rva,w) {
        if(rva===0x4c7400) return {phase:w.add(0x28).readS32(),row:w.add(0x48).readS16(),party:w.add(0x58).readS32(),resource:w.add(0x5c).readS32()};
        if(rva===0x4ceff0) return {phase:w.add(0x1c).readS32()};
        return {page:w.add(0x60).readS32(),category:w.add(0x58).readS32(),animation:w.add(0x52).readS16()};
    }
    [0x4c7400,0x4ceff0,0x4d2760].forEach(rva=>Interceptor.attach(resolve(rva),{
        onEnter(args) { this.window=args[0];this.before=snapshot(rva,this.window); },
        onLeave() { emit({rva,before:this.before,after:snapshot(rva,this.window),owner:Process.getCurrentThreadId()}); }
    }));
    emit({event:'ready',producer:'file-only field UI prototype'});
}
