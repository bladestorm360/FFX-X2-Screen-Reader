// Read the Overkill word only on its first accepted visible sprite submission.
#include "events.hpp"
#include "game_visual_text.hpp"
#include "ready_game_text.hpp"
#include <array>
namespace ffx {
namespace {
struct Popup { uintptr_t pool=0; unsigned epoch=0,owner=31; bool revealed=false; };
struct Draw { unsigned slot=0; int x=0,y=0; };
std::array<Popup,128> popups;
std::array<Draw,128> frame{};
unsigned frameCount=0;
size_t cursor=0;
int slotOf(uintptr_t pool,uintptr_t node) {
    return pool>=0x10000 && node>=pool && node<pool+128*20 && (node-pool)%20==0?
        static_cast<int>((node-pool)/20):-1;
}
void registeredBefore(Event& e) { e.before[0]=read<uintptr_t>(address(0xd360b8)); }
void registeredAfter(Event& e) {
    if(e.result!=1 || !e.before[0]) return;
    const auto pool=read<uintptr_t>(address(0xd360b0));
    const auto node=read<uintptr_t>(static_cast<uintptr_t>(e.before[0]));
    const int slot=slotOf(pool,node);
    if(slot<0) return;
    popups[slot]={}; // Every native allocation starts a distinct lifetime.
    if(e.args[2]!=61 || e.args[8]!=2 || (e.args[0]&255)>=31) return;
    popups[slot]={pool,generation(),static_cast<unsigned>(e.args[0]&255),false};
}
void renderBefore(Event&) {
    frameCount=0; cursor=0;
    if(!inBattle()) return;
    const auto pool=read<uintptr_t>(address(0xd360b0));
    auto node=read<uintptr_t>(address(0xd360b4));
    std::array<bool,128> visited{};
    using Actor=uintptr_t(__cdecl*)(unsigned);
    for(unsigned n=0;node && n<128;++n) {
        const int slot=slotOf(pool,node);
        if(slot<0 || visited[slot]) break;
        visited[slot]=true;
        const unsigned actorId=read<uint8_t>(node+2);
        if(actorId<31 && read<uint8_t>(node) && read<uint8_t>(node+1)==2 && read<uint8_t>(node+4)==61 && !read<uint8_t>(node+5)) {
            const auto actor=reinterpret_cast<Actor>(address(0x394030))(actorId);
            if(actor) {
                const bool alternate=(read<uint8_t>(node)&0x80)!=0;
                const int x=read<int>(actor+(alternate?0xf4c:0xf44))+read<int16_t>(node+8);
                const int y=read<int>(actor+(alternate?0xf50:0xf48))+read<int16_t>(node+10)-(read<int8_t>(node+6)-1);
                if(x+read<uint8_t>(node+12)>=0 && x<0x201 && y+read<uint8_t>(node+13)>=0 && y<0x1a1)
                    if(frameCount<frame.size()) frame[frameCount++]={static_cast<unsigned>(slot),x,y};
            }
        }
        node=read<uintptr_t>(node+16);
    }
}
void rendered(Event& e) {
    if(e.parentRva!=0x39f680 || e.args[2]!=2 || e.args[3]!=61 || cursor>=frameCount) return;
    // Native renderer traverses the same list in order. Include already revealed
    // records so overlapping popups still retain their own native allocation.
    const auto draw=frame[cursor++];
    if(draw.x!=static_cast<int16_t>(e.args[0]) || draw.y!=static_cast<int16_t>(e.args[1])) return;
    auto& owner=popups[draw.slot];
    if(owner.revealed || owner.owner>=31 || owner.pool!=read<uintptr_t>(address(0xd360b0)) || owner.epoch!=generation() || e.result==static_cast<uintptr_t>(-1) || !(e.args[7]&255)) return;
    const auto label=battleGraphicLabel(BattleGraphic::Overkill);
    if(!*label) return;
    owner.revealed=true;
    TextPlan p;prepareTextPlan(p,"battle-overkill",false);
    p.headline=planConcat(p,planConcat(p,planBattleName(p,owner.owner),planLiteral(p,L": ")),
        planLiteral(p,label));
    commitTextPlan(p);
}
void renderAfter(Event&) { frameCount=0; cursor=0; }
}
void resetBattleGraphics() { popups={}; frameCount=0; cursor=0; }
void installBattleGraphics() {
    hook(0x39fc80,registeredBefore,registeredAfter);
    hook(0x39f680,renderBefore,renderAfter);
    hook(0x4999b0,nullptr,rendered);
}
}
