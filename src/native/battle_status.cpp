#include "events.hpp"
#include "status_animation.hpp"
#include "ready_game_text.hpp"
#include "settings.hpp"
#include <array>
#include <algorithm>
#include <cstring>
namespace ffx {
namespace {
constexpr const wchar_t* permanent[]={L"KO",L"zombie",L"petrify",L"poison",L"power break",L"magic break",L"armor break",L"mental break",L"confuse",L"berserk",L"provoke",L"threaten"};
constexpr const wchar_t* temporal[]={L"sleep",L"silence",L"darkness",L"shell",L"protect",L"reflect",L"NulTide",L"NulBlaze",L"NulShock",L"NulFrost",L"regen",L"haste",L"slow"};
constexpr const wchar_t* extra[]={L"scan",L"distill power",L"distill mana",L"distill speed",L"distill move",L"distill ability",L"shield",L"boost",L"eject",L"auto-life",L"curse",L"defend",L"guard",L"sentinel",L"doom"};
constexpr const wchar_t* buffs[]={L"Cheer",L"Aim",L"Focus",L"Reflex",L"Luck",L"Jinx"};
using Snapshot=std::array<intptr_t,32>;
struct Calculated {
    unsigned epoch=0,id=0,source=0,serial=0,revision=0;
    uintptr_t actor,packet; // Opaque native identities; never dereferenced later.
    Snapshot before,after;
    unsigned spokenBuffs=0;
};
// Two native buffers of sixteen packets for each of31 actors. Direct packet
// addressing replaces producer allocation and linear registry scans.
std::array<Calculated,31*2*16> calculated{};
unsigned calculatedRevision=1,calculatedCount=0;
Calculated* packetRecord(unsigned id,uintptr_t actor,uintptr_t packet) {
    if(id>=31 || !actor || packet<actor+0x78c) return nullptr;
    const auto offset=packet-(actor+0x78c),bank=offset/0x2d8,part=offset%0x2d8;
    if(bank>=2 || part%0x2c || part/0x2c>=16) return nullptr;
    return &calculated[id*32+bank*16+part/0x2c];
}
bool current(const Calculated& item) {
    return item.packet && item.epoch==generation() && item.revision==calculatedRevision;
}
void discard(Calculated& item) {if(current(item) && calculatedCount) --calculatedCount;item.packet=0;}
Snapshot snapshot(unsigned id,uintptr_t expected=0) {
    Snapshot result{}; if(id>=31) return result;
    using Getter=uintptr_t(__cdecl*)(unsigned);
    const auto chr=reinterpret_cast<Getter>(address(0x394030))(id);
    if(!chr || (expected?chr!=expected:!read<uint8_t>(chr+0xdc8))) return result;
    result[0]=chr; result[1]=read<uint16_t>(chr+0x606); result[2]=read<uint16_t>(chr+0x616);
    result[3]=read<uint8_t>(chr+0xdcc);
    for(unsigned i=0;i<13;++i) result[4+i]=read<uint8_t>(chr+0x608+i)!=0;
    for(unsigned i=0;i<6;++i) result[17+i]=read<uint8_t>(chr+0x65e+i);
    result[23]=read<uint8_t>(chr+0x5c8); // Doom's remaining turn counter.
    result[24]=read<uint8_t>(chr+0xdd1); // Native capture result.
    return result;
}
unsigned target(const Event& e) { return (e.rva==0x39a1a0?e.args[0]:e.args[2])&255; }
void before(Event& e) {
    // The outer effect owns nested status clears, so one semantic mutation
    // produces one change report without suppressing repeated text.
    if(!inBattle() || e.parentRva==0x38e680 || e.parentRva==0x38f0b0 || e.parentRva==0x38c800) return;
    e.before=snapshot(target(e));
    if(e.rva==0x38e680 && static_cast<uintptr_t>(e.before[0])!=e.args[3]) e.before={};
    if(e.rva!=0x38f0b0 || !e.before[0] || (e.args[4]&8)) return;
    for(unsigned slot=0;slot<2;++slot) {
        const auto buffer=static_cast<uintptr_t>(e.before[0])+0x774+slot*0x2d8;
        const unsigned index=read<uint8_t>(buffer),count=read<uint8_t>(buffer+1);
        const bool matched=read<uint8_t>(buffer+2)==e.args[0] && read<uint8_t>(buffer+3)==e.args[1] && index<count && count<=16;
        e.copied[slot]=matched?index:256;
        e.before[26+slot]=matched?buffer+0x18+index*0x2c:0;
        e.before[28+slot]=matched?read<uint16_t>(static_cast<uintptr_t>(e.before[26+slot])+0x14):0;
        e.before[30+slot]=matched?read<uint16_t>(static_cast<uintptr_t>(e.before[26+slot])+0x16):0;
        for(unsigned i=0;i<13;++i) e.copied[2+slot*13+i]=matched && read<uint8_t>(static_cast<uintptr_t>(e.before[26+slot])+7+i)!=0;
    }
    e.copiedCount=28;
}
std::wstring changesText(const Snapshot& old,const Snapshot& now) {
    std::vector<std::wstring> changes;
    const bool oldKo=old[3] || (old[1]&1),newKo=now[3] || (now[1]&1);
    // Confirmed death has its own producer, including deactivated monsters.
    if(oldKo && !newKo) changes.push_back(L"revived");
    for(unsigned bit=1;bit<_countof(permanent);++bit) if((now[1]^old[1])&(1u<<bit))
        changes.push_back(std::wstring(permanent[bit])+((now[1]&(1u<<bit))?L"":L" cured"));
    for(unsigned i=0;i<_countof(temporal);++i) if(now[4+i]!=old[4+i])
        changes.push_back(std::wstring(temporal[i])+(now[4+i]?L"":L" ended"));
    for(unsigned bit=0;bit<_countof(extra);++bit) if((now[2]^old[2])&(1u<<bit)) {
        auto text=std::wstring(extra[bit])+((now[2]&(1u<<bit))?L"":L" ended");
        if(bit==14 && (now[2]&(1u<<bit))) text+=L", "+std::to_wstring(now[23])+L" turns";
        changes.push_back(text);
    }
    for(unsigned i=0;i<_countof(buffs);++i) if(now[17+i]!=old[17+i])
        changes.push_back(std::wstring(buffs[i])+L" "+std::to_wstring(now[17+i]));
    if(now[24] && !old[24]) changes.push_back(L"captured");
    return join(changes);
}
std::wstring statusText(const Snapshot& now) {
    std::vector<std::wstring> states;
    if(now[3] || (now[1]&1)) states.push_back(L"KO");
    for(unsigned bit=1;bit<_countof(permanent);++bit) if(now[1]&(1u<<bit)) states.push_back(permanent[bit]);
    for(unsigned i=0;i<_countof(temporal);++i) if(now[4+i]) states.push_back(temporal[i]);
    for(unsigned bit=0;bit<_countof(extra);++bit) if(now[2]&(1u<<bit)) {
        auto text=std::wstring(extra[bit]);
        if(bit==14) text+=L", "+std::to_wstring(now[23])+L" turns";
        states.push_back(text);
    }
    for(unsigned i=0;i<_countof(buffs);++i) if(now[17+i]) states.push_back(std::wstring(buffs[i])+L" "+std::to_wstring(now[17+i]));
    return states.empty()?L"None":join(states);
}
struct StatusPacket {
    Snapshot before{},after{};
    ReadyBattleName name;
    CopiedControls controls;
    std::array<int,4> vitals{};
    unsigned epoch=0,kind=0;
    bool maximumHp=false,maximumMp=false;
};
void processStatus(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)!=sizeof(StatusPacket)) return;
    StatusPacket p;std::memcpy(&p,owned.data(),sizeof(p));
    if(p.epoch!=generation() || p.kind>1) return;
    const auto name=renderReadyBattleName(p.name,copiedControlContext(p.controls));
    std::wstring text;
    if(p.kind==0) {
        const auto changes=changesText(p.before,p.after);
        if(changes.empty()) return;
        text=name+L": "+changes;
    } else {
        auto hp=L"HP "+std::to_wstring(p.vitals[0]);
        if(p.vitals[1]>0) {
            if(p.maximumHp) hp+=L"/"+std::to_wstring(p.vitals[1]);
            else hp+=L" ("+std::to_wstring(std::clamp<int64_t>((static_cast<int64_t>(p.vitals[0])*100+p.vitals[1]/2)/p.vitals[1],0,100))+L"%)";
        }
        auto mp=L"MP "+std::to_wstring(p.vitals[2]);
        if(p.maximumMp && p.vitals[3]>0) mp+=L"/"+std::to_wstring(p.vitals[3]);
        text=name+L" "+hp+L" "+mp+L". Status: "+statusText(p.after);
    }
    if(p.epoch==generation()) speechq::emitOwned(std::move(text),header,p.kind?"battle-active-stats":"battle-status",false,p.kind!=0);
}
void announce(unsigned id,const Snapshot& old,const Snapshot& now) {
    if(std::equal(old.begin()+1,old.begin()+25,now.begin()+1)) return;
    StatusPacket p;p.before=old;p.after=now;p.epoch=generation();
    copyReadyBattleName(p.name,id);prepareCopiedControlSnapshot(p.controls);
    speechq::captureDomain(speechq::BattleStatusDomain,p,p.epoch);
}
void animation(Event& e) {
    if(!inBattle() || !calculatedCount) return;
    unsigned owner=0,buff=0;
    if(!statusAnimationTarget(e,owner,buff)) return;
    const auto id=static_cast<unsigned>(e.args[0]); const auto now=snapshot(id);
    if(!now[0]) return;
    using Getter=uintptr_t(__cdecl*)(unsigned);
    const auto action=reinterpret_cast<Getter>(address(0x3b09c0))(owner);
    if(!action) return;
    const auto serial=read<uint8_t>(action+2);
    for(unsigned slot=0;slot<2;++slot) {
        const auto buffer=static_cast<uintptr_t>(now[0])+0x774+slot*0x2d8;
        const unsigned index=read<uint8_t>(buffer),count=read<uint8_t>(buffer+1);
        if(index>=count || count>16 || read<uint8_t>(buffer+2)!=owner || read<uint8_t>(buffer+3)!=serial) continue;
        const auto packet=buffer+0x18+index*0x2c;
        const auto found=packetRecord(id,static_cast<uintptr_t>(now[0]),packet);
        if(!found || !current(*found) || found->actor!=static_cast<uintptr_t>(now[0]) ||
            found->source!=owner || found->serial!=serial || found->packet!=packet || (found->spokenBuffs&(1u<<buff)) ||
            found->before[17+buff]==found->after[17+buff]) continue;
        auto old=found->after; old[17+buff]=found->before[17+buff];
        announce(id,old,found->after); found->spokenBuffs|=1u<<buff;
    }
}
void after(Event& e) {
    if(!inBattle() || !e.before[0]) return;
    const auto id=target(e); auto now=snapshot(id,static_cast<uintptr_t>(e.before[0]));
    if(e.rva==0x38e680) {
        // MsCalcDamage precomputes effects, before the action banner. Its
        // result packet is later consumed by MsApplyDamage's visible playback.
        const auto packet=e.args[6];
        auto item=packetRecord(id,static_cast<uintptr_t>(e.before[0]),packet);
        if(!item) return;
        discard(*item);
        if(now[0]==e.before[0] && !std::equal(now.begin()+1,now.begin()+25,e.before.begin()+1)) {
            *item={generation(),id,static_cast<unsigned>(e.args[0]&255),
                // 7893A0's four-word calculation counters carry the sequence
                // at word3. It assigns the target buffer header only AFTER
                // 789800/78E680 return; reading that header here is too early.
                e.args[8]?read<unsigned>(e.args[8]+12,256):256,calculatedRevision,
                static_cast<uintptr_t>(now[0]),packet,e.before,now};
            ++calculatedCount;
        }
        return;
    }
    auto old=e.before;
    std::array<Calculated,2> played{};unsigned playedCount=0,lastConsumed=2;
    if(e.rva==0x38f0b0 && e.copiedCount==28) for(unsigned slot=0;slot<2;++slot) {
        const auto packet=static_cast<uintptr_t>(e.before[26+slot]);
        if(!packet) continue;
        const auto buffer=static_cast<uintptr_t>(e.before[0])+0x774+slot*0x2d8;
        if(read<uint8_t>(buffer)!=e.copied[slot]+1) continue;
        lastConsumed=slot;
        const auto found=packetRecord(id,static_cast<uintptr_t>(now[0]),packet);
        if(!found || !current(*found) || found->actor!=static_cast<uintptr_t>(now[0]) || found->packet!=packet) continue;
        // Preserve this effect's copied deltas, even if a subsequent effect
        // has already precomputed the character's next buff count.
        for(unsigned i=1;i<25;++i) if(found->before[i]!=found->after[i]) {
            old[i]=now[i]; // This consumed packet owns that delta.
        }
        played[playedCount++]=*found;discard(*found);
    }
    if(now[0]!=e.before[0]) return;
    if(lastConsumed<2 && (now[3] || (now[1]&1))) {
        // The last consumed packet owns the ordinary status state actually
        // committed before KO/shatter cleanup. Do not turn that internal clear
        // into poison cures or ended buffs, or lose newly applied petrify.
        now[1]=(now[1]&1)|(e.before[28+lastConsumed]&~intptr_t{1});
        now[2]=e.before[30+lastConsumed];
        // 78B5A0 prepares the real Doom counter before application. 78F0B0
        // commits its extra-status flag, but KO cleanup can erase the counter.
        // Retain the copied entry counter only for a consumed Doom packet.
        if(now[2]&0x4000) now[23]=e.before[23];
        for(unsigned i=0;i<13;++i) now[4+i]=e.copied[2+lastConsumed*13+i];
        for(unsigned n=0;n<playedCount;++n) for(unsigned i=1;i<25;++i)
            if(played[n].before[i]!=played[n].after[i]) old[i]=now[i];
    }
    announce(id,old,now);
    for(unsigned i=0;i<playedCount;++i) {
        const auto& item=played[i];
        auto before=item.before;
        for(unsigned buff=0;buff<6;++buff) if(item.spokenBuffs&(1u<<buff)) before[17+buff]=item.after[17+buff];
        announce(id,before,item.after);
    }
}
}
void queueBattleActiveStats(unsigned id,uintptr_t actor) {
    StatusPacket p;p.after=snapshot(id,actor);if(!p.after[0]) return;
    p.epoch=generation();p.kind=1;copyReadyBattleName(p.name,id);prepareCopiedControlSnapshot(p.controls);
    const auto ram=actor+0x540;
    // +1A4/+1A8 are the action calculator's scratch totals. They can retain
    // pre-Regen values or uncapped healing forecasts. Original79ADE0/79AE60
    // read the committed +90/+94 values used by the native Change renderer.
    // +54/+58 are effective battle maxima, including native capacity changes;
    // the base maxima at+5C/+60 are not interchangeable with them.
    p.vitals={read<int>(ram+0x90),read<int>(ram+0x54),read<int>(ram+0x94),read<int>(ram+0x58)};
    p.maximumHp=setting(Setting::ShowMaximumHp)!=0;p.maximumMp=setting(Setting::ShowMaximumMp)!=0;
    speechq::captureDomain(speechq::BattleStatusDomain,p,p.epoch);
}
void resetBattleStatus() {++calculatedRevision;calculatedCount=0;}
void installBattleStatus() {
    speechq::registerDomain(speechq::BattleStatusDomain,processStatus,nullptr);
    // Calculation owns copied early deltas. Qualified target creation can
    // release a stack buff; native packet playback owns the general fallback.
    hook(0x38e680,before,after);
    hook(0x38f0b0,before,after);
    hook(0x39a1a0,before,after);
    // Qualified target creation supplies timing; actual calculated deltas and
    // the current source/serial/result packet still own the spoken content.
    hook(0x395a30,animation,nullptr);
}
}
