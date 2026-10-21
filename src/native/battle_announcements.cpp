#include "events.hpp"
#include "ready_game_text.hpp"
#include "message_text.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
namespace ffx {
namespace {
constexpr unsigned Receiver=3;
enum Command : unsigned {Formation=1,Message};
std::atomic<unsigned> revision{1},protectionCut{0};
struct Confirmed {unsigned epoch=0,serial=0,cut=0,cancellation=0;};
async::AtomicSnapshot<Confirmed> confirmed;
struct Opening {std::wstring text;speechq::Header header;};
struct State {unsigned epoch=0,serial=0,characters=0;bool formationRead=false;std::vector<Opening> opening;};
ProcessStorage<State> storage;
void invalidateProtection() {protectionCut.fetch_add(1,std::memory_order_acq_rel);}
bool protectedPhase(const speechq::Header& header) {
    Confirmed phase;return confirmed.tryRead(phase) && phase.epoch==header.generation && phase.serial==revision.load() &&
        phase.cut==protectionCut.load() && phase.cancellation==header.cancellation;
}
void retireWorker() {
    auto& s=storage.get();
    if(s.epoch!=generation() || s.serial!=revision.load()) {
        std::vector<Opening>{}.swap(s.opening);s.formationRead=false;s.characters=0;s.epoch=0;s.serial=0;
    }
}
void form(const speechq::Header& header,const TextPlan& p,State& s,bool complete) {
    if(s.formationRead || p.batch>8) return;
    std::vector<std::pair<std::wstring,unsigned>> groups;
    const auto context=copiedControlContext(p.controls);
    for(unsigned i=0;i<p.batch;++i) {
        const auto name=renderTextPlan(p,i+1,context);
        if(name.empty()) {if(complete) return;continue;}
        const auto found=std::find_if(groups.begin(),groups.end(),[&](const auto& item) {return item.first==name;});
        if(found==groups.end()) groups.push_back({name,1});else ++found->second;
    }
    if(groups.empty() || p.serial!=revision.load() || p.epoch!=generation()) return;
    std::vector<std::wstring> names;
    for(const auto& group:groups) names.push_back(group.first+(group.second>1?L" x"+std::to_wstring(group.second):L""));
    s.formationRead=true;confirmed.publish({p.epoch,p.serial,p.ordinal,header.cancellation});
    auto protectedHeader=header;protectedHeader.context|=speechq::QueueProtected;
    speechq::emitOwned(L"Enemies: "+join(names),protectedHeader,"battle-formation",false,false);
    for(auto& message:s.opening) {
        message.header.context|=speechq::QueueProtected;
        speechq::emitOwned(std::move(message.text),message.header,"battle-message",false,false);
    }
    s.opening.clear();s.characters=0;
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring text,std::wstring) {
    if(p.epoch!=generation() || p.serial!=revision.load()) return;
    auto& s=storage.get();
    if(s.epoch!=p.epoch || s.serial!=p.serial) {
        s.opening.clear();s.characters=0;s.formationRead=false;s.epoch=p.epoch;s.serial=p.serial;
    }
    if(p.command==Formation) {form(header,p,s,p.flags!=0);return;}
    if(p.command!=Message || text.empty()) return;
    // An empty opening message did not trigger the old formation producer.
    // Capturing both operands lets preparation retain that exact decision.
    if(p.flags&2) form(header,p,s,true);
    if(s.formationRead || !(p.flags&1)) speechq::emitOwned(std::move(text),header,"battle-message",false,false);
    else if(s.opening.size()<speechq::EventLimit && text.size()<=speechq::CharacterLimit-s.characters) {
        s.characters+=static_cast<unsigned>(text.size());s.opening.push_back({std::move(text),header});
    } else recordLog("battle opening buffer capacity refused revision=%u",p.serial);
}
void deathBefore(Event& e) {
    if(!inBattle() || e.args[1]>=31) return;
    using Getter=uintptr_t(__cdecl*)(unsigned);
    const auto chr=reinterpret_cast<Getter>(address(0x394030))(static_cast<unsigned>(e.args[1]));
    if(!chr) return;
    e.before[0]=chr;e.before[1]=read<uint8_t>(chr+0xdcc)!=0 || (read<uint16_t>(chr+0x606)&1)!=0;
    ReadyBattleName name;copyReadyBattleName(name,static_cast<unsigned>(e.args[1]));
    static_assert(sizeof(name)<=sizeof(e.copied) && sizeof(name)%sizeof(e.copied[0])==0);
    std::memcpy(e.copied.data(),&name,sizeof(name));e.copiedCount=sizeof(name)/sizeof(e.copied[0]);
}
void deathAfter(Event& e) {
    if(!inBattle() || !e.before[0] || e.before[1] || e.copiedCount*sizeof(e.copied[0])!=sizeof(ReadyBattleName)) return;
    const auto chr=static_cast<uintptr_t>(e.before[0]);
    if(!read<uint8_t>(chr+0xdcc) && !(read<uint16_t>(chr+0x606)&1)) return;
    ReadyBattleName name;std::memcpy(&name,e.copied.data(),sizeof(name));
    if(name.actorLength>name.actor.size() || name.playerLength>name.player.size()) return;
    TextPlan p;prepareTextPlan(p,"battle-ko",false);
    const auto actor=planEncoded(p,reinterpret_cast<uintptr_t>(name.actor.data()),name.actorLength);
    const auto player=planEncoded(p,reinterpret_cast<uintptr_t>(name.player.data()),name.playerLength);
    p.headline=planConcat(p,planPair(p,TextPlanOp::Or,actor,player),planLiteral(p,L": KO"));commitTextPlan(p);
}
}
void resetBattleAnnouncements() {revision.fetch_add(1,std::memory_order_acq_rel);protectSpeech(false);}
namespace {
bool captureFormation(TextPlan& p,uintptr_t monsters,bool complete) {
    for(unsigned i=0;i<8;++i) {
        const auto chr=monsters+i*0xf90;
        if(!read<uint8_t>(chr+0xdc8) || read<uint8_t>(chr+0xdcc)) continue;
        if(!read<uintptr_t>(chr) || !read<uint8_t>(chr+0x15)) {if(complete) return false;continue;}
        planEncoded(p,chr+0x540,40);
    }
    p.batch=p.count;return true;
}
void prepareAnnouncement(TextPlan& p,unsigned command) {
    prepareTextPlan(p,command==Formation?"battle-formation":"battle-message",false);
    p.receiver=Receiver;p.command=command;p.serial=revision.load();p.ordinal=protectionCut.load();
}
}
void announceBattleFormation(bool complete) {
    if(!inBattle()) return;
    Confirmed phase;if(confirmed.tryRead(phase) && phase.epoch==generation() && phase.serial==revision.load()) return;
    const auto monsters=read<uintptr_t>(address(0xd34460));if(!monsters) return;
    TextPlan p;prepareAnnouncement(p,Formation);p.flags=complete;
    if(captureFormation(p,monsters,complete) && p.batch) commitTextPlan(p);
}
void battleOpeningMessage(uintptr_t source) {
    TextPlan p;prepareAnnouncement(p,Message);
    const auto monsters=read<uintptr_t>(address(0xd34460));p.flags=monsters?1:0;
    if(monsters && captureFormation(p,monsters,true)) p.flags|=2;
    // Original893560 registers the character/mode strings in namespace7,
    // then submits banner0. The banner index is not the substitution slot.
    // Copy renderer expansions now, before a later message reuses that state.
    p.headline=planMessage(p,7,source,true);commitTextPlan(p);
}
namespace {
void encounterStarted(Event&) {resetBattle();}
void appeared(Event& e) {const auto id=static_cast<unsigned>(e.args[0])&255;if(id>=20 && id<28) announceBattleFormation(true);}
void ready(Event& e) {if(e.args[0]==5 && e.args[1]==1) announceBattleFormation(false);}
}
void installBattleAnnouncements() {
    storage.prepare();registerTextPlanReceiver(Receiver,receive);registerTextPlanMaintenance(Receiver,retireWorker);
    speechq::registerDomainProtection(speechq::TextPlanDomain,protectedPhase,invalidateProtection);
    hook(0x3810f0,encounterStarted,nullptr);hook(0x38c800,deathBefore,deathAfter);
    hook(0x3982b0,nullptr,ready);hook(0x397090,nullptr,appeared);
}
}
