"""Read-only native-body/stack diagnostic for the authorized live test session."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
import json
from pathlib import Path
import threading

ROOT = Path(__file__).resolve().parents[2]
SOURCE = r"""
const game = Process.enumerateModules().find(m => m.name.toLowerCase() === 'ffx.exe');
if (!game) throw new Error('FFX.exe is not loaded');
const checks = [[0x488c40,163,0],[0x4b7ca0,60,0],[0x4b7c80,460,0x488d70],[0x4be4d0,512,0x488d70],[0x46ea00,410,0x46a830],[0x46e990,460,0x46a830],[0x46e990,120,0x46ea00],[0x46e9d0,120,0x46ea00],[0x470b20,481,0x42b480],[0x46c500,124,0x46c270],[0x4713a0,224,0x46c500],
    [0x471280,255,0x46c500],[0x477770,135,0x46e350],
    [0x46fed0,195,0x48e9d0],[0x5093b0,708,0x509ea0],
    [0x46c570,160,0x46c2e0],[0x471400,256,0x46c570],
    [0x4712e0,288,0x46c570],[0x4777f0,144,0x46e3c0],
    [0x46ff40,208,0x48ea60],[0x471d70,240,0x487dc0]];
const functions = checks.map(([offset,size,target]) => {
    const address = game.base.add(offset), end = address.add(size);
    const instructions = [], calls = [];
    let error = null;
    try {
        for (let pc = address; pc.compare(end) < 0;) {
            const ins = Instruction.parse(pc);
            const record = {pc:String(pc),text:ins.toString()};
            if (instructions.length < (offset===0x470b20 || offset===0x488c40 || offset>=0x4b0000 && offset<0x4c0000 ? 150 : 14)) instructions.push(record);
            if (ins.mnemonic === 'call' || ins.mnemonic === 'jmp') calls.push(record);
            pc = ins.next;
        }
    } catch (e) { error = String(e); }
    return {offset,target:String(game.base.add(target)),bytes:Array.from(new Uint8Array(address.readByteArray(20))),
            instructions,calls,error};
});
const modules = Process.enumerateModules().filter(m => /ffx|dinput8|tolk|minhook|nvda/i.test(m.name))
    .map(m => ({name:m.name,base:String(m.base),size:m.size}));
const threads = Process.enumerateThreads().slice(0,80).map(t => {
    try { return {id:t.id,state:t.state,frames:Thread.backtrace(t.context,Backtracer.ACCURATE)
        .slice(0,12).map(p => String(DebugSymbol.fromAddress(p)))}; }
    catch (e) { return {id:t.id,error:String(e)}; }
});
const candidates = [0x46c270,0x46c500,0x471280,0x4713a0,0x477770,0x46fed0];
const neighborhoods = candidates.map(offset => {
    const start = game.base.add(offset-128), bytes = new Uint8Array(start.readByteArray(512));
    const entries = [];
    for (let i=0;i<bytes.length-3;++i) {
        if (bytes[i]!==0x55 || bytes[i+1]!==0x8b || bytes[i+2]!==0xec) continue;
        const pc = start.add(i), instructions=[];
        let next=pc;
        for(let j=0;j<8;++j) {const ins=Instruction.parse(next);instructions.push(ins.toString());next=ins.next;}
        entries.push({offset:pc.sub(game.base).toUInt32(),instructions});
    }
    return {offset,entries};
});
const globals = [0xf26ae8,0xf26b28,0xf26b30,0xf26b34,0xf270d0,0xf270a4,0xefbbf8,0xd2a8e0,0x1fc44e4].map(offset=>{
    try {return {offset,value:game.base.add(offset).readU32()};}catch(e){return {offset,error:String(e)};}
});
const controller = game.base.add(0xf26b28).readPointer();
const controllerData = {address:String(controller),bytes:Array.from(new Uint8Array(controller.readByteArray(128)))};
const cached = game.base.add(0xf270e4).readPointer();
let cachedWorker = null;
try {
    const header=cached.readPointer(),chunk=cached.add(4).readPointer();
    const bytes=new Uint8Array(chunk.add(chunk.add(0x0c).readU32()).readByteArray(16));
    cachedWorker={address:String(cached),header:String(header),chunk:String(chunk),nameBytes:Array.from(bytes)};
}catch(e){cachedWorker={error:String(e),address:String(cached)};}
const common=game.base.add(0xf28558).readPointer();
const callTargets=[0x11,0x13,0x42,0x43,0x5c,0x64,0x6a,0x7c,0x84,0x8f,0x95,0x126].map(id=>{
    const entry=common.add(id*16);
    const targets=[0,4,8,12].map(o=>entry.add(o).readPointer());
    const bodies=targets.filter(p=>!p.isNull()).map(p=>{
        const code=[];let next=p;
        try {for(let j=0;j<128;++j){const ins=Instruction.parse(next);code.push(ins.toString());next=ins.next;if(ins.mnemonic==='ret')break;}}
        catch(e){code.push(String(e));}
        return {address:String(p),rva:p.sub(game.base).toUInt32(),instructions:code};
    });
    return {id,targets:targets.map(String),bodies};
});
const workerDebug=[];
const n=controller.add(0x0c).readU16()-controller.add(0x14).readU16()-controller.add(0x16).readU16()-controller.add(0x18).readU16();
let worker=controller.add(0x1c).readPointer();
for(let i=0;i<Math.min(controller.add(0x0c).readU16(),100);++i) {
 const chunk=worker.add(4).readPointer(), code=chunk.add(chunk.add(0x30).readU32());
 const threads=[];
 for(let l=0;l<9;++l) {const t=worker.add(0x12c+l*0x4c);threads.push({level:l,flags:t.readU32(),pc:t.add(0x18).readPointer().sub(code).toInt32(),call:t.add(0x1c).readU16(),entry:t.add(0x2c).readU16()});}
 workerDebug.push({index:i,flags:worker.add(0x34).readU32(),priority:worker.add(0x32).readU8(),queue:String(worker.add(0x80).readPointer()),threads});
 worker=worker.add(i<n?0xb58:i<n+controller.add(0x16).readU16()?0x558:i<n+controller.add(0x16).readU16()+controller.add(0x14).readU16()?0x5b8:0x2e8);
}
const owner=game.base.add(0xf26b82).readU16(), windows=[];
for(let slot=0;slot<8;++slot){
 const atel=game.base.add(0xf26d70+(owner*8+slot)*0x2c);
 const tk=game.base.add(0x14676f0+slot*4).readPointer();
 let data=null;try{data=Array.from(new Uint8Array(tk.readByteArray(64)));}catch(e){}
 windows.push({slot,atel:Array.from(new Uint8Array(atel.readByteArray(44))),tk:String(tk),data});
}
const buttonMap=Array.from(new Uint8Array(game.base.add(0xf30488).readByteArray(32)));
send({type:'diagnostic',buttonMap,windows,workerDebug,functions,modules,threads,neighborhoods,globals,controllerData,cachedWorker,callTargets});
"""


def main():
    import frida
    directory = evidence_path(ROOT / "Docs/Scripts/Frida/output/FFX")
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / ("cloister_native_diagnostic_" + datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f") + ".json")
    finished = threading.Event()
    result = {}

    def message(value, data):
        result.update(value.get("payload", value))
        finished.set()

    session = frida.attach("FFX.exe")
    try:
        script = session.create_script(SOURCE)
        script.on("message", message)
        script.load()
        if not finished.wait(10):
            raise RuntimeError("No diagnostic result received")
        path.write_text(json.dumps(result, indent=2), encoding="utf-8")
        print("Diagnostic saved: " + str(path))
        for function in result.get("functions", []):
            matched = any(function["target"] in call["text"] for call in function["calls"])
            print(f"RVA {function['offset']:x}: expected call {matched}; parse error {function['error']}")
    finally:
        session.detach()


if __name__ == "__main__":
    main()
