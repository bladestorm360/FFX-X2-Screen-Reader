#include "butterfly_processing.hpp"
#include "speech_domains.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include "camera_direction.hpp"
#include <deque>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace ffx::butterfly {
namespace {
constexpr unsigned FrameLimit=64;
struct Pending {speechq::Header header;Frame frame;};
struct State {
    std::deque<Pending> pending;std::vector<Entity> targets;
    unsigned epoch=0;int caught=-1;bool hunt=false,verifiedSeven=false;
};
ProcessStorage<State> storage;
std::array<std::atomic<unsigned>,FrameLimit> tickets{};
std::atomic<unsigned> latest{0},activeEpoch{0},recoverySerial{0};
async::AtomicSnapshot<Frame> recovery;
async::AtomicSnapshot<Targets> published;
unsigned ticketHint=0,ticketSerial=0;bool overloaded=false; // Native only.
void release(const Frame& frame) {
    if(frame.ticket>=FrameLimit) return;auto expected=frame.ticketSerial;
    tickets[frame.ticket].compare_exchange_strong(expected,0,std::memory_order_release,std::memory_order_relaxed);
}
bool claim(Frame& frame) {
    const auto serial=++ticketSerial?ticketSerial:++ticketSerial;
    for(unsigned i=0;i<FrameLimit;++i) {
        const auto at=(ticketHint+i)%FrameLimit;unsigned empty=0;
        if(tickets[at].compare_exchange_strong(empty,serial,std::memory_order_acquire,std::memory_order_relaxed)) {
            frame.ticket=at;frame.ticketSerial=serial;ticketHint=at+1;return true;
        }
    }return false;
}
void cancelCue() {
    ButterflyCue cue;cue.epoch=generation();publishButterflyCue(cue);cue.hazard=true;publishButterflyCue(cue);
}
int time(const Frame& frame) {
    return frame.tenths>=0 && frame.tenths<=9 && frame.seconds>=0 && frame.seconds<=59 && frame.minutes>=0 && frame.minutes<=1?
        frame.minutes*600+frame.seconds*10+frame.tenths:-1;
}
AtelPreparation qualify(const Frame& frame) {
    if(frame.epoch!=generation() || frame.course>=std::size(courses)) return AtelPreparation::Retired;
    const auto state=atelPreparation(frame.span.chunk);if(state!=AtelPreparation::Ready) return state;
    struct Qualification {const Course* course;bool valid=false;} proof{&courses[frame.course]};
    const auto inspect=[](const AtelScript& script,void* context) {
        auto& result=*static_cast<Qualification*>(context);const auto& course=*result.course;
        result.valid=script.codeLength==course.length && script.workers.size()==course.workers &&
            script.checksum==course.hash && script.name==course.name;
    };
    if(!visitReadyAtelScript(frame.span,inspect,&proof)) return AtelPreparation::Pending;
    return proof.valid?AtelPreparation::Ready:AtelPreparation::Rejected;
}
const Entity* nearest(const State& state,bool red) {
    const Entity* best=nullptr;for(const auto& target:state.targets)
        if(target.hazardous==red && (!best || target.distance<best->distance)) best=&target;
    return best;
}
bool buildTargets(State& state,const Frame& frame,Targets& copied) {
    state.targets.clear();copied={};copied.epoch=frame.epoch;copied.serial=frame.serial;copied.active=state.hunt;
    if(!state.hunt) return true;
    for(const auto& fact:frame.targets) {
        if(!fact.bound || !fact.enabled || !fact.visible || fact.id!=0x5008 || (fact.flags&0x100) ||
            !std::isfinite(fact.alpha) || !std::isfinite(fact.targetAlpha) || fact.alpha<=0 || fact.targetAlpha<=0) continue;
        const bool blue=std::abs(fact.red-.1f)<.001f && std::abs(fact.green-.3f)<.001f && std::abs(fact.blue-1.f)<.001f;
        const bool red=std::abs(fact.red-.9f)<.001f && std::abs(fact.green-.1f)<.001f && std::abs(fact.blue-.1f)<.001f;
        if(!blue && !red) continue;
        bool enabled=false;
        for(unsigned channel=0;channel<7;++channel) if(fact.interactions.channels&(1u<<channel)) {
            bool available=false;
            if(!readyAtelCallbackAvailable(frame.span,fact.local,fact.interactions.entries[channel],frame.saved,available)) return false;
            enabled|=available;
        }
        if(!enabled || (!fact.zone.valid && blue)) continue;
        Entity entity;entity.name=blue?L"Blue butterfly":L"Red butterfly";entity.category=L"Object";
        entity.butterfly=true;entity.hazardous=red;entity.worker=fact.worker;entity.actor=fact.actor;entity.id=0xb000u+fact.local;
        static_cast<InteractionShape&>(entity.zone)=fact.zone;entity.zone.worker=fact.worker;
        entity.zone.contactMinY=fact.contactMinY;entity.zone.contactMaxY=fact.contactMaxY;
        entity.position=fact.position;entity.distance=distanceXZ(frame.player,fact.position);entity.usefulEntries=EntryMask(1u<<4);
        state.targets.push_back(std::move(entity));copied.facts[copied.count++]=fact;
    }return true;
}
void audio(const State& state,const Frame& frame) {
    if(frame.serial!=latest.load(std::memory_order_acquire)) return;
    if(!state.hunt || !frame.allowed || !frame.cues || !frame.cameraValid) {cancelCue();return;}
    for(const auto& target:state.targets) if(target.hazardous && !target.zone.valid) {cancelCue();return;}
    const auto danger=nearest(state,true);auto target=nearest(state,false);
    if(danger && danger->distance<=danger->zone.range+35) target=danger;
    ButterflyCue cue;cue.epoch=frame.epoch;
    if(target) {
        const auto dx=target->position.x-frame.player.x,dz=target->position.z-frame.player.z,length=std::hypot(dx,dz);
        if(std::isfinite(length)) {
            cue.pan=static_cast<float>(length>.001?std::clamp((dx*frame.right.x+dz*frame.right.z)/length,-1.0,1.0):0);
            cue.proximity=static_cast<float>(std::clamp(1-target->distance/(20*11.13),0.0,1.0));cue.active=true;cue.hazard=target->hazardous;
        }
    }
    ButterflyCue inactive;inactive.epoch=frame.epoch;inactive.hazard=!cue.hazard;publishButterflyCue(inactive);publishButterflyCue(cue);
}
bool process(const speechq::Header& header,const Frame& frame) {
    auto& state=storage.get();
    if(state.epoch!=frame.epoch) {state.targets.clear();state.caught=-1;state.hunt=state.verifiedSeven=false;state.epoch=frame.epoch;}
    if(frame.caught<0 || frame.caught>7) {if(frame.serial==latest.load()) {activeEpoch=0;cancelCue();}return true;}
    const bool next=frame.active && !frame.exiting;
    bool verified=frame.caught==0?false:state.verifiedSeven;
    if(frame.collection && state.hunt && state.caught>=0 && frame.caught>state.caught && frame.caught==7) verified=true;
    const auto previousHunt=state.hunt;state.hunt=next;Targets copied;
    if(!buildTargets(state,frame,copied)) {state.hunt=previousHunt;return false;}
    const auto say=[&](std::wstring text,const char* producer,bool interrupt=false) {speechq::emitOwned(std::move(text),header,producer,false,interrupt);};
    if(frame.announce && frame.allowed) {
        if(next && !previousHunt) say(L"Butterfly hunt started","butterfly-start");
        else if(frame.collection && next && previousHunt && state.caught>=0 && frame.caught>state.caught)
            say(L"Blue butterfly. "+std::to_wstring(frame.caught)+L" of 7","butterfly-catch");
        if(!next && previousHunt) say(verified?L"All seven blue butterflies caught":time(frame)==0?L"Butterfly hunt time expired":L"Butterfly hunt ended","butterfly-end");
    }
    state.caught=frame.caught;state.verifiedSeven=verified;
    if(frame.allowed && state.hunt && frame.command) {
        if(frame.command==Status) {
            auto text=std::to_wstring(state.caught)+L" of 7 blue butterflies";const int remaining=time(frame);
            text+=remaining>=0?L". Time: "+std::to_wstring(remaining/10)+L"."+std::to_wstring(remaining%10)+L" seconds":L". Time unavailable";
            say(std::move(text),"butterfly-status",true);
        } else {
            const bool red=frame.command==Red;const auto target=nearest(state,red);
            if(!target) say(red?L"No active red butterfly":L"No active blue butterfly","butterfly-directions",true);
            else {
                wchar_t number[32]{};swprintf_s(number,L"%.1f",target->distance/11.13);
                say(target->name+L": "+number+L" "+cameraDirectionLabel({target->position.x-frame.player.x,0,target->position.z-frame.player.z},frame.forward,frame.cameraValid),"butterfly-directions",true);
                const auto danger=nearest(state,true);if(!red && danger && danger->distance<=danger->zone.range+35) say(L"Red butterfly nearby","butterfly-hazard");
            }
        }
    }
    if(frame.serial==latest.load(std::memory_order_acquire)) {activeEpoch=state.hunt?frame.epoch:0;published.publish(copied);}
    audio(state,frame);return true;
}
void maintain() {
    auto& pending=storage.get().pending;
    while(!pending.empty()) {
        const auto status=qualify(pending.front().frame);if(status==AtelPreparation::Pending) break;
        if(status==AtelPreparation::Ready && !process(pending.front().header,pending.front().frame)) break;
        if(status!=AtelPreparation::Ready && pending.front().frame.serial==latest.load()) {activeEpoch=0;cancelCue();}
        release(pending.front().frame);pending.pop_front();
    }
    Frame fresh;auto serial=recoverySerial.load(std::memory_order_acquire);
    if(serial && pending.empty() && recovery.tryRead(fresh) && fresh.serial==serial) {
        const auto status=qualify(fresh);
        if(status!=AtelPreparation::Pending && (status!=AtelPreparation::Ready || process({},fresh)))
            recoverySerial.compare_exchange_strong(serial,0,std::memory_order_acq_rel);
    }
}
void receive(const speechq::Header& header,const std::wstring& packet) {
    if(packet.size()*sizeof(wchar_t)!=sizeof(Frame)) return;Frame frame;std::memcpy(&frame,packet.data(),sizeof(frame));
    try {storage.get().pending.push_back({header,frame});} catch(...) {release(frame);throw;}maintain();
}
void resetWorker(unsigned epoch) {
    auto& state=storage.get();for(auto at=state.pending.begin();at!=state.pending.end();) {
        if(at->frame.epoch!=epoch) {release(at->frame);at=state.pending.erase(at);} else ++at;
    }
    state.targets.clear();state.hunt=state.verifiedSeven=false;state.caught=-1;state.epoch=epoch;
}
}
void installProcessing() {
    static_assert(std::is_trivially_copyable_v<Frame> && sizeof(Frame)%sizeof(wchar_t)==0);
    storage.prepare();speechq::registerDomain(speechq::ButterflyDomain,receive,resetWorker);
    speechq::registerDomainMaintenance(speechq::ButterflyDomain,maintain);
    if(!onAtelPrepared(speechq::notifyPreparation)) throw std::runtime_error("Butterfly preparation notification unavailable");
}
bool submit(Frame& frame) {
    frame.serial=latest.fetch_add(1,std::memory_order_acq_rel)+1;frame.ticket=FrameLimit;
    if(!frame.allowed || !frame.cues) cancelCue();
    if(!claim(frame) || !speechq::captureDomain(speechq::ButterflyDomain,&frame,sizeof(frame),nullptr,0,frame.epoch)) {
        release(frame);frame.announce=frame.collection=false;frame.command=Observe;recovery.publish(frame);recoverySerial=frame.serial;
        if(!overloaded) {overloaded=true;output(L"The Butterfly request queue is full.",false,"butterfly-overload");}return false;
    }
    overloaded=false;return true;
}
void resetProcessing() {latest.fetch_add(1);activeEpoch=0;recoverySerial=0;overloaded=false;for(auto& ticket:tickets) ticket=0;cancelCue();}
bool processingActive() {return activeEpoch.load(std::memory_order_acquire)==generation() && generation()!=0;}
bool copiedTargets(Targets& targets) {return published.tryRead(targets) && targets.epoch==generation() && targets.serial==latest.load(std::memory_order_acquire);}
}
