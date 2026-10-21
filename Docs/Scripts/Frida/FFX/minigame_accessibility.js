// @desc Minigame accessibility prototype: checked match events and continuous race pickup stereo.
// Offline-authored first; user runs this through the launcher. Refuses a native
// minigame producer by switching to read-only diagnostics when it is installed.
'use strict';
// BEGIN GENERATED MINIGAME MODEL
// Offline Frida prototype model. Native production has one producer.
(function(root) {
  'use strict';
  function pickup(player, target, angle, radius, height) {
    if (![player.x,player.y,player.z,target.x,target.y,target.z,angle,radius,height].every(Number.isFinite) || radius<=.1 || height<=0) return null;
    const x=target.x-player.x,z=target.z-player.z;
    const ahead=x*Math.cos(angle)+z*Math.sin(angle), lateral=x*Math.sin(angle)-z*Math.cos(angle);
    if (ahead < -radius || Math.abs(target.y-player.y)>=height) return null;
    return {ahead,lateral,pan:Math.max(-1,Math.min(1,lateral/radius)),
      proximity:Math.max(.2,1-Math.max(0,ahead)/450),aligned:Math.abs(lateral)<radius-.1};
  }
  function carrier(base,slot) { return (base===2 || base===10) && slot>=0 && slot<6 ? base+slot : -1; }
  function team(worker) { return worker>=2 && worker<=7 ? 0 : worker>=10 && worker<=15 ? 1 : -1; }
  function possession(previous,next) { return team(previous)>=0 && team(next)>=0 && team(previous)!==team(next); }
  function goal(previous,next) { return previous>=0 && next>previous; }
  const model={pickup,carrier,team,possession,goal};
  if (typeof module!=='undefined') module.exports=model; else root.MinigameSpatial=model;
})(globalThis);

const minigameRecords=[{"name": "bltz0000", "checksum": 732075706, "length": 4647290, "workers": 41, "bindings": [{"index": 11, "low": 5202, "high": 1}, {"index": 12, "low": 5203, "high": 1}, {"index": 166, "low": 201330550, "high": 1}, {"index": 175, "low": 201330608, "high": 1}, {"index": 192, "low": 201330853, "high": 1}, {"index": 215, "low": 201330899, "high": 1}], "pickups": [], "source": "ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp"}, {"name": "bltz0000", "checksum": 1282163248, "length": 4647290, "workers": 41, "bindings": [{"index": 11, "low": 5202, "high": 1}, {"index": 12, "low": 5203, "high": 1}, {"index": 166, "low": 201330550, "high": 1}, {"index": 175, "low": 201330608, "high": 1}, {"index": 192, "low": 201330853, "high": 1}, {"index": 215, "low": 201330899, "high": 1}], "pickups": [], "source": "ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/cn_bltz0000.ebp"}, {"name": "bltz0000", "checksum": 1612610588, "length": 4646208, "workers": 41, "bindings": [{"index": 11, "low": 5202, "high": 1}, {"index": 12, "low": 5203, "high": 1}, {"index": 166, "low": 201330550, "high": 1}, {"index": 175, "low": 201330608, "high": 1}, {"index": 192, "low": 201330853, "high": 1}, {"index": 215, "low": 201330899, "high": 1}], "pickups": [], "source": "ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/psv_bltz0000.ebp"}, {"name": "bltz0000", "checksum": 4016248118, "length": 4647290, "workers": 41, "bindings": [{"index": 11, "low": 5202, "high": 1}, {"index": 12, "low": 5203, "high": 1}, {"index": 166, "low": 201330550, "high": 1}, {"index": 175, "low": 201330608, "high": 1}, {"index": 192, "low": 201330853, "high": 1}, {"index": 215, "low": 201330899, "high": 1}], "pickups": [], "source": "ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/psvcn_bltz0000.ebp"}, {"name": "nagi0000", "checksum": 1730323603, "length": 225132, "workers": 150, "bindings": [{"index": 67, "low": 1811939356, "high": 1}, {"index": 116, "low": 201326780, "high": 1}, {"index": 117, "low": 201326781, "high": 1}, {"index": 128, "low": 218104009, "high": 1}, {"index": 130, "low": 218104011, "high": 4}, {"index": 135, "low": 201326803, "high": 1}, {"index": 137, "low": 201326805, "high": 1}, {"index": 149, "low": 201326817, "high": 1}], "pickups": [{"worker": 39, "element": 0, "mask": 1}, {"worker": 40, "element": 0, "mask": 2}, {"worker": 41, "element": 0, "mask": 4}, {"worker": 42, "element": 0, "mask": 8}, {"worker": 43, "element": 0, "mask": 16}, {"worker": 44, "element": 1, "mask": 4}, {"worker": 45, "element": 1, "mask": 8}, {"worker": 46, "element": 1, "mask": 16}, {"worker": 47, "element": 1, "mask": 32}, {"worker": 48, "element": 1, "mask": 64}, {"worker": 49, "element": 1, "mask": 128}, {"worker": 50, "element": 2, "mask": 1}, {"worker": 51, "element": 2, "mask": 2}, {"worker": 52, "element": 2, "mask": 4}, {"worker": 53, "element": 2, "mask": 8}, {"worker": 54, "element": 2, "mask": 16}, {"worker": 55, "element": 2, "mask": 32}, {"worker": 56, "element": 2, "mask": 64}, {"worker": 57, "element": 2, "mask": 128}, {"worker": 58, "element": 3, "mask": 1}], "source": "ffx_ps2/ffx/master/jppc/event/obj/na/nagi0000/dbg_nagi0000.ebp"}, {"name": "nagi0000", "checksum": 1119091750, "length": 225368, "workers": 150, "bindings": [{"index": 67, "low": 1811939356, "high": 1}, {"index": 116, "low": 201326780, "high": 1}, {"index": 117, "low": 201326781, "high": 1}, {"index": 128, "low": 218104009, "high": 1}, {"index": 130, "low": 218104011, "high": 4}, {"index": 135, "low": 201326803, "high": 1}, {"index": 137, "low": 201326805, "high": 1}, {"index": 149, "low": 201326817, "high": 1}], "pickups": [{"worker": 39, "element": 0, "mask": 1}, {"worker": 40, "element": 0, "mask": 2}, {"worker": 41, "element": 0, "mask": 4}, {"worker": 42, "element": 0, "mask": 8}, {"worker": 43, "element": 0, "mask": 16}, {"worker": 44, "element": 1, "mask": 4}, {"worker": 45, "element": 1, "mask": 8}, {"worker": 46, "element": 1, "mask": 16}, {"worker": 47, "element": 1, "mask": 32}, {"worker": 48, "element": 1, "mask": 64}, {"worker": 49, "element": 1, "mask": 128}, {"worker": 50, "element": 2, "mask": 1}, {"worker": 51, "element": 2, "mask": 2}, {"worker": 52, "element": 2, "mask": 4}, {"worker": 53, "element": 2, "mask": 8}, {"worker": 54, "element": 2, "mask": 16}, {"worker": 55, "element": 2, "mask": 32}, {"worker": 56, "element": 2, "mask": 64}, {"worker": 57, "element": 2, "mask": 128}, {"worker": 58, "element": 3, "mask": 1}], "source": "ffx_ps2/ffx/master/jppc/event/obj/na/nagi0000/nagi0000.ebp"}];
// END GENERATED MINIGAME MODEL
const proxy=Process.getModuleByName('dinput8.dll');
function exported(name,result,args) {
  return new NativeFunction(proxy.getExportByName(name),result,args,'mscdecl');
}
const features=exported('FfxNative_Features','uint',[]);
const diagnosticsOnly=(features()&0x100)!==0;
const resolve=exported('FfxNative_ProfileAddress','pointer',['uint']);
const epoch=exported('FfxNative_Generation','uint',[]);
const speak=exported('FfxNative_Output','int',['pointer','int']);
const nativeCue=exported('FfxNative_PositionalCue','void',['float','float','int','uint']);
let copiedCue={pan:0,proximity:0,active:0,epoch:0};
function cue(pan,proximity,active,generation) { copiedCue={pan,proximity,active,epoch:generation}; if(!diagnosticsOnly)nativeCue(pan,proximity,active,generation); }
function native(rva,result,args) {
  const p=resolve(rva);if(p.isNull())throw new Error('Unverified native address: '+rva.toString(16));
  return new NativeFunction(p,result,args,'mscdecl');
}
const worker=native(0x46a7c0,'pointer',['int']);
const geometry=native(0x46c240,'pointer',['pointer']);
const variable=native(0x46c500,'pointer',['pointer','int','int']);
let generation=0,chunk=null,record=null,previousCarrier=-1,scores=null;
function say(text) { if(diagnosticsOnly)console.log(text); else speak(Memory.allocUtf16String(text),0); }
function reset() { generation=epoch();chunk=null;record=null;previousCarrier=-1;scores=null;cue(0,0,0,generation); }
function identity(w) {
  if(generation!==epoch())reset();
  if(w.isNull())return null;
  const c=w.add(4).readPointer();if(chunk && c.equals(chunk))return record;
  const length=c.readU32(),off=c.add(0x30).readU32(),count=c.add(0x34).readU16();
  if(!length || length>0x800000 || off>0x800000-length)return null;
  const name=c.add(c.add(12).readU32()).readUtf8String(8);
  const rows=minigameRecords.filter(r=>r.name===name && r.length===length && r.workers===count);
  if(!rows.length)return null;
  let hash=2166136261;
  const bytes=new Uint8Array(c.add(off).readByteArray(length));
  for(const byte of bytes)hash=Math.imul(hash^byte,16777619)>>>0;
  record=rows.find(r=>r.checksum===hash)||null;chunk=c;return record;
}
function value(w,b,element=0) {
  const h=w.readPointer(),c=w.add(4).readPointer();
  if(b.index>=h.add(2).readU16() || element>=(b.high&65535))throw new Error('Declaration bounds');
  const d=c.add(h.add(0x14).readU32()+b.index*8);
  if(d.readU32()!==b.low || d.add(4).readU32()!==b.high)throw new Error('Declaration changed');
  const p=variable(w,b.index,element);return b.low>>>28===6?p.readFloat():p.readU8();
}
function position(w) { const g=geometry(w);return {x:g.readFloat(),y:g.add(4).readFloat(),z:g.add(8).readFloat()}; }
function update(w) {
  const r=identity(w);if(!r)return;
  if(r.name==='bltz0000') {
    const n=MinigameSpatial.carrier(value(w,r.bindings[3]),value(w,r.bindings[2]));
    if(MinigameSpatial.possession(previousCarrier,n))say((MinigameSpatial.team(n)===0?'Your team':'Opponents')+' have the ball.');
    if(n>=0)previousCarrier=n;
    const next=[value(w,r.bindings[0]),value(w,r.bindings[1])];
    if(scores && next.some((x,i)=>MinigameSpatial.goal(scores[i],x)))say('Goal. Score '+next.join(' to '));
    scores=next;
  } else race(w,r);
}
function race(w,r) {
  const b=r.bindings;
  if(value(w,b[1])!==4 || value(w,b[7])!==1 || value(w,b[3])&2) {cue(0,0,0,generation);return;}
  const angle=value(w,b[0]),p=position(worker(0));let best=null;
  for(const target of r.pickups) {
    if(!(value(w,b[4],target.element)&target.mask))continue;
    const tw=worker(target.worker),g=geometry(tw);
    const s=MinigameSpatial.pickup(p,position(tw),angle,g.add(0x48).readFloat(),g.add(0x1c).readFloat());
    if(s && (!best || s.ahead<best.ahead))best=s;
  }
  cue(best?best.pan:0,best?best.proximity:0,best?1:0,generation);
}
Interceptor.attach(resolve(0x4713a0),{onEnter(args){this.worker=args[0];},onLeave(){try{update(this.worker);}catch(error){cue(0,0,0,epoch());console.log(String(error));}}});
Interceptor.attach(resolve(0x43d3b0),{onLeave(){try{const w=worker(0),r=identity(w);if(r && r.name==='nagi0000')race(w,r);}catch(error){cue(0,0,0,epoch());}}});
for(const rva of [0x46fed0,0x5093b0])Interceptor.attach(resolve(rva),{onEnter:reset});
rpc.exports.query=function(slot) {
  const w=worker(0),r=identity(w);if(!r)return 'No checked minigame is active.';
  if(r.name==='nagi0000')return JSON.stringify({balloons:value(w,r.bindings[5]),birdHits:value(w,r.bindings[6]),cue:copiedCue});
  const i=Number(slot),target=worker(i);return JSON.stringify({worker:i,position:position(target)});
};
console.log(diagnosticsOnly?'Read-only minigame diagnostics attached; native producer owns speech/audio.':'Prototype attached. Offline bindings checked; live actor/timing acceptance remains pending.');
