// Keep nonblocking swimming control instructions distinct from dialogue. Native
// window/source/serial facts are rechecked against the owned page classification
// before review keys are admitted. A selector or missing preparation stays
// blocking, so uncertain text never silently hands control back to movement.
// See Docs/features/menus/index.md#dialogue-and-review.
#include "field_controls.hpp"
#include "navigation.hpp"
#include "text_plan.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include <cstring>
namespace ffx {
namespace {
constexpr unsigned Receiver=6;
std::atomic<unsigned> revision{1};
FieldControlsSnapshot nativeView;unsigned nextSerial=0;
struct Ready {unsigned epoch=0,revision=0,serial=0;uintptr_t window=0,source=0;unsigned valid=0;};
async::AtomicSnapshot<Ready> ready;
struct State {FieldControlsSnapshot view;std::vector<std::wstring> lines;unsigned selected=0;};
ProcessStorage<State> storage;
bool same(const FieldControlsSnapshot& a,const FieldControlsSnapshot& b) {
    return a.epoch==b.epoch && a.revision==b.revision && a.slot==b.slot && a.serial==b.serial && a.window==b.window && a.source==b.source;
}
bool available() {
    const auto& s=nativeView;
    if(!s.candidate || s.epoch!=generation() || s.revision!=revision.load(std::memory_order_acquire) ||
        s.slot>=8 || scene || inBattle() || !swimmingPlane()) return false;
    Ready prepared;
    if(ready.tryRead(prepared) && prepared.epoch==s.epoch && prepared.revision==s.revision &&
        prepared.serial==s.serial && prepared.window==s.window && prepared.source==s.source && !prepared.valid) return false;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    return controller && (read<uint8_t>(controller)&2) && s.window &&
        read<uintptr_t>(address(0x14676b0)+s.slot*4)==s.window &&
        read<uintptr_t>(s.window+0x94)==s.source && read<uint16_t>(s.window+0x2c)>0 && read<uint16_t>(s.window+0x2c)<3;
}
void maintain() {
    auto& s=storage.get();
    if(s.view.epoch && (s.view.epoch!=generation() || s.view.revision!=revision.load(std::memory_order_acquire))) s={};
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring,std::wstring) {
    FieldControlsSnapshot copied;static_assert(sizeof(copied)<=sizeof(p.context));
    std::memcpy(&copied,p.context.data(),sizeof(copied));
    auto& s=storage.get();maintain();
    if(!same(copied,s.view) || s.lines.empty()) return;
    if(p.flags==VK_UP && s.selected) --s.selected;
    if(p.flags==VK_DOWN && s.selected+1<s.lines.size()) ++s.selected;
    if(p.flags==VK_LEFT) s.selected=0;
    speechq::emitOwned(p.flags==VK_RIGHT?join(s.lines,L". "):s.lines[s.selected],header,"field-controls-review",false,true);
}
}
void installFieldControls() {
    storage.prepare();registerTextPlanReceiver(Receiver,receive);registerTextPlanMaintenance(Receiver,maintain);
}
void resetFieldControls() {revision.fetch_add(1,std::memory_order_release);nativeView={};}
FieldControlsSnapshot captureFieldControls(unsigned slot,uintptr_t source) {
    FieldControlsSnapshot s;s.epoch=generation();s.revision=revision.load(std::memory_order_acquire);s.slot=slot;s.source=source;
    invalidateFieldControlsSlot(slot);
    if(slot>=8 || scene || inBattle() || !swimmingPlane() || read<uint8_t>(source)!=9 || read<uint8_t>(source+1)!=0x30) return s;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    if(!controller || !(read<uint8_t>(controller)&2)) return s;
    s.window=read<uintptr_t>(address(0x14676b0)+slot*4);s.serial=++nextSerial;s.candidate=true;nativeView=s;return s;
}
void invalidateFieldControlsSlot(unsigned slot) {if(nativeView.slot==slot) nativeView.candidate=false;}
void rejectFieldControlsCapture(const FieldControlsSnapshot& s) {if(same(s,nativeView)) nativeView.candidate=false;}
FieldControlsSnapshot fieldControlsForSource(unsigned slot,uintptr_t source) {
    if(nativeView.candidate && nativeView.slot==slot && nativeView.source==source) return nativeView;
    return {};
}
FieldControlsClassification classifyFieldControls(const FieldControlsSnapshot& s) {
    if(!s.candidate || s.epoch!=generation() || s.revision!=revision.load(std::memory_order_acquire) ||
        s.slot>=8 || read<uintptr_t>(address(0x14676b0)+s.slot*4)!=s.window || read<uintptr_t>(s.window+0x94)!=s.source)
        return FieldControlsClassification::Blocking;
    Ready prepared;
    if(!ready.tryRead(prepared) || prepared.epoch!=s.epoch || prepared.revision!=s.revision || prepared.serial!=s.serial ||
        prepared.window!=s.window || prepared.source!=s.source) return FieldControlsClassification::Pending;
    return prepared.valid?FieldControlsClassification::Nonblocking:FieldControlsClassification::Blocking;
}
bool observeFieldControlsOwned(const MessagePage& page,const FieldControlsSnapshot& copied) {
    if(!copied.candidate || copied.epoch!=generation() || copied.revision!=revision.load(std::memory_order_acquire)) return false;
    const bool valid=!page.selector && !page.lines.empty();
    ready.publish({copied.epoch,copied.revision,copied.serial,copied.window,copied.source,valid?1u:0u});
    if(!valid) return false;
    clearDialogueReviewOwned();auto& s=storage.get();s.view=copied;s.lines=page.lines;s.selected=0;return true;
}
bool fieldControlsCapturesKey(unsigned vk,bool control) {
    return control && available() && (vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT);
}
bool dispatchFieldControlsKey(unsigned vk,bool control) {
    if(!fieldControlsCapturesKey(vk,control)) return false;
    TextPlan p;prepareTextPlan(p,"field-controls-review");p.receiver=Receiver;p.flags=vk;
    std::memcpy(p.context.data(),&nativeView,sizeof(nativeView));commitTextPlan(p);return true;
}
}
