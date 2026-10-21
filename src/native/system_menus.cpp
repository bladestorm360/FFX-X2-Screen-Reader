#include "events.hpp"
#include "esc_details.hpp"
#include "settings.hpp"
#include "naming.hpp"
#include "selection_details.hpp"
#include "system_menu_capture.hpp"
#include "resource_reset.hpp"

namespace ffx {
namespace {
int previousSlot = -1, previousYesNo = -1;
bool queueCursor = false;
int previousConfig = -1, previousOption = -1;

void slotPopulation(Event& e) {
    // Only caller-owned ready bytes: no shared native reader state is accessed.
    // Preserve loading rejection; fresh post-load data survives late owner reset.
    if(e.args[0]>=200 || nativeResourceLoading()) return;
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::SlotPopulation);
    packet.index=static_cast<unsigned>(e.args[0]);
    copySystemAscii(e.args[2],packet.text[0],64);copySystemAscii(e.args[5],packet.text[1],64);
    if(packet.text[0].length || packet.text[1].length) commitSystemMenuPacket(packet);
}
void slotFocus(Event& e) {
    const int index=static_cast<int>(e.args[0]);
    if(index<0 || index>=200 || index==previousSlot) return;
    previousSlot=index;
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::SlotFocus);
    packet.index=static_cast<unsigned>(index);packet.detail=beginSelectionDetail();
    commitSystemMenuPacket(packet);
}
void popup(Event& e) {
    const auto offset=e.rva==0x302fe0?0x8eb690:0x8eb250;
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::Popup);
    copySystemAscii(address(offset),packet.text[0],256);packet.detail=beginSelectionDetail();
    commitSystemMenuPacket(packet);
    if(e.rva==0x302fe0) {queueCursor=true;previousYesNo=-1;}
}
void saveHelp(Event&) {
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::SaveHelp);
    copySystemAscii(address(0x8ebcc8),packet.text[0],256);
    copySystemAscii(address(0x8ebec8),packet.text[1],32);
    packet.detail=beginSelectionDetail();commitSystemMenuPacket(packet);
}
void yesNo(Event&) {
    const auto cursor=read<int>(address(0x8e72f0),-1);
    if(cursor<0 || cursor>1 || previousYesNo==cursor) return;
    previousYesNo=cursor;
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::YesNo);
    copySystemAscii(address(cursor==1?0x8eb890:0x8eba90),packet.text[0],64);
    packet.interrupt=!queueCursor;
    packet.automatic=!queueCursor && setting(Setting::AutoDetail)!=0;
    packet.detail=beginSelectionDetail();commitSystemMenuPacket(packet);
    queueCursor=false;
}
void esc(Event& e) {
    if(announceEscSetting(e.args[0])) return;
    resetSelectionDetail();
    const auto id=read<unsigned>(e.args[0]);
    if(!id || id>0xffff) return;
    char buffer[256]{};
    using Resolve=void(__cdecl*)(int,char*,int);
    reinterpret_cast<Resolve>(address(0x2414a0))(id,buffer,sizeof(buffer));
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::EscHelp);
    copySystemAscii(reinterpret_cast<uintptr_t>(buffer),packet.text[0],256);
    packet.detail=beginSelectionDetail();commitSystemMenuPacket(packet);
    recordLog("esc help loc=%x tags=%d/%d",id,read<int16_t>(e.args[0]+4),read<int16_t>(e.args[0]+6));
}
void loc(unsigned id,ReadySystemText& text) {
    char buffer[256]{};
    using Resolve=void(__cdecl*)(int,char*);
    reinterpret_cast<Resolve>(address(0x241480))(id,buffer);
    copySystemEncoded(reinterpret_cast<uintptr_t>(buffer),text,256);
}
uintptr_t configText(unsigned id) {
    using Language=unsigned(__cdecl*)();
    using Getter=uintptr_t(__cdecl*)(int,unsigned,unsigned);
    const auto language=reinterpret_cast<Language>(address(0x3852b0))();
    return reinterpret_cast<Getter>(address(0x38fd40))(7,id,language);
}
void config(Event& e) {
    const int phase=read<int>(e.args[0]+0x1c,-1);
    if(phase<3) {previousConfig=previousOption=-1;return;}
    if(phase!=3) return;
    const int cursor=read<uint8_t>(address(0x146a404));
    const auto count=read<int>(address(0x146a408));
    if(cursor<0 || cursor>=count || count>256) return;
    const auto array=read<uintptr_t>(address(0x146a40c));
    if(!array) return;
    const auto item=read<uintptr_t>(array+cursor*4);
    if(!item) return;
    const int option=read<int>(item+8,-1),options=read<int>(item+4);
    if(option<0 || option>=options || options>64 || (cursor==previousConfig && option==previousOption)) return;
    SystemMenuPacket packet;prepareSystemMenuPacket(packet,SystemMenuKind::Config,true);
    packet.joinLabel=cursor!=previousConfig;packet.automatic=setting(Setting::AutoDetail)!=0;
    packet.detail=beginSelectionDetail();
    if(cursor==read<int>(address(0x146a3f8),-1)) {
        loc(0x1cd,packet.text[0]);
        if(option<2) loc(option==0?0x1ce:0x1d0,packet.text[1]);
    } else {
        const auto label=read<uint16_t>(item+0x18);
        copySystemEncoded(configText(label),packet.text[0],256);
        copySystemEncoded(configText(read<unsigned>(item+0x1c+option*4)),packet.text[1],256);
        copySystemEncoded(configText(label),packet.text[2],1024);
    }
    commitSystemMenuPacket(packet);
    previousConfig=cursor;previousOption=option;
}
}
void installStatusReview();
void resetStatusReview();
void installShops();
void installConfirmations();
void installEscSettings();
void installSystemMenus() {
    installSelectionDetails();installSystemMenuCapture();
    hookSnapshot(0x302e70,slotPopulation,nullptr);hook(0x302db0,slotFocus,nullptr);
    hook(0x302fe0,nullptr,popup);hook(0x302f70,nullptr,popup);hook(0x302e00,nullptr,saveHelp);
    hook(0x2efd90,nullptr,yesNo);hook(0x2f00e0,nullptr,yesNo);hook(0x2f06f0,nullptr,yesNo);
    hook(0x28e6e0,esc,nullptr);hook(0x4cb0b0,nullptr,config);
    installStatusReview();installShops();installConfirmations();installEscSettings();
    installEscDetails();installNaming();
}
void resetSystemMenus() {
    resetSystemMenuCapture();
    previousSlot=previousYesNo=previousConfig=previousOption=-1;
    queueCursor=false;resetStatusReview();resetEscDetails();resetNaming();
}
}
