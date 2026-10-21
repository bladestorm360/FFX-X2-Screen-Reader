#include "battle_sensor.hpp"
#include "settings.hpp"
#include "message_text.hpp"
#include "ready_game_text.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>
namespace ffx {
namespace {
constexpr unsigned Receiver=2,MaximumRows=8197;
enum Command : unsigned {Begin=1,Row,Description,Complete,Move};
struct View {unsigned epoch=0,serial=0,captured=0,owner=255,target=255,players=0,monsters=0;uintptr_t window=0;};
View view;unsigned nextSerial=0;
async::AtomicSnapshot<View> published;
std::atomic<unsigned> activeBatch{0};
struct Ready {unsigned epoch=0,serial=0,count=0;};
async::AtomicSnapshot<Ready> ready;
struct State {unsigned epoch=0,serial=0,pendingSerial=0,received=0,index=0;bool valid=false;std::vector<std::wstring> rows,pending;};
ProcessStorage<State> storage;
uintptr_t currentWindow() {
    const auto slot=read<uint8_t>(address(0x1fcc092),255);
    return slot<16?address(0xf3c910)+slot*0xf0:0;
}
uintptr_t character(unsigned id) {
    using Getter=uintptr_t(__cdecl*)(unsigned);
    return reinterpret_cast<Getter>(address(0x394030))(id);
}
bool validNativeContext() {
    return view.epoch==generation() && !scene && inBattle() && view.window && view.window==currentWindow() &&
        read<uint8_t>(view.window+2)==1 && read<uint8_t>(view.window+8)==view.owner &&
        read<uint16_t>(view.window+0xd8)==view.players && read<uint16_t>(view.window+0xda)==view.monsters &&
        read<uint16_t>(address(0xf3d1b4),255)==view.target && read<uint8_t>(address(0xf3c8fe)) &&
        read<int>(address(0xf3d6a0))!=1;
}
void retireWorker() {
    auto& s=storage.get();View current;
    if(s.epoch!=generation() || (!speechq::capturesPending() && (!published.tryRead(current) || current.serial!=s.serial || current.epoch!=s.epoch))) {
        std::vector<std::wstring>{}.swap(s.rows);s.serial=0;s.index=0;ready.publish({});
    }
    if(s.epoch!=generation() || (!speechq::capturesPending() && activeBatch.load()!=s.pendingSerial)) {
        std::vector<std::wstring>{}.swap(s.pending);s.pendingSerial=0;s.valid=false;
    }
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring text,std::wstring) {
    if(p.epoch!=generation()) return;auto& s=storage.get();
    if(p.command==Begin) {s.epoch=p.epoch;s.pendingSerial=p.serial;s.pending.clear();s.received=0;s.valid=true;return;}
    if(p.command==Row || p.command==Description) {
        if(s.pendingSerial!=p.serial || !s.valid) return;
        if(p.ordinal!=s.received++) {s.valid=false;s.pending.clear();return;}
        if(p.command==Row) {if(!text.empty()) s.pending.push_back(std::move(text));}
        else {
            size_t begin=0;
            for(size_t end=0;end<=text.size();++end) if(end==text.size() || text[end]==L'\n') {
                if(end>begin) s.pending.push_back(text.substr(begin,end-begin));begin=end+1;
            }
        }
        if(s.pending.size()>MaximumRows) {s.valid=false;s.pending.clear();}return;
    }
    if(p.command==Complete) {
        if(s.pendingSerial!=p.serial) return;
        if(!s.valid || p.ordinal!=s.received) {s.pending.clear();s.valid=false;return;}
        s.rows.swap(s.pending);s.serial=p.serial;s.index=0;s.pendingSerial=0;s.pending.clear();s.valid=false;
        ready.publish({p.epoch,p.serial,static_cast<unsigned>(s.rows.size())});return;
    }
    if(p.command==Move && s.serial==p.serial && !s.rows.empty()) {
        if(p.flags==VK_LEFT) s.index=0;
        if(p.flags==VK_RIGHT) s.index=static_cast<unsigned>(s.rows.size()-1);
        if(p.flags==VK_UP && s.index) --s.index;
        if(p.flags==VK_DOWN && s.index+1<s.rows.size()) ++s.index;
        speechq::emitOwned(s.rows[s.index],header,"battle-sensor-review",false,true);
    }
}
void prepare(TextPlan& p,unsigned command,unsigned ordinal=0) {
    prepareTextPlan(p,"battle-sensor-buffer",false);p.receiver=Receiver;p.command=command;p.serial=view.serial;p.ordinal=ordinal;
}
bool information(unsigned id,uintptr_t chr) {
    unsigned count=0;bool accepted=true;
    TextPlan begin;prepare(begin,Begin);accepted=commitTextPlan(begin);
    constexpr unsigned fields[]={0x5dd,0x5dc,0x5da,0x5db};
    constexpr const wchar_t* labels[]={L"Weakness",L"Absorbs",L"Nullifies",L"Resists"};
    constexpr unsigned bits[]={1,4,8,2};
    constexpr const wchar_t* elements[]={L"Fire",L"Lightning",L"Water",L"Ice"};
    for(unsigned kind=0;kind<4;++kind) {
        TextPlan p;prepare(p,Row,count);TextRef names=0;const auto mask=read<uint8_t>(chr+fields[kind]);
        for(unsigned i=0;i<4;++i) if(mask&bits[i]) names=planJoin(p,names,planLiteral(p,elements[i]));
        if(!names) continue;
        p.headline=planConcat(p,planLiteral(p,labels[kind]),planConcat(p,planLiteral(p,L": "),names));
        accepted=commitTextPlan(p) && accepted;++count;
    }
    const auto permanent=read<uint16_t>(chr+0x606),extra=read<uint16_t>(chr+0x616);
    // Getters can reuse their text buffer: copy each localized name before the next.
    TextPlan status;prepare(status,Row,count);TextRef statuses=0;
    for(unsigned i=0;i<12;++i) if(permanent&(1u<<i)) statuses=planJoin(status,statuses,planMenuText(status,1,0x1006+i));
    for(unsigned i=0;i<13;++i) if(read<uint8_t>(chr+0x608+i)) statuses=planJoin(status,statuses,planMenuText(status,1,0x1012+i));
    for(unsigned i=0;i<15;++i) if(extra&(1u<<i)) statuses=planJoin(status,statuses,planMenuText(status,1,0x1020+i));
    if(statuses) {
        status.headline=planConcat(status,planLiteral(status,L"Status: "),statuses);
        accepted=commitTextPlan(status) && accepted;++count;
    }
    using Help=uintptr_t(__cdecl*)(unsigned,int);
    TextPlan description;prepare(description,Description,count);
    description.headline=planMessage(description,0,reinterpret_cast<Help>(address(0x38f810))(id,1),true);
    accepted=commitTextPlan(description) && accepted;++count;
    TextPlan finish;prepare(finish,Complete,count);
    if(accepted) accepted=commitTextPlan(finish);
    activeBatch.store(0,std::memory_order_release);return accepted;
}
void displayed(Event&) {
    if(!read<uint8_t>(address(0xf3c8fe))) {resetBattleSensor();return;}
    highlightBattleSensor(currentWindow());
}
}
bool battleSensorVisible(unsigned id) {
    if(!inBattle() || id<20 || id>=28) return false;
    const auto chr=character(id);if(!chr || !read<uint8_t>(chr+0xdc8)) return false;
    if(read<uint16_t>(chr+0x616)&1) return true;
    using Party=int(__cdecl*)();using Allowed=int(__cdecl*)(unsigned);
    return reinterpret_cast<Party>(address(0x38fa30))()==-1 && reinterpret_cast<Allowed>(address(0x38f9e0))(id)==1;
}
void highlightBattleSensor(uintptr_t win) {
    const auto id=read<uint16_t>(address(0xf3d1b4),255);
    if(!inBattle() || scene || !win || win!=currentWindow() || read<uint8_t>(win+2)!=1 ||
        read<int>(address(0xf3d6a0))==1 || !battleSensorVisible(id)) {resetBattleSensor();return;}
    const unsigned nextPlayers=read<uint16_t>(win+0xd8),nextMonsters=read<uint16_t>(win+0xda);
    const auto nextOwner=read<uint8_t>(win+8);
    if(view.epoch==generation() && view.window==win && view.owner==nextOwner && view.target==id && view.players==nextPlayers && view.monsters==nextMonsters) return;
    const auto chr=character(id);if(!chr || !read<uint8_t>(chr+0xdc8)) {resetBattleSensor();return;}
    view={generation(),++nextSerial,0,nextOwner,id,nextPlayers,nextMonsters,win};published.publish(view);
    activeBatch.store(view.serial,std::memory_order_release);
    if(!information(id,chr)) resetBattleSensor();else {view.captured=1;published.publish(view);}
}
void resetBattleSensor() {view={};view.serial=++nextSerial;published.publish(view);activeBatch.store(0,std::memory_order_release);}
bool battleSensorAvailable() {
    if(!validNativeContext() || !view.captured) return false;
    Ready status;
    if(ready.tryRead(status) && status.epoch==view.epoch && status.serial==view.serial) return status.count!=0;
    // Complete capture precedes review keys in the same FIFO. Keep input owned
    // while preparation is pending; never wait or send early arrows to the game.
    return true;
}
bool battleSensorCapturesKey(unsigned vk,bool control) {
    return control && (vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT) && battleSensorAvailable();
}
bool dispatchBattleSensorKey(unsigned vk,bool control) {
    if(!battleSensorCapturesKey(vk,control)) return false;
    if(!validNativeContext() || !battleSensorVisible(view.target)) return true;
    TextPlan p;prepare(p,Move);p.flags=vk;commitTextPlan(p);return true;
}
void installBattleSensor() {
    storage.prepare();registerTextPlanReceiver(Receiver,receive);
    registerTextPlanMaintenance(Receiver,retireWorker);
    hook(0x4913f0,nullptr,displayed);
}
}
