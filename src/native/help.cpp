// Observe only ready text accepted by the native Help renderer.
#include "events.hpp"
#include "status_review.hpp"
namespace ffx {
bool monsterArenaContext();
void installMonsterArena();
void resetHelp();
namespace {
bool pending=false;
// Native event identity, never a comparison of text content. A page can be
// replaced by the update script or its caller without an input result.
std::array<uintptr_t,7> lastPage{};
struct Frame {StatusBatch batch;unsigned parts=0,entries=0;uintptr_t focused=0,source=0,entry=0,start=0,selected=0;bool active=false,needsDiagram=false,diagramSeen=false;};
thread_local Frame frame;
thread_local unsigned drawDepth=0;
bool isHelp() {return (scene==11 || read<int>(address(0x8416b0))==11) && !monsterArenaContext();}
uintptr_t source(uintptr_t entry) {
    if(!entry || read<uint8_t>(entry,255)!=0) return 0;
    int index=read<int16_t>(entry+0x16,-1);
    const int variable=read<int16_t>(entry+0x18,-1);
    if(variable>=0) {
        const auto vars=read<uintptr_t>(address(0xd2a0a4));
        if(variable>=4096 || !vars) return 0;
        index+=read<int>(vars+variable*4);
    }
    const auto table=read<uintptr_t>(address(0xd2a024)),strings=read<uintptr_t>(address(0xd2a040));
    if(!table || !strings || index<0 || index>=0x10000) return 0;
    return strings+read<int>(table+index*4);
}
void arm() {pending=isHelp();}
void initialize(Event&) {arm();}
void inputAfter(Event& e) {
    if(!isHelp()) {resetHelp();return;}
    const int result=read<int>(e.args[0]);
    // Confirm can initialize a child while still inside this dispatcher.
    // Its completed Help context must survive the outer return.
    if(result==1 || result==2) arm();
}
void drawing(Event& e) {
    e.before[0]=++drawDepth;
    if(drawDepth==1) frame.active=false;
    if(drawDepth!=1 || !isHelp()) return;
    const auto pages=read<uintptr_t>(address(0xd2a01c));
    const int page=read<int>(address(0xd2a088),-1),count=read<int>(address(0xd2a080));
    if(!pages || page<0 || page>=count || count>4096) return;
    const auto entry=read<uintptr_t>(address(0xd2a084)),base=read<uintptr_t>(address(0xd2a03c));
    const std::array<uintptr_t,7> identity{generation(),pages,static_cast<uintptr_t>(page),entry,base,
        read<uintptr_t>(address(0xd2a040)),static_cast<uintptr_t>(read<int>(address(0x8416b0)))};
    const bool opening=identity[0]!=lastPage[0] || identity[1]!=lastPage[1] || identity[2]!=lastPage[2] ||
        identity[4]!=lastPage[4] || identity[5]!=lastPage[5] || identity[6]!=lastPage[6];
    if(identity!=lastPage) {lastPage=identity;pending=true;}
    if(!pending) return;
    const int entries=read<int16_t>(pages+page*8+4,-1);
    if(entries<0 || entries>512) return;
    if(opening) closeHelpReview();
    frame={};frame.batch=beginStatusRows(0,page,false,true);
    frame.batch.help=true;frame.batch.present=true;
    frame.batch.helpOpening=opening;frame.entries=static_cast<unsigned>(entries);
    frame.start=base+read<unsigned>(pages+page*8);frame.selected=entry;
    frame.batch.focusRow=~0u;
    if(page>=5 && page<=7) {using Locale=unsigned(__cdecl*)();frame.needsDiagram=reinterpret_cast<Locale>(address(0x241290))()==1;}
    frame.focused=source(entry);
    // 776220 follows these exact relative directional links. Preserve native
    // menu/page navigation; buffer arrows own only axes with no native link.
    if(entry) for(unsigned direction=0;direction<4;++direction) {
        const auto offset=read<int>(entry+0x38+direction*12);
        const auto destination=base+offset;
        // Offset0 points to the SPS2 header, never a native page entry.
        // 776220 searches for a real entry before accepting a transition.
        if(offset && destination && destination!=entry) frame.batch.nativeArrows|=direction<2?1u:2u;
    }
    frame.batch.helpMenu=frame.batch.nativeArrows!=0;
    frame.active=frame.batch.valid;e.before[1]=frame.active;
}
void decoded(Event& e) {
    // 76F580 executes page scripts before drawing. Its F70B opcode changes
    // the selected entry and sets *update=1; the outer update then resolves
    // that entry's page. This transition bypasses 776220's result entirely.
    // Arm only the accepted native mutation, never its unrendered text.
    if(e.epoch==generation() && isHelp() && e.parentRva==0x36f580 &&
        e.args[3] && !e.args[4] && read<int>(e.args[3])==1) {arm();return;}
    if(drawDepth!=1 || !frame.active || !pending || !isHelp() ||
       e.epoch!=generation() || e.parentRva!=0x36f960 || e.args[3] || !e.args[4]) return;
    frame.source=e.args[1];
    frame.entry=e.edi>=frame.start && (e.edi-frame.start)%0x138==0 &&
        (e.edi-frame.start)/0x138<frame.entries?e.edi:0;
}
void rendered(Event& e) {
    // Kind5 (ordinary Help) has no stack output-buffer argument: space,
    // text,x,y,... versus output-buffer,space,text,x,y,... in other fonts.
    const unsigned text=e.rva==0x4e75c0?1:2;
    if(drawDepth!=1 || !frame.active || !pending || !isHelp() ||
       e.epoch!=generation() || e.parentRva!=0x36f960 || !e.args[text])return;
    // The original page includes invisible control/sound entries, including
    // Dummy at(-17,-55). Observe final native text coordinates, not raw list
    // selection, and copy only the string actually submitted to the font draw.
    const int x=static_cast<int>(e.args[text+1]),y=static_cast<int>(e.args[text+2]);
    if(x<0 || y<0 || x>=512 || y>=416)return;
    if(frame.parts>=512) {frame.batch.valid=false;return;}
    const bool selected=frame.entry?frame.entry==frame.selected:frame.focused && frame.source==frame.focused;
    if(selected) {frame.batch.focusRow=frame.parts;if(frame.entry) frame.batch.helpMenu=true;}
    TextPlan p;prepareTextPlan(p,"help-buffer",false);
    p.context[2]=x;p.context[3]=y;p.context[4]=1;
    const int id=frame.entry?read<int16_t>(frame.entry+0x16,-1):-1;
    // Original SPS2 control-bar IDs56..61 are all separate native entries at
    // y356/357. Capture only their accepted font draw, never infer from words.
    if(id>=56 && id<=61 && y>=350) p.context[5]=HelpControls;
    else if(y<64) p.context[5]=HelpPrompt;
    else if(frame.batch.page==24 && id>=119 && id<=125) p.context[5]=HelpCharacter;
    auto ready=e.args[text];unsigned limit=4096;
    // 776A50 preserves13/30..37; the native font8E9B80 resolves it with
    // 8B6610(space,token). Unknown slots draw literal??? and never enter here.
    if(p.context[5]==HelpCharacter && read<uint8_t>(ready)==0x13 && read<uint8_t>(ready+1)-0x30u<8) {
        using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t);
        ready=reinterpret_cast<Getter>(address(0x4b6610))(static_cast<unsigned>(e.args[text-1]),ready);limit=40;
    }
    auto textRow=planEncoded(p,ready,limit,TextPlanOp::LineBreaks);
    if(p.context[5]==HelpCharacter) textRow=planPair(p,TextPlanOp::Or,textRow,planLiteral(p,L"Empty slot"));
    addStatusRow(frame.batch,p,textRow,0,true);
    ++frame.parts;
}
void diagram(Event& e) {
    if(drawDepth!=1 || !frame.active || !pending || !isHelp() || e.epoch!=generation() ||
       e.parentRva!=0x36f960 || !e.args[4] || !e.edi)return;
    // Qualified76F960 keeps the current native entry in EDI at774630.
    // The accepted image identities are from the original English pages.
    if(read<uint8_t>(e.edi)!=6 || read<int16_t>(e.edi+0x14)!=48 ||
       read<int16_t>(e.edi+0x18)!=-1)return;
    const int image=read<int16_t>(e.edi+0x16);
    const wchar_t* labels=nullptr;
    if(frame.batch.page==5 && image==19) labels=
        L"Normal. Options: Pause game (disabled in some areas). "
        L"Directional pad or left stick: Move character. Triangle: Menu. "
        L"Circle: Hold down to walk / cancel. Cross: Talk / Examine / Confirm.";
    else if(frame.batch.page==6 && image==30) labels=
        L"L1: Display switch window / Select character to switch. "
        L"L2: Select character to switch. R1 / R2: Scroll CTB window. "
        L"Triangle: Defend (pass turn). Circle: Cancel. Cross: Confirm. "
        L"Directional pad / left stick: Move cursor / Change command windows. "
        L"Touch pad: Display / Hide help window.";
    else if(frame.batch.page==7 && image==29) labels=
        L"L1 / R1: Switch characters. L2 / R2: Scroll lists. "
        L"Directional pad / left stick: Move cursor. Circle: Cancel. Cross: Confirm.";
    if(!labels)return;
    const int x=static_cast<int>(e.args[0]),y=static_cast<int>(e.args[1]);
    if(x<0 || y<0 || x>=512 || y>=416 || frame.parts>=512)return;
    using Locale=unsigned(__cdecl*)();if(reinterpret_cast<Locale>(address(0x241290))()!=1)return;
    TextPlan p;prepareTextPlan(p,"help-diagram",false);p.context[2]=x;p.context[3]=y;p.context[4]=1;
    // Exact visible English asset labels and the controller symbols they point
    // to, not guessed keyboard bindings or additional gameplay advice.
    addStatusRow(frame.batch,p,planLiteral(p,labels));
    frame.diagramSeen=true;
    ++frame.parts;
}
void drawn(Event& e) {
    if(drawDepth) --drawDepth;
    if(e.before[0]!=1 || !e.before[1] || !frame.parts || !pending || !isHelp()) return;
    if(frame.needsDiagram && !frame.diagramSeen)return; // Wait for the original image to be ready.
    if(finishStatusRows(frame.batch)) pending=false;
}
}
bool helpMenuContext() {return isHelp();}
void helpSceneChanged() {if(isHelp()) arm();else resetHelp();}
void resetHelp() {pending=false;lastPage={};closeHelpReview();}
void installHelp() {
    installMonsterArena();
    hook(0x375df0,nullptr,initialize);hook(0x375ef0,nullptr,initialize);
    hook(0x376220,nullptr,inputAfter);hook(0x376a50,nullptr,decoded);
    hook(0x4e52e0,nullptr,rendered);
    hook(0x4e75c0,nullptr,rendered);hook(0x4e52b0,nullptr,rendered);hook(0x4ee210,nullptr,rendered);
    hook(0x374630,nullptr,diagram);
    hook(0x36f960,drawing,drawn);
}
}
