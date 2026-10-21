#include "sphere_text.hpp"
#include "ready_game_text.hpp"
#include "events.hpp"
#include "profile.hpp"
#include "settings.hpp"
#include "sphere_grid.hpp"
#include "sphere_jump.hpp"
#include "status_review.hpp"
#include <atomic>

namespace ffx {
namespace {
int previousNode = -1, previousCharacter = -1, inputDepth = 0;
std::atomic<bool> visible{false};
std::atomic<unsigned> visibleEpoch{0};
bool keyContext() { return visible.load() && visibleEpoch.load()==generation() && !statusReviewAvailable(); }
using Key = std::array<intptr_t, 32>;
Key previousList{};
bool opened() { return sphereGridOpen(); }
void rememberContext() { visible=opened(); visibleEpoch=generation(); }
void announceCharacter() {
    const auto engine=read<uintptr_t>(address(0x1f05834));if(!engine) return;
    previousCharacter=read<uint8_t>(engine+0x115bc);
    TextPlan p;prepareTextPlan(p,"sphere-character");p.headline=planPlayerName(p,previousCharacter);commitTextPlan(p);
}
void announceNode() {
    const auto engine=read<uintptr_t>(address(0x1f05834));if(!engine) return;
    const int index=read<int16_t>(engine+0x112fc,-1);if(index<0 || index>=read<int16_t>(engine+2) || index==previousNode) return;
    TextPlan p;prepareTextPlan(p,"sphere-node");const auto node=planSphereNode(p,index,true);
    auto line=planPair(p,TextPlanOp::Or,node.name,planLiteral(p,L"Unnamed node"));const auto character=read<uint8_t>(engine+0x115bc);
    if(character<7 && (node.activated&(1u<<character))) line=planConcat(p,line,planConcat(p,planLiteral(p,L", activated for "),planPlayerName(p,character)));
    else if(node.sphere) {const auto name=planRecordName(p,0x390a40,node.sphere);line=planConcat(p,line,planPair(p,TextPlanOp::IfText,name,planConcat(p,planLiteral(p,L", requires 1 "),name)));}
    p.headline=planConcat(p,line,planConcat(p,planLiteral(p,L", node "),planInteger(p,index+1)));
    planSelectionDetail(p,node.description,setting(Setting::AutoDetail)!=0);previousNode=index;commitTextPlan(p);
}
void openingBefore(Event& e) { e.before[0] = opened(); }
void openingAfter(Event& e) {
    rememberContext();
    if (!e.before[0] && opened()) { previousNode = previousCharacter = -1; announceCharacter(); announceNode(); }
}
void closing(Event&) { rememberContext(); if (!opened()) { previousNode = previousCharacter = -1; resetSelectionDetail(); resetSpherePlanner(); } }
void nodeSelected(Event&) { rememberContext(); if (opened()) announceNode(); }
void characterSelected(Event&) {
    if(statusReviewAvailable()) return;
    const auto engine = read<uintptr_t>(address(0x1f05834));
    if (opened() && engine && read<uint8_t>(engine + 0x115bc) != previousCharacter) { announceCharacter(); previousNode=-1; announceNode(); }
}
void levels(bool) {
    if(!opened()) return;TextPlan p;prepareTextPlan(p,"sphere-levels");using Saved=uintptr_t(__cdecl*)(unsigned);
    for(int i=0;i<7;++i) {const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(i);if(saved) p.headline=planJoin(p,p.headline,planConcat(p,planPlayerName(p,i),planConcat(p,planLiteral(p,L" "),planInteger(p,read<uint8_t>(saved+0x3b)))));}
    commitTextPlan(p);
}
Key selected() {
    Key k{};
    auto pool = read<uintptr_t>(address(0x1686108));
    if (!pool) return k;
    int slot = read<int16_t>(pool + 0x2700, -1);
    const auto callbackPointer=read<uintptr_t>(pool+0x2704);
    const auto callback=logicalRva(callbackPointer);
    const bool movement=slot==11 && callbackPointer==address(0x656160);
    if (!((slot == 7 && callback == 0x656330) || (slot == 8 && callback == 0x6560d0) ||
          (slot == 11 && (callback == 0x656060 || movement)))) return k;
    auto win = pool + slot * 0x340;
    int row = read<int16_t>(win + 0x18, -1), count = read<int16_t>(win + 0x1e);
    if (read<uint8_t>(win + 0x23) != 1 || count <= 0 || count > 64 || row < 0 || row >= count) return k;
    k[0] = pool; k[1] = slot; k[2] = row; k[4]=movement?9:slot==11?10:-1;
    auto entry = win + 0x40 + row * 12;
    k[3] = slot == 8 ? read<uint16_t>(entry + 8) : read<uintptr_t>(entry);
    return k;
}
void selectionBefore(Event& e) { e.before = selected(); if (e.rva == 0x6583e0) ++inputDepth; }
void selectionAfter(Event& e) {
    if (e.rva == 0x6583e0) --inputDepth;
    const auto k = selected();
    if(inputDepth) return;
    if(!k[0]) { previousList={}; return; }
    if(k==previousList) return;
    const bool opening=k[0]!=previousList[0] || k[1]!=previousList[1] || k[4]!=previousList[4];
    previousList=k;
    const bool confirmation=k[4]>=0;
    TextPlan p;prepareTextPlan(p,confirmation && opening?"sphere-confirmation-prompt":"sphere-list",!(confirmation && opening));
    const auto name=k[1]==8?planRecordName(p,0x390a40,static_cast<unsigned>(k[3])):planEncoded(p,k[3]);
    const auto line=k[1]==8?planCountedName(p,name,quantity(static_cast<unsigned>(k[3]))):name;
    if(confirmation && opening) {p.interrupt=true;p.headline=planEncoded(p,read<uintptr_t>(k[0]+k[4]*0x340+0x40));addTextPlanOutput(p,line,"sphere-list",false);}
    else p.headline=line;
    if(k[1]==8) {
        const auto entry=k[0]+k[1]*0x340+0x40+k[2]*12;
        if(read<uint8_t>(entry+4)) addTextPlanOutput(p,planLiteral(p,L"Unavailable"),"sphere-availability",false);
        planSelectionDetail(p,planSphereItemDescription(p,static_cast<unsigned>(k[3])),setting(Setting::AutoDetail)!=0);
    }else if(confirmation) resetSelectionDetail();
    else planSelectionDetail(p,planMenuDescription(p,9,k[2]==0?0x1f:0x1e),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
void preparingMovement(Event& e) {
    if(!opened() || e.args[0]>=7) return;
    const auto engine=read<uintptr_t>(address(0x1f05834));
    if(!engine || read<uintptr_t>(engine+0x115b0)) return;
    const int count=read<int16_t>(engine+2),destination=static_cast<int16_t>(e.args[1]);
    const int source=read<int16_t>(engine+0x110cc+e.args[0]*0x50,-1);
    if(count<1 || count>1024 || source<0 || source>=count || destination<0 || destination>=count || source==destination) return;
    // 648C80 starts the native character/path preview before the Yes/No
    // prompt. Long paths can take seconds here; no timer or distance guess.
    output(L"Preparing to move",true,"sphere-preparing-movement");
}
void movementBefore(Event& e) {
    if(e.args[2]!=0 || !e.args[3] || !opened()) return;
    const auto engine=read<uintptr_t>(address(0x1f05834));
    if(!engine) return;
    const unsigned character=read<uint8_t>(engine+0x115bc,255);
    const int source=read<int16_t>(engine+0x11630,-1),count=read<int16_t>(engine+2);
    if(character>=7 || count<1 || count>1024 || source<0 || source>=count) return;
    e.before[0]=engine; e.before[1]=character; e.before[2]=source;
    using Saved=uintptr_t(__cdecl*)(unsigned);
    const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(character);
    if(saved) { e.before[3]=read<uint8_t>(saved+0x3b); e.before[4]=1; }
}
void movementAfter(Event& e) {
    const auto engine=read<uintptr_t>(address(0x1f05834));
    if(!e.before[0] || engine!=static_cast<uintptr_t>(e.before[0])) return;
    const unsigned character=static_cast<unsigned>(e.before[1]);
    const int destination=read<int16_t>(engine+0x110cc+character*0x50,-1);
    if(destination<0 || destination>=read<int16_t>(engine+2) || destination==e.before[2]) return;
    TextPlan p;prepareTextPlan(p,"sphere-movement");const auto node=planSphereNode(p,destination);if(!node.valid) return;p.gate=node.name;
    auto line=planConcat(p,planPlayerName(p,character),planConcat(p,planLiteral(p,L" moved to "),node.name));
    line=planConcat(p,line,planConcat(p,planLiteral(p,L", node "),planInteger(p,destination+1)));
    using Saved=uintptr_t(__cdecl*)(unsigned);const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(character);
    if(saved && e.before[4]) {const auto levels=read<uint8_t>(saved+0x3b);if(levels!=e.before[3]) {
        line=planConcat(p,line,planConcat(p,planLiteral(p,L". Sphere Levels "),planInteger(p,e.before[3])));
        line=planConcat(p,line,planConcat(p,planLiteral(p,L" to "),planInteger(p,levels)));}}
    p.headline=line;planSelectionDetail(p,node.description,false);commitTextPlan(p);
}
void activationBefore(Event& e) {
    const auto engine=read<uintptr_t>(address(0x1f05834));
    if(!engine || e.args[0]>=7 || e.args[1]>=static_cast<unsigned>(read<int16_t>(engine+2))) return;
    e.before[0]=read<uint8_t>(engine+0x829+e.args[1]*0x28);
    e.before[1]=1;
    using Saved=uintptr_t(__cdecl*)(unsigned);
    using Stat=unsigned(__cdecl*)(uintptr_t,uintptr_t,int);
    const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(static_cast<unsigned>(e.args[0]));
    if(saved) {
        e.before[12]=1;
        for(unsigned row=0;row<10;++row)
            e.before[2+row]=reinterpret_cast<Stat>(address(0x4cec70))(saved,address(0x85a608)+row*0x1c,1);
    }
}
void activationAfter(Event& e) {
    if(!e.before[1]) return;
    const auto engine=read<uintptr_t>(address(0x1f05834));
    if(!engine || !(read<uint8_t>(engine+0x829+e.args[1]*0x28)&(1u<<e.args[0])) ||
        (e.before[0]&(1u<<e.args[0]))) return;
    TextPlan p;prepareTextPlan(p,"sphere-activation",false);const auto node=planSphereNode(p,static_cast<int>(e.args[1]));
    auto line=planConcat(p,planPlayerName(p,static_cast<int>(e.args[0])),planConcat(p,planLiteral(p,L": "),planConcat(p,node.name,planLiteral(p,L" activated"))));
    using Saved=uintptr_t(__cdecl*)(unsigned);using Stat=unsigned(__cdecl*)(uintptr_t,uintptr_t,int);
    const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(static_cast<unsigned>(e.args[0]));
    if(saved && e.before[12]) for(unsigned row=0;row<10;++row) {
        const auto entry=address(0x85a608)+row*0x1c;const auto value=reinterpret_cast<Stat>(address(0x4cec70))(saved,entry,1);
        if(value!=static_cast<unsigned>(e.before[2+row])) {
            auto change=planConcat(p,planMenuText(p,12,read<unsigned>(entry+4)),planConcat(p,planLiteral(p,L" "),planInteger(p,e.before[2+row])));
            change=planConcat(p,change,planConcat(p,planLiteral(p,L" to "),planInteger(p,value)));line=planConcat(p,line,planConcat(p,planLiteral(p,L". "),change));
        }
    }
    p.headline=line;commitTextPlan(p);
}
}
void rememberSphereCharacter(unsigned id) { previousCharacter=static_cast<int>(id); previousNode=-1; }
bool readSphereDescription() {
    if(!sphereGridOpen() || statusReviewAvailable()) return false;
    const auto k=selected();
    TextPlan p;prepareTextPlan(p,"sphere-description");TextRef description=0;
    if(k[0]) {
        if(k[4]>=0) description=planEncoded(p,read<uintptr_t>(k[0]+k[4]*0x340+0x40),1024,TextPlanOp::Lines);
        else description=k[1]==8?planSphereItemDescription(p,static_cast<unsigned>(k[3])):planMenuDescription(p,9,k[2]==0?0x1f:0x1e);
    }else {const auto engine=read<uintptr_t>(address(0x1f05834));if(engine) description=planSphereNode(p,read<int16_t>(engine+0x112fc,-1)).description;}
    p.headline=planPair(p,TextPlanOp::Or,description,planLiteral(p,L"No native description is available for this selection."));commitTextPlan(p);
    return true;
}
void prepareSphereRetirement();
void installSphereGrid() {
    installSphereJump();
    prepareSphereRetirement();prepareSphereText();
    hook(0x653570, openingBefore, openingAfter);
    for (auto rva : {0x653de0, 0x654b40, 0x656060}) hook(rva, nullptr, closing);
    hook(0x648d70, nullptr, nodeSelected); hook(0x4a9810, nullptr, characterSelected);
    for (auto rva : {0x6583e0, 0x648e40, 0x659950, 0x6598a0}) hook(rva, selectionBefore, selectionAfter);
    hotkeyWhen('S',levels,keyContext);
    hotkeyWhen(VK_F2,openSpherePathMenu,keyContext); hotkeyWhen(VK_F3,openSpherePlanner,keyContext);
    hotkeyWhen(VK_F4,[](bool) { sphereSectionKey(false); },keyContext); hotkeyWhen(VK_F5,[](bool) { sphereSectionKey(true); },keyContext);
    hotkeyWhen('O',sphereOrientationKey,keyContext);
    hook(0x648910,activationBefore,activationAfter);
    hook(0x656160,movementBefore,movementAfter);
    hook(0x648c80,preparingMovement,nullptr);
}
void resetSphereGrid() { resetSphereJump();visible=false; previousNode = previousCharacter = -1; inputDepth = 0; previousList={}; invalidateSphereText();clearSphereTargets(); }
}
