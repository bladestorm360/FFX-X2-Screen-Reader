// Read the accepted native expanded Monster Arena menu, rather than raw script
// strings whose conditional tokens may hide/reorder rows. Capture prompt, focus
// and instructions separately at the outer draw; pending generation/context
// guards prevent an unrelated message from completing the menu announcement.
// See Docs/features/menus/index.md#system-screens.
// Native conditional UI is expanded and accepted by 76F960 before capture.
#include "events.hpp"
#include "copied_text.hpp"
#include <atomic>
namespace ffx {
bool monsterArenaContext() {
    const auto kind=read<int>(address(0x8416b0));
    return scene==11 && (kind==8 || kind==11) && read<uint16_t>(address(0xd2ca90)+0xba)==307;
}
namespace {
bool pending=false,opening=false;
unsigned armedEpoch=0;
struct Frame { CopiedTextPacket packet; uintptr_t focus=0,title=0,instructions[4]{}; bool captured=false,active=false; };
thread_local Frame frame;
thread_local unsigned drawDepth=0;
uintptr_t indexedSource(int index) {
    const auto table=read<uintptr_t>(address(0xd2a024)),base=read<uintptr_t>(address(0xd2a040));
    if(!table || !base || index<0 || index>=0x10000) return 0;
    return base+read<unsigned>(table+index*4);
}
uintptr_t focusedSource() {
    const auto entry=read<uintptr_t>(address(0xd2a084));
    if(!entry || read<uint8_t>(entry,255)!=0) return 0;
    int index=read<int16_t>(entry+0x16,-1);
    const int variable=read<int16_t>(entry+0x18,-1);
    if(variable>=0) {
        const auto values=read<uintptr_t>(address(0xd2a0a4));
        if(!values || variable>=4096) return 0;
        index+=read<int>(values+variable*4);
    }
    return indexedSource(index);
}
void arm(bool first) {
    pending=monsterArenaContext(); opening=first; armedEpoch=generation();
    if(pending) resetSelectionDetail();
}
void initialized(Event&) { if(monsterArenaContext()) arm(true); }
void inputBefore(Event& e) { e.before[0]=read<int>(address(0xd2a088)); }
void inputAfter(Event& e) {
    if(!monsterArenaContext()) { pending=false; return; }
    const auto result=read<int>(e.args[0]);
    if(result==1) arm(e.before[0]!=read<int>(address(0xd2a088)));
    else if(result==2) { pending=false; resetSelectionDetail(); }
}
void drawing(Event& e) {
    e.before[0]=++drawDepth;
    if(drawDepth==1) frame.active=false;
    if(drawDepth!=1 || !nativeGameThread() || !pending || armedEpoch!=generation() || !monsterArenaContext()) return;
    frame={}; frame.active=true; prepareCopiedText(frame.packet); frame.focus=focusedSource();
    frame.title=opening && read<int>(address(0xd2a088))==0?indexedSource(12):0;
    for(unsigned i=0;i<4;++i) frame.instructions[i]=indexedSource(39+i);
    e.before[1]=1;
}
void expanded(Event& e) {
    // Nested text expansion is not a new menu frame. Match source identity to
    // the focused row/title/footer before copying the fixed ready native buffer.
    if(drawDepth!=1 || !frame.active || !pending || armedEpoch!=e.epoch || !monsterArenaContext() ||
        e.parentRva!=0x36f960 || e.args[3] || !e.args[4] || !read<uint8_t>(e.args[0])) return;
    unsigned part=3;
    if(e.args[1]==frame.focus) part=1;
    else if(frame.title && e.args[1]==frame.title) part=0;
    else for(auto source:frame.instructions) if(e.args[1]==source) { part=2; break; }
    if(part>=3 || frame.packet.length[part]) return;
    // Native buffer is 0x4000 bytes. Copy a fixed ready prefix, never its pointer.
    if(copy(e.args[0],frame.packet.text[part].data(),frame.packet.text[part].size())) {
        frame.packet.length[part]=static_cast<unsigned>(frame.packet.text[part].size());
        if(part<2) frame.captured=true;
    }
}
void drawn(Event& e) {
    if(drawDepth) --drawDepth;
    if(e.before[0]!=1 || !e.before[1] || !frame.captured || !monsterArenaContext()) return;
    if(commitCopiedText(frame.packet)) { pending=false; opening=false; }
}
}
void installMonsterArena() {
    hook(0x375df0,nullptr,initialized); hook(0x375ef0,nullptr,initialized);
    hook(0x376220,inputBefore,inputAfter); hook(0x376a50,nullptr,expanded);
    hook(0x36f960,drawing,drawn);
}
}
