// Native event capture for the two catalogued butterfly courses. Capture only
// qualified script variables and bound, visible interactive actors; preparation
// owns cue/target computation. Reset/allocation changes retire the previous hunt.
// See Docs/features/puzzles/index.md#minigames and Docs/butterfly_catching_20261008.md.
#include "butterfly_processing.hpp"
#include "butterfly.hpp"

#include "events.hpp"
#include "native_worker_index.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace ffx {
namespace {
using Worker=uintptr_t(__cdecl*)(int);
using Variable=uintptr_t(__cdecl*)(uintptr_t,int,int);
using butterfly::Course;
using butterfly::courses;
const Course* course=nullptr;
uintptr_t chunk=0,owner=0;
AtelSpan span;
unsigned epoch=0;
bool exiting=false;
bool allowed() { return !scene && !inBattle() && !modMenuActive() && !utilityConsoleActive() && gameFocused(); }
void cancelCue() { ButterflyCue cue; cue.epoch=generation(); publishButterflyCue(cue); cue.hazard=true; publishButterflyCue(cue); }
uintptr_t worker(unsigned local) {
    AtelSpan current;
    if(!course || local>=course->workers || !nativeAtelSpan(chunk,current) || current.serial!=span.serial ||
        0x38ull+(local+1ull)*4>span.size) return 0;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const unsigned count=read<uint16_t>(controller+0xc);
    if(!controller || count>500) return 0;
    NativeWorkerIdentity identity;if(!nativeWorkerIdentity(controller,chunk,local,identity)) return 0;
    const auto offset=read<unsigned>(chunk+0x38+local*4);
    return offset<=span.size && span.size-offset>=0x34 && read<uintptr_t>(identity.worker)==chunk+offset?identity.worker:0;
}
int globalWorker(uintptr_t w) {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const unsigned count=read<uint16_t>(controller+0xc);
    if(!controller || count>500) return -1;
    const unsigned local=read<uint16_t>(w+0x2e,0xffff);NativeWorkerIdentity identity;
    if(!w || !nativeWorkerIdentity(controller,read<uintptr_t>(w+4),local,identity) || identity.worker!=w) return -1;
    return static_cast<int>(identity.global);
}
bool inside(unsigned offset,unsigned size) {return offset<=span.size && size<=span.size-offset;}
bool qualified() {
    AtelIdentity identity;return course && readyAtelIdentity(chunk,identity) && identity.span.serial==span.serial &&
        identity.checksum==course->hash && identity.length==course->length && identity.workers==course->workers && !std::strcmp(identity.name,course->name);
}
bool identify(uintptr_t w) {
    if(!w) return false;
    const auto c=read<uintptr_t>(w+4);AtelSpan captured;
    if(!c || !nativeAtelSpan(c,captured) || captured.size<0x38) return false;
    const auto header=read<uintptr_t>(w);const auto length=read<unsigned>(c),codeOffset=read<unsigned>(c+0x30);
    const auto count=read<uint16_t>(c+0x34);
    if(header<c || header-c>captured.size || captured.size-(header-c)<0x28 || codeOffset>captured.size ||
        length>captured.size-codeOffset || 0x38ull+count*4ull>captured.size) return false;
    const unsigned nameOffset=read<unsigned>(c+0xc);
    if(nameOffset>captured.size || captured.size-nameOffset<8) return false;
    char name[9]{};if(!copy(c+nameOffset,name,8)) return false;
    const Course* candidate=nullptr;
    for(const auto& record:courses) if(!std::strcmp(name,record.name) && length==record.length && count==record.workers) candidate=&record;
    if(!candidate) return false;
    const auto preparation=atelPreparation(c);
    if(preparation==AtelPreparation::Rejected || preparation==AtelPreparation::Retired) return false;
    if(preparation==AtelPreparation::Ready) {
        AtelIdentity identity;if(!readyAtelIdentity(c,identity)) return false;
        if(identity.checksum!=candidate->hash || identity.span.serial!=captured.serial) return false;
    }
    if(epoch!=generation() || chunk!=c || span.serial!=captured.serial) {
        resetButterfly();course=candidate;chunk=c;span=captured;epoch=generation();
    }
    owner=worker(0);return owner!=0;
}
bool value(uintptr_t w,unsigned index,unsigned expected,int& result) {
    const auto header=read<uintptr_t>(w);
    if(!w || read<uintptr_t>(w+4)!=chunk || header<chunk || header-chunk>UINT32_MAX || !inside(static_cast<unsigned>(header-chunk),0x28) || index>=read<uint16_t>(header+2)) return false;
    const unsigned offset=read<unsigned>(header+0x14);unsigned row[2]{};
    if(!inside(offset,(index+1)*8) || !copy(chunk+offset+index*8,row,sizeof(row)) || row[0]!=expected || row[1]!=1) return false;
    const auto p=reinterpret_cast<Variable>(address(0x46c500))(w,index,0);
    if(expected>>28==0) {uint8_t v;if(copy(p,&v,1)) {result=v;return true;}}
    if(expected>>28==5) return copy(p,&result,4);
    return false;
}
void capture(bool announce=false,bool collection=false,unsigned command=butterfly::Observe) {
    AtelSpan current;
    if(!nativeGameThread()) return;
    if(!course || epoch!=generation() || !nativeAtelSpan(chunk,current) || current.serial!=span.serial) {butterfly::resetProcessing();return;}
    owner=worker(0);const auto timer=worker(course->timer);int active=0,caught=0;
    butterfly::Frame frame;frame.epoch=epoch;frame.span=span;frame.course=course==&courses[0]?0u:1u;
    if(!value(owner,course->active,0x0c000041,active) || !value(owner,course->count,0x0c000042,caught)) {butterfly::resetProcessing();return;}
    frame.active=active==1;frame.caught=caught;frame.exiting=exiting;frame.announce=announce;frame.collection=collection;frame.command=command;
    value(timer,course->time,0x56000000,frame.tenths);value(timer,course->time+1,0x56000004,frame.seconds);value(timer,course->time+2,0x56000008,frame.minutes);
    frame.allowed=allowed();frame.cues=setting(Setting::ButterflyCues)!=0;frame.player=playerPosition();frame.cameraValid=cameraBasis(frame.forward,frame.right);
    const auto controller=read<uintptr_t>(address(0xf26ae8)),saved=address(0xd2ca90)+0x1ec;
    frame.saved.valid=read<uintptr_t>(controller+0x2c)==saved && copy(saved,frame.saved.bytes.data(),frame.saved.bytes.size());
    const auto actors=read<uintptr_t>(address(0x1fc44e4));const auto capacity=read<unsigned>(address(0x1fc44e0));
    if(actors && capacity && capacity<=128) for(unsigned i=course->first;i<=course->last;++i) {
        auto& fact=frame.targets[i-course->first];fact.local=i;
        const auto w=worker(i),actor=read<uintptr_t>(w+0x9c);
        if(!w || read<uint8_t>(read<uintptr_t>(w))!=1 || actor<actors || (actor-actors)%0x880 || (actor-actors)/0x880>=capacity) continue;
        fact.bound=true;fact.actor=static_cast<int>((actor-actors)/0x880);fact.worker=globalWorker(w);fact.id=read<uint16_t>(actor);
        if(fact.worker<0) {fact.bound=false;continue;}
        fact.enabled=read<uint16_t>(actor+2)!=0;fact.flags=read<unsigned>(actor+0x194);fact.visible=!(read<uint8_t>(actor+0x180) && !read<uint8_t>(actor+0x181));
        fact.red=read<float>(actor+0x334);fact.green=read<float>(actor+0x340);fact.blue=read<float>(actor+0x34c);
        fact.alpha=read<float>(actor+0x354);fact.targetAlpha=read<float>(actor+0x358);
        const auto zone=butterflyTouchZone(actor,playerActor());fact.zone=static_cast<const InteractionShape&>(zone);
        fact.contactMinY=zone.contactMinY;fact.contactMaxY=zone.contactMaxY;fact.position=zone.center;
        fact.interactions=captureNativeInteractionFacts(w,EntryMask(1u<<4));
    }
    butterfly::submit(frame);
}
bool observedIndex(unsigned i) { return course && (i==course->active || i==course->count || (i>=course->time && i<=course->time+2)); }
void beforeVariable(Event& event) {
    if(!nativeGameThread() || !identify(event.args[0])) return;
    const unsigned index=static_cast<unsigned>(event.args[1]);if(!observedIndex(index) || event.args[2]!=0) return;
    if(index>=course->time && index<=course->time+2 && read<uint16_t>(event.args[0]+0x2e,0xffff)!=course->timer) return;
    const unsigned raw=index==course->active?0x0c000041:index==course->count?0x0c000042:0x56000000+(index-course->time)*4;int previous=0;
    if(value(event.args[0],index,raw,previous)) {event.before[24]=1;event.before[25]=previous;}
}
void afterVariable(Event& event) {
    if(!nativeGameThread() || !event.before[24] || event.epoch!=generation() || !identify(event.args[0])) return;
    const unsigned index=static_cast<unsigned>(event.args[1]);int next=0;
    const unsigned raw=index==course->active?0x0c000041:index==course->count?0x0c000042:0x56000000+(index-course->time)*4;
    if(!value(event.args[0],index,raw,next) || next==static_cast<int>(event.before[25])) return;
    const unsigned local=read<uint16_t>(event.args[0]+0x2e,0xffff);
    const bool collector=index==course->count && local>=course->first && local<=course->last;
    const unsigned firstExit=course==&courses[0]?39u:27u;
    if(index==course->count && (local==firstExit || local==firstExit+1) && next==7) exiting=true;
    capture(true,collector);
}
void afterMovement(Event&) {if(nativeGameThread() && course && epoch==generation()) capture();}
}
InteractionZone butterflyTouchZone(uintptr_t actor,uintptr_t player) {
    InteractionZone zone; zone.shape=1; zone.height=-1;
    if(!actor || !player) return zone;
    zone.center={read<float>(actor+0xc),read<float>(actor+0x10),read<float>(actor+0x14)};
    const double actorRadius=actorContactRadius(actor,actor)*.5,playerRadius=actorContactRadius(player,player)*.5;
    zone.range=actorRadius+playerRadius;
    if((read<unsigned>(actor+0x194)&read<unsigned>(player+0x194)&0x40000000)!=0) {
        const auto bottom=[](uintptr_t body) { const double extent=read<float>(body+0x51c); return extent==0?read<float>(body+0x6a8):read<float>(body+0x10)-extent; };
        const double playerY=read<float>(player+0x10);
        zone.contactMinY=bottom(actor)-(read<float>(player+0x698)-playerY);
        zone.contactMaxY=read<float>(actor+0x698)-(bottom(player)-playerY);
    }
    zone.valid=std::isfinite(actorRadius) && actorRadius>0 && std::isfinite(playerRadius) && playerRadius>0 && std::isfinite(zone.range) && zone.range>0 && std::isfinite(zone.center.x) && std::isfinite(zone.center.y) && std::isfinite(zone.center.z) && std::isfinite(zone.contactMinY) && std::isfinite(zone.contactMaxY) && zone.contactMinY<=zone.contactMaxY;
    return zone;
}
void configureButterflyStarter(Entity& entity,const char* script,unsigned local,uintptr_t w) {
    const bool south=script && !std::strcmp(script,"mcfr0100") && local==19;
    const bool north=script && !std::strcmp(script,"mcfr0200") && local==9;
    if(!south && !north) return;
    try {
        if(!identify(w) || !qualified() || course!=(south?&courses[0]:&courses[1]) || worker(local)!=w) return;
        entity.name=L"Butterfly hunt start"; entity.category=L"Object";
        entity.butterfly=true; entity.butterflyStart=true; entity.hazardous=false;
        entity.butterflyEpoch=generation(); entity.usefulEntries=EntryMask(0x30u);
        entity.inactive=true; entity.zone.valid=false;
        const int global=globalWorker(w);
        if(global<0) return;
        entity.worker=global;
        const auto actors=read<uintptr_t>(address(0x1fc44e4)),actor=read<uintptr_t>(w+0x9c);
        const unsigned capacity=read<unsigned>(address(0x1fc44e0));
        if(!actors || !capacity || capacity>128 || actor<actors || actor>=actors+capacity*0x880 || (actor-actors)%0x880) return;
        entity.actor=static_cast<int>((actor-actors)/0x880);
        if(!entity.id) entity.id=0xb000u+local;
        int nativeActive=0;
        if(!value(owner,course->active,0x0c000041,nativeActive) || !allowed() || exiting || nativeActive!=0 || read<uint16_t>(actor)!=0x5008 || !read<uint16_t>(actor+2) || (read<unsigned>(actor+0x194)&0x100) || (read<uint8_t>(actor+0x180) && !read<uint8_t>(actor+0x181))) return;
        const float alpha=read<float>(actor+0x354),targetAlpha=read<float>(actor+0x358);
        if(!std::isfinite(alpha) || !std::isfinite(targetAlpha) || alpha<=0 || targetAlpha<=0) return;
        unsigned channels=0;
        const auto entries=interactionEntries(w,entity.usefulEntries,&channels);
        if(!(channels&4) || std::find(entries.begin(),entries.end(),4u)==entries.end()) return;
        entity.zone=butterflyTouchZone(actor,playerActor()); entity.zone.worker=global;
        entity.position=entity.zone.center;
        entity.distance=distanceXZ(playerPosition(),entity.position);
        entity.inactive=!entity.zone.valid;
    } catch(...) { entity.inactive=true; entity.zone.valid=false; }
}
void refreshButterflyStarter(Entity& entity) {
    if(!entity.butterflyStart) return;
    if(entity.butterflyEpoch!=generation() || entity.worker<0) { entity.inactive=true; entity.zone.valid=false; return; }
    const auto w=reinterpret_cast<Worker>(address(0x46a7c0))(entity.worker);
    const unsigned local=read<uint16_t>(w+0x2e,0xffff);
    const unsigned id=entity.id; const int actor=entity.actor;
    entity.inactive=true; entity.zone.valid=false;
    configureButterflyStarter(entity,local==19?"mcfr0100":local==9?"mcfr0200":"",local,w);
    entity.id=id;
    if(entity.actor!=actor) { entity.inactive=true; entity.zone.valid=false; }
}
void resetButterfly() {course=nullptr;chunk=owner=0;span={};epoch=0;exiting=false;butterfly::resetProcessing();}
bool butterflyNavigationLeaving(const AtelSpan& current) {return nativeGameThread() && epoch==generation() && current.chunk==chunk && current.serial==span.serial && exiting;}
bool butterflyActive() {return butterfly::processingActive() && allowed();}
void refreshButterflyAudio() {if(course) capture();else cancelCue();}
std::vector<Entity> butterflyTargets() {
    std::vector<Entity> entities;butterfly::Targets targets;
    if(!butterflyActive() || !butterfly::copiedTargets(targets)) return entities;
    for(unsigned i=0;i<targets.count && i<targets.facts.size();++i) {
        const auto& fact=targets.facts[i];const bool red=std::abs(fact.red-.9f)<.001f;
        Entity entity;entity.name=red?L"Red butterfly":L"Blue butterfly";entity.category=L"Object";entity.butterfly=true;entity.hazardous=red;
        entity.worker=fact.worker;entity.actor=fact.actor;entity.id=0xb000u+fact.local;entity.position=fact.position;
        static_cast<InteractionShape&>(entity.zone)=fact.zone;entity.zone.worker=fact.worker;
        entity.zone.contactMinY=fact.contactMinY;entity.zone.contactMaxY=fact.contactMaxY;
        entity.distance=distanceXZ(playerPosition(),fact.position);entity.usefulEntries=EntryMask(1u<<4);entities.push_back(std::move(entity));
    }return entities;
}
void refreshButterflyTarget(Entity& entity) {
    const auto id=entity.id;const int actor=entity.actor,local=entity.worker;
    const auto targets=butterflyTargets();
    for(const auto& target:targets) if(target.id==id && target.actor==actor && target.worker==local && target.hazardous==entity.hazardous) {entity=target;return;}
    entity.inactive=true;entity.zone.valid=false;
}
void butterflyStatus() {capture(false,false,butterfly::Status);}
void butterflyDirections(bool red) {capture(false,false,red?butterfly::Red:butterfly::Blue);}
void installButterfly() {
    butterfly::installProcessing();hook(0x4713a0,beforeVariable,afterVariable);hook(0x43d3b0,nullptr,afterMovement);
    hook(0x425010,nullptr,afterMovement);hook(0x42b500,nullptr,afterMovement);hook(0x426770,nullptr,afterMovement);
}
}
