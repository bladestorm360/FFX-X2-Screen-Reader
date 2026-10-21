// Native save/load window15: copied state at its own input mutations.
// Airship choices are the distinct hiku2100 field interface.
#include "events.hpp"
#include "settings.hpp"
#include "profile.hpp"
#include "ready_game_text.hpp"
namespace ffx {
namespace {
using Key=std::array<intptr_t,32>;
Key mainKey(uintptr_t root) {
    Key k{};
    const auto popup=read<uintptr_t>(address(0x146621c));
    if(scene!=15 || !root || !popup || read<int>(root+0x1c)!=10 ||
        read<uint8_t>(popup+0x41) || read<uintptr_t>(popup+0x58)!=root ||
        read<uintptr_t>(popup+0x10)!=address(0x4b3810)) return k;
    const auto row=read<unsigned>(address(0x1466224),2);
    if(row>1) return k;
    k[0]=popup; k[1]=row; k[2]=read<uint8_t>(address(0x1466228)+row); k[9]=1;
    return k;
}
void mainBefore(Event& e) { e.before=mainKey(e.args[0]); }
void mainAfter(Event& e) {
    const auto k=mainKey(e.args[0]);
    if(!k[9]) { if(e.before[9]) resetSelectionDetail(); return; }
    if(k==e.before) return;
    TextPlan p;prepareTextPlan(p,"save-load-menu");
    auto text=planMenuText(p,3,0x2001+static_cast<unsigned>(k[1]));
    if(e.before[0]!=k[0]) {
        const auto title=planMenuText(p,3,0x2000);
        text=planPrefix(p,title,text,L". ");
    }
    if(!k[2]) text=planConcat(p,text,planLiteral(p,L". Unavailable"));
    p.headline=text;commitTextPlan(p);resetSelectionDetail();
}
Key destinationKey(uintptr_t window) {
    Key k{};
    if(scene!=15 || !window || window!=read<uintptr_t>(address(0x1466214)) ||
        read<uint8_t>(window+0x41) || read<int>(window+0x28)!=2 ||
        read<int8_t>(window+0x45)!=0 || read<uintptr_t>(window+0x10)!=address(0x4b4a00)) return k;
    const int row=read<int16_t>(window+0x48,-1),count=read<uint16_t>(window+0x30);
    if(row<0 || row>=count || count>99) return k;
    k[0]=window; k[1]=row; k[2]=read<uint8_t>(address(0x1466250)+row);
    k[3]=read<uint8_t>(address(0x146624a)); k[9]=1; return k;
}
void destinationBefore(Event& e) { e.before=destinationKey(e.args[0]); }
void destinationAfter(Event& e) {
    const auto k=destinationKey(e.args[0]);
    if(!k[9]) { if(e.before[9]) resetSelectionDetail(); return; }
    if(k==e.before) return;
    using Record=uintptr_t(__cdecl*)(int);
    using Label=uintptr_t(__cdecl*)(unsigned,uintptr_t);
    const auto record=reinterpret_cast<Record>(address(0x4b4df0))(static_cast<int>(k[1]));
    if(!record) return;
    TextPlan p;prepareTextPlan(p,"saved-game-list");
    auto text=planEncoded(p,reinterpret_cast<Label>(address(0x4b5510))(static_cast<unsigned>(k[2]),record),256);
    // Only discovered entries render this second line. Other rows draw placeholders.
    TextRef detail=0;
    if(k[2]==1) {
        using Locale=unsigned(__cdecl*)();
        using Area=uintptr_t(__cdecl*)(unsigned,unsigned);
        const auto locale=reinterpret_cast<Locale>(address(0x241290))();
        const auto language=locale==0?read<uint8_t>(record+0x1f):reinterpret_cast<Locale>(address(0x385290))();
        detail=planEncoded(p,reinterpret_cast<Area>(address(0x46c3c0))(read<uint16_t>(record+0x18),language),256);
    }
    if(e.before[0]!=k[0]) {
        const auto title=planMenuText(p,3,k[3]?0x2004:0x201f);
        text=planPrefix(p,title,text,L". ");
    }
    text=planSuffix(p,text,detail,L". ");
    const bool available=k[3]?(k[2]<3 || k[2]==4):k[2]==1;
    if(!available) text=planConcat(p,text,planLiteral(p,L". Unavailable"));
    p.headline=text;planSelectionDetail(p,detail,false);commitTextPlan(p);
}
}
void installSaveLoadMenu() {
    hook(0x4b1580,mainBefore,mainAfter);
    hook(0x4b4460,destinationBefore,destinationAfter);
}
}
