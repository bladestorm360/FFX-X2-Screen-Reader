// Bounded native capture for catalogued Blitzball/Chocobo scripts. A readable
// name is only a candidate: prepared checksum/allocation identity, declaration
// type and worker-to-actor binding must qualify each native operand.
// See Docs/features/puzzles/index.md#minigames.
#include "minigame_memory.hpp"
#include "events.hpp"
#include "native_worker_index.hpp"
#include <array>
#include <cstring>
#pragma optimize("gt",on)
namespace ffx {
namespace {
using Worker=uintptr_t(__cdecl*)(int);
using Geometry=uintptr_t(__cdecl*)(uintptr_t);
using Variable=uintptr_t(__cdecl*)(uintptr_t,int,int);
bool inside(const MinigameScript& script,unsigned offset,unsigned size) {
    return offset<=script.span.size && size<=script.span.size-offset;
}
bool declaration(const MinigameScript& script,const MinigameBinding& binding) {
    const auto header=read<uintptr_t>(script.worker);
    if(header<script.chunk || header-script.chunk>UINT32_MAX ||
        !inside(script,static_cast<unsigned>(header-script.chunk),0x28)) return false;
    const unsigned count=read<uint16_t>(header+2),offset=read<unsigned>(header+0x14);
    if(binding.index>=count || !inside(script,offset,count*8)) return false;
    unsigned raw[2]{};
    return copy(script.chunk+offset+binding.index*8,raw,sizeof(raw)) && raw[0]==binding.low && raw[1]==binding.high;
}
}
MinigameScript captureMinigameScript(uintptr_t worker) {
    MinigameScript script;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const auto count=read<uint16_t>(controller+0xc);
    if(!controller || !count || count>500) return script;
    if(!worker) worker=reinterpret_cast<Worker>(address(0x46a7c0))(read<uint16_t>(controller+0xa));
    const auto chunk=read<uintptr_t>(worker+4);
    if(!worker || !chunk || !read<uintptr_t>(worker) || !nativeAtelSpan(chunk,script.span)) return {};
    script.worker=worker;script.chunk=chunk;
    if(!inside(script,0,0x38)) return {};
    script.length=read<unsigned>(chunk);script.codeOffset=read<unsigned>(chunk+0x30);
    const auto nameOffset=read<unsigned>(chunk+0xc);script.workers=read<uint16_t>(chunk+0x34);
    if(!script.length || !inside(script,script.codeOffset,script.length) || !inside(script,nameOffset,8) ||
        !script.workers || script.workers>500) return {};
    char name[9]{};if(!copy(chunk+nameOffset,name,8)) return {};
    for(const auto& entry:minigameCatalog) if(script.length==entry.length && script.workers==entry.workers && !std::strcmp(name,entry.name)) {
        script.kind=name[0]=='b'?MinigameKind::Blitz:MinigameKind::Race;break;
    }
    return script;
}
// Resource preparation owns hashing. Pending is distinct from rejected so the
// first native event need not scan a whole script or accept an unverified map.
AtelPreparation qualifyMinigameScript(const MinigameScript& script) {
    if(script.kind==MinigameKind::None) return AtelPreparation::Rejected;
    const auto status=atelPreparation(script.chunk);if(status!=AtelPreparation::Ready) return status;
    AtelIdentity identity;if(!readyAtelIdentity(script.chunk,identity)) return AtelPreparation::Pending;
    if(identity.span.epoch!=script.span.epoch || identity.span.serial!=script.span.serial) return AtelPreparation::Retired;
    for(const auto& entry:minigameCatalog) if(identity.checksum==entry.checksum && identity.length==entry.length &&
        identity.workers==entry.workers && !std::strcmp(identity.name,entry.name)) return AtelPreparation::Ready;
    return AtelPreparation::Rejected;
}
MinigameScript minigameScript(uintptr_t worker) {
    auto result=captureMinigameScript(worker);
    if(qualifyMinigameScript(result)!=AtelPreparation::Ready) result.kind=MinigameKind::None;
    return result;
}
bool captureMinigameValue(const MinigameScript& script,const MinigameBinding& binding,unsigned element,float& result) {
    if(script.kind==MinigameKind::None || element>=(binding.high&0xffff) || !declaration(script,binding)) return false;
    const auto pointer=reinterpret_cast<Variable>(address(0x46c500))(script.worker,binding.index,element);
    if(binding.low>>28==6) return copy(pointer,&result,4) && std::isfinite(result);
    if(binding.low>>28!=0) return false;
    uint8_t value;if(!copy(pointer,&value,1)) return false;result=value;return true;
}
const MinigameBinding* minigameBinding(const MinigameScript& script,unsigned index) {
    if(script.kind==MinigameKind::Blitz) for(const auto& binding:blitzBindings) if(binding.index==index) return &binding;
    if(script.kind==MinigameKind::Race) for(const auto& binding:raceBindings) if(binding.index==index) return &binding;
    return nullptr;
}
MinigameActor minigameActor(const MinigameScript& script,unsigned local) {
    MinigameActor result; result.local=local;
    if(script.kind==MinigameKind::None || local>=script.workers) return result;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const unsigned count=read<uint16_t>(controller+0xc);
    if(!count || count>500) return result;
    NativeWorkerIdentity workerIdentity;
    if(!nativeWorkerIdentity(controller,script.chunk,local,workerIdentity)) return result;
    const auto worker=workerIdentity.worker;const unsigned index=workerIdentity.global;
    if(!inside(script,0x38+local*4,4)) return result;
    const auto offset=read<unsigned>(script.chunk+0x38+local*4);
    if(!inside(script,offset,0x34) || read<uintptr_t>(worker)!=script.chunk+offset || read<uint8_t>(script.chunk+offset)!=1) return result;
    const auto actor=read<uintptr_t>(worker+0x9c),actors=read<uintptr_t>(address(0x1fc44e4));
    const unsigned capacity=read<unsigned>(address(0x1fc44e0));
    if(!actors || capacity>128 || actor<actors || actor>=actors+capacity*0x880 || (actor-actors)%0x880) return result;
    uint16_t identity[2]; if(!copy(actor,identity,4) || !identity[1] || identity[1]>255 || read<unsigned>(actor+0x824)==0xcdcdcdcd) return result;
    const auto geometry=reinterpret_cast<Geometry>(address(0x46c240))(worker);
    float fields[19]{}; if(!geometry || !copy(geometry,fields,sizeof(fields))) return result;
    result.position={fields[0],fields[1],fields[2]}; result.radius=fields[18]; result.height=fields[7];
    if(!std::isfinite(fields[0]) || !std::isfinite(fields[1]) || !std::isfinite(fields[2]) || std::abs(fields[0])>50000 || std::abs(fields[2])>50000) return result;
    result.worker=worker; result.actor=actor; result.index=index; result.model=identity[0]; result.valid=true; return result;
}
}
