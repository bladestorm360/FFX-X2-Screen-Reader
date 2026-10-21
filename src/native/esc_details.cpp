#include "esc_details.hpp"
#include "settings.hpp"
#include "game_bindings.hpp"
#include "esc_capture.hpp"
#include <cstring>

namespace ffx {
namespace {
thread_local bool focusing=false;
thread_local int page=-1,row=-1;
bool bindingHelp=false;
uintptr_t popupOwner=0;unsigned popupEpoch=0;bool popupYes=false;
ReadyBytes<512> popupText,popupYesText,popupNoText;
void endPopup() {if(popupOwner) {popupOwner=0;resetEscCapture();(void)beginSelectionDetail();}}

void popupFocus(bool opening) {
    EscPacket packet;prepareEscPacket(packet,opening?EscCaptureKind::Popup:EscCaptureKind::QuitFocus);
    packet.text[0]=popupText;packet.text[1]=popupYes?popupYesText:popupNoText;
    packet.detail=beginSelectionDetail();commitEscPacket(packet);
}

void quitOpened(Event&) {
    // Original Main.showPopUpWithBtn defaults to No, after resolving307..309.
    popupOwner=read<uintptr_t>(address(0x8cc868));popupEpoch=generation();popupYes=false;
    copyEscLocalized(0x307,popupText);copyEscLocalized(0x308,popupYesText);copyEscLocalized(0x309,popupNoText);
    popupFocus(true);
}
bool equalNativeText(uintptr_t source,const char* expected,size_t capacity) {
    for(size_t i=0;i<capacity;++i) {char value=0;if(!copy(source+i,&value,1))return false;
        if(value!=expected[i])return false;if(!value)return true;}
    return false;
}
void parameterBefore(Event& e) {
    if(e.parentRva==0x28fca0 || !nativeGameThread() || e.epoch!=generation() ||
       !equalNativeText(e.args[1],"showPopUpWithBtn",32) || e.args[0]==0 ||
       e.args[0]!=read<uintptr_t>(address(0x8cc868))) return;
    if(!e.nativeStack) return;
    const uintptr_t tail=e.nativeStack+4+12*4;
    // Values are passed by value: these argument words ARE the type tags.
    // Treating tag5 as a pointer would reject every real native invocation.
    if(e.args[2]!=5 || e.args[6]!=5 || e.args[10]!=5) return;
    const unsigned promptLength=static_cast<unsigned>(e.args[5]),yesLength=static_cast<unsigned>(e.args[9]);
    const unsigned noLength=read<unsigned>(tail+4);
    if(!promptLength || promptLength>512 || !yesLength || yesLength>512 || !noLength || noLength>512) return;
    e.before[0]=static_cast<intptr_t>(read<uintptr_t>(tail));
    e.before[1]=1; // Qualification marker only; borrowed pointers stay on this native call.
}
void parameterAfter(Event& e) {
    if(e.before[1]!=1 || !nativeGameThread() || e.epoch!=generation() ||
       e.args[0]!=read<uintptr_t>(address(0x8cc868)) || e.args[0]==0) return;
    ReadyBytes<512> prompt,yes,no;
    copyReadyAscii(e.args[4],prompt,512);copyReadyAscii(e.args[8],yes,512);
    copyReadyAscii(static_cast<uintptr_t>(e.before[0]),no,512);
    if(!prompt.length || !yes.length || !no.length) return;
    // Retain only ready bytes: the original wrapper owns/frees none of these caller strings.
    popupOwner=e.args[0];popupEpoch=e.epoch;popupYes=false;
    popupText=prompt;popupYesText=yes;popupNoText=no;
    popupFocus(true);
}
void quitSound(Event& e) {
    if(!popupOwner || popupEpoch!=generation() || e.args[0]!=popupOwner ||
        popupOwner!=read<uintptr_t>(address(0x8cc868)) || !read<uint8_t>(popupOwner+0x70)) return;
    // playEscSound's native UTF8 argument is already expanded at value+8.
    // Main.changeSelectedBtn and both mouse drag-over branches emit "change"
    // exactly when the opposite button owns focus, before transferring it.
    // Observe that accepted AS3 event; do not infer focus from raw inputs.
    char sound[7]{};const auto text=read<uintptr_t>(e.args[1]+8);
    for(unsigned i=0;i<sizeof(sound);++i) {if(!copy(text+i,sound+i,1))return;if(!sound[i])break;}
    if(!std::memcmp(sound,"change",7)) {popupYes=!popupYes;popupFocus(false);}
    else if(!std::memcmp(sound,"click",6) || !std::memcmp(sound,"cancel",7)) endPopup();
}
void escVisibility(Event& e) {
    // +70 is unchanged in both pinned layouts. This covers Esc closure even
    // when no popup-confirmed callback is delivered.
    if(e.ecx==popupOwner && !e.args[0]) endPopup();
}
void focusBefore(Event&) { focusing=true; page=row=-1; bindingHelp=false; }
void specialControls(uintptr_t entry,EscPacket& packet) {
    //702740 passes table+4 as activation text and binds table-4's primary
    // keyboard action. This page is an instruction list, not a value spinner.
    copyEscLocalized(read<unsigned>(entry-12),packet.text[1]);
    copyEscKey(read<unsigned>(entry-20,81),false,packet.bindings[0].first);
}
void featureBefore(Event& e) {
    // Original6E88B0 refuses an absent Flash movie before dispatching OnPlay.
    // Observe its native result message, never an attempted physical key.
    if(!nativeGameThread() || e.epoch!=generation() || !read<uintptr_t>(e.ecx)) return;
    switch(e.args[0]) {
    case 0x2f0:case 0x2f1: // HUD visibility
    case 0x2f6:case 0x2f7: // Supercharge
    case 0x2f9:case 0x2fa:case 0x2fb: // Encounter frequency
    case 0x2fd:case 0x2fe:case 0x2ff: // Turbo speed
    case 0x37f:case 0x380: // Auto Battle
        e.before[0]=static_cast<intptr_t>(e.args[0]);break;
    }
}
void featureAfter(Event& e) {
    // Both native returns explicitly set AL:0 for no movie,1 after OnPlay.
    if(!e.before[0] || !(e.result&0xff) || e.epoch!=generation()) return;
    EscPacket packet;prepareEscPacket(packet,EscCaptureKind::SpecialChanged);
    copyEscLocalized(static_cast<unsigned>(e.before[0]),packet.text[0]);
    commitEscPacket(packet);
}
void focusValue(Event& e) {
    if(!focusing) return;
    if(e.args[0]==0) page=static_cast<int>(e.result);
    else if(e.args[0]==1) row=static_cast<int>(e.result);
}
void focusAfter(Event&) {
    focusing=false;
    if(row<0) return;
    // Native68E730 sends page12 through702650's Special Features table.
    // It is not a controller binding page; its queued help callback owns focus.
    // Extracted EscPage.as identifies1120=Keyboard and1121=Controller.
    if(page==0x461 && row<static_cast<int>(_countof(controller))) {
        EscPacket packet;prepareEscPacket(packet,EscCaptureKind::BindingFocus);
        packet.pad=true;packet.count=1;copyEscBinding(controller[row],true,packet.bindings[0]);
        commitEscPacket(packet);bindingHelp=true;
    } else if(page==0x460 && row<static_cast<int>(_countof(keyboard))) {
        EscPacket packet;prepareEscPacket(packet,EscCaptureKind::BindingFocus);
        packet.pad=false;packet.count=1;
        copyEscBinding(keyboard[row],false,packet.bindings[0],0);
        commitEscPacket(packet);bindingHelp=true;
    }
}
void bindingBefore(Event& e) {
    const auto config=read<uintptr_t>(address(0x1efb504));if(!config) return;
    const bool pad=e.rva==0x301f40;
    const auto* rows=pad?controller:keyboard;
    const auto count=pad?_countof(controller):_countof(keyboard);
    for(size_t i=0;i<count;++i) {
        e.copied[i*2]=read<unsigned>(config+0x60+rows[i].first*4);
        e.copied[i*2+1]=read<unsigned>(config+0x60+rows[i].second*4);
    }
    e.before[0]=static_cast<intptr_t>(count*2);
}
void bindingAfter(Event& e) {
    const auto config=read<uintptr_t>(address(0x1efb504));if(!config) return;
    const bool pad=e.rva==0x301f40;
    const auto* rows=pad?controller:keyboard;
    const auto count=pad?_countof(controller):_countof(keyboard);
    if(e.before[0]!=static_cast<intptr_t>(count*2)) return;
    EscPacket packet;prepareEscPacket(packet,EscCaptureKind::BindingChanges);packet.pad=pad;
    for(size_t i=0;i<count;++i) {
        if(e.copied[i*2]!=read<unsigned>(config+0x60+rows[i].first*4) ||
           e.copied[i*2+1]!=read<unsigned>(config+0x60+rows[i].second*4))
            copyEscBinding(rows[i],pad,packet.bindings[packet.count++]);
    }
    if(packet.count) commitEscPacket(packet);
}
}
bool announceEscSetting(uintptr_t entry) {
    if(bindingHelp) {
        bindingHelp=false;
        EscPacket packet;prepareEscPacket(packet,EscCaptureKind::DetailOnly);
        copyEscLocalized(read<unsigned>(entry),packet.text[2]);
        packet.detail=beginSelectionDetail();packet.automatic=setting(Setting::AutoDetail)!=0;
        commitEscPacket(packet);return true;
    }
    const int selectedPage=read<int16_t>(entry+4,-1),selectedRow=read<int16_t>(entry+6,-1);
    const auto config=read<uintptr_t>(address(0x1efb504));
    EscPacket packet;prepareEscPacket(packet,EscCaptureKind::SettingFocus);
    const auto firstSpecial=address(0x83a2bc)+16;
    if(entry>=firstSpecial && entry-firstSpecial<5*24 && (entry-firstSpecial)%24==0) {
        // 702740 sends table+0 captions to addSpecialFeaturePageItem;
        // 702650 forwards table+20 to this help callback. This identity is
        // independent of the numeric row or context-hidden home options.
        packet.kind=EscCaptureKind::SpecialFocus;
        copyEscLocalized(read<unsigned>(entry-16),packet.text[0]);
        specialControls(entry,packet);
    } else if(config && selectedPage==110 && selectedRow>=0 && selectedRow<4) {
        const auto record=address(0x839620)+selectedRow*24;
        copyEscLocalized(read<unsigned>(record+4),packet.text[0]);
        packet.value=read<unsigned>(config+read<unsigned>(record+12)*4);
    } else if(config && selectedPage==111 && selectedRow>=0 && selectedRow<12) {
        packet.video=true;packet.index=static_cast<unsigned>(selectedRow);
        packet.value=read<unsigned>(config+escVideoFields[selectedRow]*4);
        copyEscLocalized(escVideoNames[selectedRow],packet.text[0]);
        copyEscVideo(packet.index,packet.value,packet.text[1]);
    } else {
        // 68E870's setHomePage/setSysSettingPage/setKeyBindingPage and
        // setControlBar supply these exact localized captions. The help ID
        // identifies the semantic option even when context hides other rows.
        const auto help=read<unsigned>(entry);unsigned label=0;
        switch(help) {
        case 0x218:label=0x217;break;case 0x21a:label=0x219;break;
        case 0x21c:label=0x21b;break;case 0x21e:label=0x21d;break;case 0x220:label=0x21f;break;
        case 0x222:label=0x221;break;case 0x224:label=0x223;break;case 0x226:label=0x225;break;
        case 0x26f:label=0x26e;break;case 0x271:label=0x270;break;
        case 0x20f:label=0x20e;break;case 0x214:label=0x213;break;case 0x216:label=0x215;break;
        // 6FC4D0 supplies these three addChangeParaPageItem captions.
        case 0x2b7:label=0x2b6;break;case 0x2be:label=0x2bd;break;case 0x2c5:label=0x2c4;break;
        default:return false;
        }
        packet.kind=EscCaptureKind::LabelFocus;copyEscLocalized(label,packet.text[0]);
    }
    copyEscLocalized(read<unsigned>(entry),packet.text[2]);
    packet.detail=beginSelectionDetail();packet.automatic=setting(Setting::AutoDetail)!=0;
    commitEscPacket(packet);return true;
}
bool escMenuActive() {
    // 690130 drives native Esc visibility and invokes onEscClose using +70.
    const auto menu=read<uintptr_t>(address(0x8cc868));
    return menu && read<uint8_t>(menu+0x70)!=0;
}
bool readEscControls() {
    if(!escMenuActive() || page!=12 || row<0 || row>=5) return false;
    EscPacket packet;prepareEscPacket(packet,EscCaptureKind::SpecialControls);
    specialControls(address(0x83a2bc)+16+row*24,packet);
    commitEscPacket(packet);return true;
}
void installEscDetails() {
    installEscCapture();
    hook(0x28df70,parameterBefore,parameterAfter);
    hook(0x28fca0,nullptr,quitOpened);
    hook(0x28fb60,quitSound,nullptr);hook(0x628ef0,escVisibility,nullptr);
    hook(0x28e730,focusBefore,focusAfter);hook(0x628b20,nullptr,focusValue);
    hook(0x301760,bindingBefore,bindingAfter);hook(0x301f40,bindingBefore,bindingAfter);
    hook(0x2e88b0,featureBefore,featureAfter);
}
void resetEscDetails() { focusing=false;page=row=-1;bindingHelp=false;popupOwner=0;popupEpoch=0;popupYes=false;popupText={};popupYesText={};popupNoText={};resetEscCapture(); }
int escVideoFieldIndex(unsigned field) {
    if(page!=111) return -1;
    for(unsigned i=0;i<_countof(escVideoFields);++i) if(escVideoFields[i]==field) return static_cast<int>(i);
    return -1;
}
}
