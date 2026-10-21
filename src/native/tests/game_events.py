"""Inspect mod startup and trace existing Tolk output during authorized game tests."""
from datetime import datetime, timezone
import json
from pathlib import Path
import threading

ROOT=Path(__file__).resolve().parents[3]
import sys
sys.path.insert(0,str(ROOT/"Docs/Scripts/Offline"))
from evidence_paths import evidence_root
SOURCE=r'''
const proxy=Process.getModuleByName('dinput8.dll');
function own(name) { const e=proxy.enumerateExports().find(e => e.name===name); if(!e)throw new Error('Missing mod export '+name); return new NativeFunction(e.address,'uint',[],{abi:'mscdecl'}); }
send({type:'mod-startup',features:own('FfxNative_Features')(),speechReady:own('FfxNative_SpeechReady')(),modules:Process.enumerateModules().filter(m=>/ffx|dinput8|tolk|minhook|nvda/i.test(m.name)).map(m=>({name:m.name,base:String(m.base),path:m.path}))});
const output=Process.getModuleByName('Tolk.dll').enumerateExports().find(e=>e.name==='Tolk_Output');
if(!output)throw new Error('Missing original Tolk_Output');
const listener=Interceptor.attach(output.address,{
 onEnter(args){this.text=args[0].readUtf16String();this.interrupt=args[1].toInt32();},
 onLeave(ret){send({type:'tolk-output',text:this.text,interrupt:this.interrupt,accepted:ret.toInt32()});}
});
'''
def main():
    import frida
    directory=evidence_root()/'frida-output/FFX'; directory.mkdir(parents=True,exist_ok=True)
    path=directory/('native_repair_events_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')+'.jsonl')
    done=threading.Event(); session=frida.attach('FFX.exe')
    with path.open('x',encoding='utf-8') as log:
        def message(value,data):
            payload=value.get('payload',value); log.write(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(),message=payload))+'\n'); log.flush(); print(json.dumps(payload),flush=True)
        session.on('detached',lambda *a:done.set())
        try:
            script=session.create_script(SOURCE); script.on('message',message); script.load(); print('Evidence: '+str(path),flush=True); done.wait(300)
        finally:
            try:session.detach()
            except frida.InvalidOperationError:pass
if __name__=='__main__':main()
