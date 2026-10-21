#include "esc_capture.hpp"
#include <atomic>
#include <cstring>
namespace ffx {
namespace {
std::atomic<unsigned> serial{1};
std::wstring localized(const ReadyBytes<512>& text) {return clean(decodeReadyAscii(text));}
std::wstring specialActivation(const EscPacket& p) {
    auto text=decodeReadyAscii(p.text[1]);
    const auto marker=text.find(L"$key$");
    if(marker!=std::wstring::npos) {
        const auto key=decodeReadyAscii(p.bindings[0].first);
        if(key.empty()) return {};
        // Original702670 replaces the first marker in702740's activation text.
        text.replace(marker,5,key);
    }
    return clean(std::move(text));
}
std::wstring videoValue(unsigned index,unsigned value,const ReadyBytes<512>& label) {
    if(index==0) return std::to_wstring(value>>16)+L" by "+std::to_wstring(value&0xffff);
    if(index==2) return std::to_wstring(value);
    if(index==5) return value<5?std::to_wstring(1u<<value)+L" x":L"";
    return localized(label);
}
std::wstring binding(const EscBindingText& p,bool pad,bool single=false) {
    auto text=localized(p.label)+L": "+decodeReadyAscii(p.first);
    if(!pad && !single) text+=L", "+decodeReadyAscii(p.second);
    return text;
}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(EscPacket)) return;
    EscPacket p;std::memcpy(&p,owned.data(),sizeof(p));
    if(p.epoch!=generation() || p.serial!=serial.load()) return;
    std::wstring line,detail;const char* producer="esc-setting-focus";bool interrupt=true;
    switch(p.kind) {
    case EscCaptureKind::Popup:
        speechq::emitOwned(localized(p.text[0]),header,"esc-popup-prompt",false,true);
        line=localized(p.text[1]);detail=localized(p.text[0]);producer="esc-popup-focus";interrupt=false;break;
    case EscCaptureKind::Quit:
        speechq::emitOwned(localized(p.text[0]),header,"esc-quit-prompt",false,true);
        line=localized(p.text[1]);detail=localized(p.text[0]);producer="esc-quit-focus";interrupt=false;break;
    case EscCaptureKind::QuitFocus:line=localized(p.text[1]);detail=localized(p.text[0]);producer="esc-quit-focus";break;
    case EscCaptureKind::BindingFocus:line=binding(p.bindings[0],p.pad,!p.pad);producer=p.pad?"esc-controller-focus":"esc-keyboard-focus";break;
    case EscCaptureKind::BindingChanges: {
        if(p.count>p.bindings.size()) return;
        std::vector<std::wstring> changes;for(unsigned i=0;i<p.count;++i) changes.push_back(binding(p.bindings[i],p.pad));
        line=join(changes);producer="esc-bindings-change";break; }
    case EscCaptureKind::DetailOnly:detail=localized(p.text[2]);break;
    case EscCaptureKind::LabelFocus:line=localized(p.text[0]);detail=localized(p.text[2]);break;
    case EscCaptureKind::SpecialFocus: {
        line=localized(p.text[0]);const auto activation=specialActivation(p);
        if(!activation.empty()) {if(!line.empty()) line+=L". ";line+=activation;}
        detail=localized(p.text[2]);break; }
    case EscCaptureKind::SpecialControls:
        line=specialActivation(p);if(line.empty()) line=L"No controls displayed.";
        producer="esc-special-controls";break;
    case EscCaptureKind::SpecialChanged:line=localized(p.text[0]);producer="game-special-feature";break;
    case EscCaptureKind::SettingFocus:
        line=localized(p.text[0])+L": "+(p.video?videoValue(p.index,p.value,p.text[1]):std::to_wstring(p.value));
        detail=localized(p.text[2]);break;
    case EscCaptureKind::AudioChanged: {
        auto name=localized(p.text[0]);if(!name.empty()) line=name+L": "+std::to_wstring(p.value);
        producer="esc-audio-value";break; }
    case EscCaptureKind::VideoChanged:line=localized(p.text[0])+L": "+videoValue(p.index,p.value,p.text[1]);producer="esc-video-value";interrupt=false;break;
    }
    if(p.epoch!=generation() || p.serial!=serial.load()) return;
    speechq::emitOwned(std::move(line),header,producer,false,interrupt);
    if(p.detail.serial) emitSelectionDetailOwned(std::move(detail),p.detail,p.automatic,header);
}
}
void prepareEscPacket(EscPacket& p,EscCaptureKind kind) {p.kind=kind;p.epoch=generation();p.serial=serial.load();}
void copyEscLocalized(unsigned id,ReadyBytes<512>& text,unsigned limit) {
    char buffer[512]{};using Resolve=void(__cdecl*)(unsigned,char*,unsigned);
    limit=std::min<unsigned>(limit,512);reinterpret_cast<Resolve>(address(0x2414a0))(id,buffer,limit);
    copyReadyAscii(reinterpret_cast<uintptr_t>(buffer),text,limit);
}
void copyEscKey(unsigned action,bool pad,ReadyBytes<64>& bytes) {
    bytes.length=0;
    if(action>80) return;
    const auto config=read<uintptr_t>(address(0x1efb504));if(!config) return;
    using Name=uintptr_t(__cdecl*)(unsigned);
    const auto value=read<unsigned>(config+0x60+action*4,128);
    if(value<128) copyReadyAscii(reinterpret_cast<Name>(address(pad?0x5b10:0x5af0))(value),bytes,64);
}
void copyEscBinding(const Binding& row,bool pad,EscBindingText& text,unsigned selected) {
    copyEscLocalized(row.label,text.label);
    copyEscKey(selected==1?row.second:row.first,pad,text.first);
    if(!pad && selected==2) copyEscKey(row.second,false,text.second);
}
void copyEscVideo(unsigned index,unsigned value,ReadyBytes<512>& text) {
    unsigned id=0;
    switch(index) {
    case 1:if(value<3) id=0x235+value;break;
    case 3:if(value<2) id=value?0x23c:0x23d;break;
    case 4:if(value<=2) id=0x24b+2-value;break;
    case 6: {constexpr unsigned ids[]={0x243,0x248,0x247,0x246};if(value<4) id=ids[value];break;}
    case 7:if(value==0) id=0x250;else if(value==2) id=0x251;else if(value==4) id=0x252;else if(value==8) id=0x253;break;
    case 8:if(value<2) id=value?0x256:0x257;break;
    case 9:if(value<6) id=0x25a+value;break;
    case 10:if(value<2) id=value?0x262:0x263;break;
    case 11:if(value<4) id=0x266+value;break;
    }
    if(id) copyEscLocalized(id,text);
}
bool commitEscPacket(const EscPacket& p) {return speechq::captureDomain(speechq::EscMenuDomain,p,p.epoch);}
void installEscCapture() {speechq::registerDomain(speechq::EscMenuDomain,process,nullptr);}
void resetEscCapture() {serial.fetch_add(1);}
}
