#include "system_menu_capture.hpp"
#include "events.hpp"
#include "process_storage.hpp"
#include <atomic>
#include <map>
#include <cstring>
namespace ffx {
namespace {
std::atomic<unsigned> serial{1};
unsigned nativeResetEpoch=0; // Bootstrap, then the serialized native reset owner.
struct Slot {std::wstring area,time;};
struct State {std::map<unsigned,Slot> slots;std::wstring prompt;unsigned epoch=0,serial=0;};
ProcessStorage<State> storage;
void reset(unsigned epoch) {auto& s=storage.get();s.slots.clear();std::wstring{}.swap(s.prompt);s.epoch=epoch;s.serial=serial.load();}
void maintenance() {auto& s=storage.get();if(s.epoch!=generation() || s.serial!=serial.load()) reset(generation());}
std::vector<std::wstring> encoded(const ReadySystemText& source,const OwnedTextContext& context) {
    return decodeOwnedBytes(source.bytes.data(),source.length,OwnedTextMode::Terminated,context);
}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(SystemMenuPacket)) return;
    SystemMenuPacket p;std::memcpy(&p,owned.data(),sizeof(p));
    if(p.epoch!=generation() || p.serial!=serial.load()) return;
    auto& s=storage.get();if(s.epoch!=p.epoch || s.serial!=p.serial) reset(p.epoch);
    std::wstring line,detail;const char* producer="save-slot";
    switch(p.kind) {
    case SystemMenuKind::SlotPopulation: {
        if(p.index>=200) return;
        auto area=decodeReadyAscii(p.text[0]),time=decodeReadyAscii(p.text[1]);
        if(!area.empty() || !time.empty()) s.slots[p.index]={std::move(area),std::move(time)};
        return; }
    case SystemMenuKind::SlotFocus: {
        line=L"Slot "+std::to_wstring(p.index);const auto found=s.slots.find(p.index);
        if(found==s.slots.end()) line+=L" empty";
        else {if(!found->second.area.empty()) line+=L": "+found->second.area;if(!found->second.time.empty()) line+=L", "+found->second.time;}
        detail=line;break; }
    case SystemMenuKind::Popup:line=decodeReadyAscii(p.text[0]);s.prompt=line;detail=line;producer="save-popup";break;
    case SystemMenuKind::SaveHelp: {
        line=decodeReadyAscii(p.text[0]);const auto count=decodeReadyAscii(p.text[1]);
        if(!line.empty() && !count.empty()) line+=L", "+count;
        detail=line;producer="save-help";break; }
    case SystemMenuKind::YesNo:line=decodeReadyAscii(p.text[0]);detail=s.prompt;producer="save-confirm";break;
    case SystemMenuKind::EscHelp:line=clean(decodeReadyAscii(p.text[0]));detail=line;producer="esc-help";break;
    case SystemMenuKind::Config: {
        const auto context=copiedControlContext(p);
        const auto labels=encoded(p.text[0],context),values=encoded(p.text[1],context),descriptions=encoded(p.text[2],context);
        const auto label=labels.empty()?std::wstring{}:labels.front(),value=values.empty()?std::wstring{}:values.front();
        line=p.joinLabel?label+L": "+value:value;
        if(descriptions.size()>2 && descriptions[1]==L"-") detail=join(std::vector<std::wstring>(descriptions.begin()+2,descriptions.end()),L" ");
        producer="config";break; }
    case SystemMenuKind::Reset:if(!p.index) reset(p.epoch);return;
    }
    if(p.epoch!=generation() || p.serial!=serial.load()) return;
    speechq::emitOwned(std::move(line),header,producer,false,p.interrupt);
    if(p.detail.serial) emitSelectionDetailOwned(std::move(detail),p.detail,p.automatic,header);
}
}
void prepareSystemMenuPacket(SystemMenuPacket& p,SystemMenuKind kind,bool encodedText) {
    p.kind=kind;p.epoch=generation();p.serial=serial.load();if(encodedText) prepareCopiedControlSnapshot(p);
}
void copySystemAscii(uintptr_t source,ReadySystemText& text,unsigned limit) {
    copyReadyAscii(source,text,limit);
}
void copySystemEncoded(uintptr_t source,ReadySystemText& text,unsigned limit) {
    text.length=copyReadyGameText(source,text.bytes.data(),std::min<unsigned>(limit,static_cast<unsigned>(text.bytes.size())));
}
bool commitSystemMenuPacket(const SystemMenuPacket& p) {return speechq::captureDomain(speechq::SystemMenuDomain,p,p.epoch);}
void resetSystemMenuCapture() {
    const auto epoch=generation();const bool resource=nativeResetEpoch!=epoch;nativeResetEpoch=epoch;
    // Preparation already retires each resource epoch before its fresh records.
    // A delayed native owner reset must preserve fresh same-epoch slot data.
    if(!resource) serial.fetch_add(1);
    SystemMenuPacket p;prepareSystemMenuPacket(p,SystemMenuKind::Reset);p.index=resource?1u:0u;commitSystemMenuPacket(p);
}
void installSystemMenuCapture() {nativeResetEpoch=generation();storage.prepare();speechq::registerDomain(speechq::SystemMenuDomain,process,reset);speechq::registerDomainMaintenance(speechq::SystemMenuDomain,maintenance);}
}
