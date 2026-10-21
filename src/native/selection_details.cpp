#include "events.hpp"
#include "selection_details.hpp"
#include "copied_text.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include <atomic>
#include <cstring>
namespace ffx {
namespace {
enum Kind : unsigned {Replace,Append,Read,Reset};
struct Packet {SelectionDetailToken token;unsigned kind=Replace,automatic=0;};
struct View {unsigned epoch=0,serial=0,selected=0;};
async::AtomicSnapshot<View> published;
std::atomic<unsigned> serial{0};
struct Detail {std::wstring text;SelectionDetailToken token;};
ProcessStorage<Detail> storage;
bool current(SelectionDetailToken token) {
    View view;return published.tryRead(view) && view.selected && view.epoch==token.epoch &&
        view.serial==token.serial && token.epoch==generation();
}
bool same(SelectionDetailToken a,SelectionDetailToken b) {return a.epoch==b.epoch && a.serial==b.serial;}
void retire() {
    auto& detail=storage.get();
    if(!current(detail.token)) {std::wstring{}.swap(detail.text);detail.token={};}
}
void resourceReset(unsigned) {auto& detail=storage.get();std::wstring{}.swap(detail.text);detail.token={};}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(Packet)) return;
    Packet packet;std::memcpy(&packet,owned.data(),sizeof(packet));
    if(packet.token.epoch!=generation()) return;
    auto& detail=storage.get();
    if(packet.kind==Replace) {
        emitSelectionDetailOwned(owned.substr(sizeof(Packet)/sizeof(wchar_t)),packet.token,packet.automatic!=0,header);
    } else if(packet.kind==Append) {
        appendSelectionDetailOwned(owned.substr(sizeof(Packet)/sizeof(wchar_t)),packet.token);
    } else if(packet.kind==Read) {
        if(current(packet.token) && same(detail.token,packet.token))
            speechq::emitOwned(detail.text.empty()?L"No native description is available for this selection.":detail.text,
                header,"selection-description-hotkey",false,true);
    } else retire();
}
}
SelectionDetailToken beginSelectionDetail() {
    const SelectionDetailToken token{generation(),serial.fetch_add(1)+1};
    published.publish({token.epoch,token.serial,1});return token;
}
SelectionDetailToken currentSelectionDetailToken() {
    View view;
    if(!published.tryRead(view) || !view.selected || view.epoch!=generation()) return {};
    return {view.epoch,view.serial};
}
void appendSelectionDetailOwned(std::wstring text,SelectionDetailToken token) {
    auto& detail=storage.get();if(!current(token) || !same(detail.token,token)) return;
    if(!text.empty()) {if(!detail.text.empty()) detail.text+=L". ";detail.text+=text;}
}
void emitSelectionDetailOwned(std::wstring text,SelectionDetailToken token,bool automatic,const speechq::Header& header) {
    if(token.epoch!=generation()) return;
    if(current(token)) {auto& detail=storage.get();detail.text=text;detail.token=token;}
    // Automatic output belongs to each accepted producer event. A later equal
    // selection may replace the review buffer without dropping this event.
    if(automatic && !text.empty()) speechq::emitOwned(std::move(text),header,"selection-description");
}
void selectionDetail(const std::wstring& text,bool automatic) {
    Packet packet;packet.token=beginSelectionDetail();packet.automatic=automatic;
    speechq::captureDomain(speechq::SelectionDetailDomain,packet,packet.token.epoch,text);
}
void appendSelectionDetail(const std::wstring& text) {
    View view;if(text.empty() || !published.tryRead(view) || !view.selected || view.epoch!=generation()) return;
    Packet packet;packet.kind=Append;packet.token={view.epoch,view.serial};
    speechq::captureDomain(speechq::SelectionDetailDomain,packet,view.epoch,text);
}
bool readSelectionDetail() {
    if(readCopiedTextDetail()) return true;
    View view;if(!published.tryRead(view) || !view.selected || view.epoch!=generation()) return false;
    Packet packet;packet.kind=Read;packet.token={view.epoch,view.serial};
    speechq::captureDomain(speechq::SelectionDetailDomain,packet,view.epoch);
#ifdef FFX_READER_TEST
    extern void drainSpeechDomainsForTest();drainSpeechDomainsForTest();
#endif
    return true; // Pressure cannot fall through to a stale native buffer.
}
void resetSelectionDetail() {
    resetCopiedTextDetail();const auto next=serial.fetch_add(1)+1;published.publish({0,next,0});
    Packet packet;packet.kind=Reset;packet.token={generation(),next};
    speechq::captureDomain(speechq::SelectionDetailDomain,packet,packet.token.epoch);
}
void installSelectionDetails() {
    storage.prepare();speechq::registerDomain(speechq::SelectionDetailDomain,process,resourceReset);
    speechq::registerDomainMaintenance(speechq::SelectionDetailDomain,retire);
}
}
