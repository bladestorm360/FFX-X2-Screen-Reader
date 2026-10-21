"""Read-only event trace of original launcher sound/speech calls during authorized tests."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import threading

ROOT = Path(__file__).resolve().parents[2]
SOURCE = r'''// @desc Original launcher FMOD sound and Tolk event trace (read-only)
const listeners = [];
const modules = Process.enumerateModules();
const fmod = modules.find(m => m.name.toLowerCase() === 'fmodex.dll');
if (!fmod) throw new Error('Original FMOD module is absent');
const exports = fmod.enumerateExports();
function find(name) { return exports.find(e => e.name === name || e.name.includes(name + '@')); }
const play = find('FMOD_System_PlaySound');
if (!play) throw new Error('Verified FMOD_System_PlaySound export is absent');
listeners.push(Interceptor.attach(play.address, {
    onEnter(args) { this.sound = String(args[2]); this.paused = args[3].toInt32(); this.channelOut = args[4]; },
    onLeave(ret) { send({type:'play-sound',sound:this.sound,paused:this.paused,result:ret.toInt32(),channel:String(this.channelOut.readPointer())}); }
}));
const volume = find('FMOD_Channel_SetVolume');
if (volume) listeners.push(Interceptor.attach(volume.address, {
    onEnter(args) { this.channel = String(args[0]); const b=new ArrayBuffer(4); new Uint32Array(b)[0]=args[1].toUInt32(); this.volume=new Float32Array(b)[0]; },
    onLeave(ret) { send({type:'channel-volume',channel:this.channel,volume:this.volume,result:ret.toInt32()}); }
}));
const tolk=modules.find(m => m.name.toLowerCase() === 'tolk.dll');
if (tolk) {
    const output=tolk.enumerateExports().find(e => e.name === 'Tolk_Output');
    if (output) listeners.push(Interceptor.attach(output.address, {
        onEnter(args) { this.text=args[0].readUtf16String(); this.interrupt=args[1].toInt32(); },
        onLeave(ret) { send({type:'tolk-output',text:this.text,interrupt:this.interrupt,accepted:ret.toInt32()}); }
    }));
}
send({type:'ready',process:Process.id,fmod:fmod.name});
'''

def main():
    import frida
    p=argparse.ArgumentParser(description=__doc__); p.add_argument('pid',type=int); p.add_argument('--seconds',type=int,default=180); args=p.parse_args()
    directory=ROOT/'Docs/Scripts/Frida/output/Launcher'; directory.mkdir(parents=True,exist_ok=True)
    path=directory/('launcher_events_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')+'.jsonl')
    done=threading.Event(); session=frida.attach(args.pid)
    with path.open('w',encoding='utf-8') as log:
        def message(value,data):
            payload=value.get('payload',value); record=dict(utc=datetime.now(timezone.utc).isoformat(),message=payload)
            log.write(json.dumps(record)+'\n'); log.flush(); print(json.dumps(payload),flush=True)
        session.on('detached',lambda *a:done.set())
        try:
            script=session.create_script(SOURCE); script.on('message',message); script.load()
            print('Log: '+str(path),flush=True); done.wait(args.seconds)
        finally:
            try:session.detach()
            except frida.InvalidOperationError:pass
    print('Launcher trace detached.',flush=True)
if __name__=='__main__':main()
