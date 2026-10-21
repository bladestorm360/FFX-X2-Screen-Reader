#include "events.hpp"
#include "naming.hpp"
#include "copied_controls.hpp"
#include "speech_domains.hpp"
#include "settings.hpp"
#include "airship_map.hpp"
#include <cstring>
namespace ffx {
namespace {
// Namespace state is owned by serialized native readers/input; resets run only on that owner.
enum Kind : unsigned {ReadName,Glyph,Control};
struct Packet {
    CopiedControls controls;
    std::array<uint8_t,128> cells{};
    std::array<uint8_t,256> text{};
    unsigned count=0,length=0,session=0,kind=ReadName;
    bool password=false; // Copied on native owner; workers never inspect the menu.
};
Packet entered;
std::atomic<unsigned> session{1};
bool active=false,pending=false;
unsigned epoch=0;
int cursor=-1;
using Key=std::array<intptr_t,32>;
thread_local int group=-1,column=0;
thread_local bool drawing=false;
// Resolve labels from copied glyph text on the speech worker, so punctuation
// remains audible without adding reads or work to the native hook.
const wchar_t* spokenPunctuation(const std::wstring& glyph) {
    if(glyph.size()!=1) return nullptr;
    struct Entry { wchar_t symbol; const wchar_t* name; };
    static constexpr Entry names[]={
        {L'*',L"asterisk"},{L'/',L"slash"},{L'%',L"percent"},{L'&',L"ampersand"},
        {L'.',L"period"},{L',',L"comma"},{L'-',L"hyphen"},{L'\'',L"apostrophe"},
        {L'\"',L"quotation mark"},{L'!',L"exclamation mark"},{L'?',L"question mark"},
        {L':',L"colon"},{L';',L"semicolon"},{L'_',L"underscore"},{L'(',L"left parenthesis"},
        {L')',L"right parenthesis"},{L'+',L"plus"},{L'=',L"equals"},{L'\\',L"backslash"},
        {L'#',L"number sign"},{L'@',L"at sign"},{L'$',L"dollar sign"},{L'[',L"left bracket"},
        {L']',L"right bracket"}
    };
    for(const auto& entry:names) if(glyph[0]==entry.symbol) return entry.name;
    return nullptr;
}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(Packet)) return;
    Packet packet;std::memcpy(&packet,owned.data(),sizeof(packet));
    if(packet.session!=session.load(std::memory_order_acquire)) return;
    const auto context=copiedControlContext(packet.controls);
    std::wstring text;const char* producer="name-read";
    if(packet.kind==ReadName) {
        for(unsigned i=0;i<packet.count && i<32 && packet.cells[i*4];++i)
            text+=join(decodeOwnedBytes(packet.cells.data()+i*4,4,OwnedTextMode::Literal,context),L"");
        text=packet.password?(text.empty()?L"Password is empty":L"Password: "+text):
            (text.empty()?L"Name is empty":L"Name: "+text);
    } else {
        const auto parts=decodeOwnedBytes(packet.text.data(),packet.length,
            packet.kind==Glyph?OwnedTextMode::Literal:OwnedTextMode::Terminated,context);
        if(packet.kind==Glyph) {
            text=join(parts,L"");
            if(text==L" ") text=L"Space";
            else if(setting(Setting::SpeakPunctuationNames))
                if(const auto* spoken=spokenPunctuation(text)) text=spoken;
            producer="name-letter";
        }
        else {text=parts.empty()?std::wstring{}:parts.front();producer="name-control";}
    }
    speechq::emitOwned(std::move(text),header,producer,false,true);
}
void send(Packet& packet,unsigned eventEpoch) {
    packet.session=session.load(std::memory_order_acquire);
    (void)speechq::captureDomain(speechq::NamingDomain,packet,eventEpoch);
}
Key focus(uintptr_t window) {
    Key k{};
    k[0]=read<int>(window+0x28,-1);
    k[1]=read<int>(address(0x146a21c),-1);
    k[2]=read<int>(address(0x146a228),-1);
    k[3]=read<int>(address(0x146a22c),-1);
    k[4]=read<int>(address(0x146a230),-1);
    k[5]=read<int>(address(0x146a218),-2);
    return k;
}
void before(Event& e) { e.before=focus(e.args[0]); }
void after(Event& e) {
    const auto k=focus(e.args[0]);
    // The native renderer reads one terminated glyph per four-byte name cell.
    using Getter=uintptr_t(__cdecl*)();
    const auto descriptor=reinterpret_cast<Getter>(address(0x4c4f60))();
    const auto limit=read<uint16_t>(descriptor+2);
    Packet name;prepareCopiedControlSnapshot(name.controls);
    // Original8C4230 normalizes the password selector to -1; ordinary character
    // and aeon naming use0..17. Qualify the airship UI as well so this requested
    // label does not change any unrelated naming screen. This state is copied
    // with the cells, before the native descriptor/window can be reused.
    name.password=k[5]==-1 && (airshipMapActive() || airshipSearchScript()!=0);
    if(limit>0 && limit<=32 && copy(address(0x146a240),name.cells.data(),limit*4)) name.count=limit;
    const bool visible=k[0]>=0 && k[0]<9 && !read<uint8_t>(e.args[0]+0x41);
    {
      if(active && !visible) session.fetch_add(1,std::memory_order_release);
      active=visible; epoch=e.epoch; entered=name;
      if(!visible) { pending=false; return; }
      if(k==e.before || k[0]!=4) return;
      cursor=static_cast<int>(k[2]); pending=k[4]==0; }
    if(k[4]!=1 || k[3]<0 || k[3]>=16) return;
    const auto controls=reinterpret_cast<Getter>(address(0x4c4fc0))();
    // Native control order consists of (localized text ID, action ID) pairs.
    for(int i=0;i<=k[3];++i) if(!read<uint16_t>(controls+i*4)) return;
    using Label=uintptr_t(__cdecl*)(unsigned);
    Packet packet;packet.kind=Control;prepareCopiedControlSnapshot(packet.controls);
    const auto source=reinterpret_cast<Label>(address(0x4beda0))(read<uint16_t>(controls+k[3]*4));
    packet.length=copyReadyGameText(source,packet.text.data(),static_cast<unsigned>(packet.text.size()));
    send(packet,e.epoch);
}
void opened(Event&) { resetNaming(); }
void gridBefore(Event& e) {
    e.before[0]=drawing; e.before[1]=group; e.before[2]=column;
    { drawing=active && epoch==e.epoch && pending; }
    group=-1; column=0;
}
void gridAfter(Event& e) { drawing=e.before[0]!=0; group=static_cast<int>(e.before[1]); column=static_cast<int>(e.before[2]); }
void paletteRow(Event& e) {
    if(drawing && e.parentRva==0x4c4530) { ++group; column=0; }
}
void glyph(Event& e) {
    if(!drawing || group<0 || e.parentRva!=0x4c4530) return;
    const int slot=group*5+column++;
    {
      if(!active || epoch!=e.epoch || !pending || slot!=cursor) return;
      pending=false; }
    Packet packet;packet.kind=Glyph;prepareCopiedControlSnapshot(packet.controls);
    packet.length=copyReadyGameText(e.args[0],packet.text.data(),4);
    send(packet,e.epoch);
}
}
void resetNaming() { active=pending=false;entered={};cursor=-1;session.fetch_add(1,std::memory_order_release); }
bool namingReadbackAvailable() {return active && epoch==generation();}
bool readEnteredName() {
    if(!namingReadbackAvailable()) return false;
    auto packet=entered;send(packet,epoch);
#ifdef FFX_READER_TEST
    extern void drainSpeechDomainsForTest();drainSpeechDomainsForTest();
#endif
    return true;
}
void installNaming() {
    speechq::registerDomain(speechq::NamingDomain,process,nullptr);
    hook(0x4c4230,opened,nullptr);
    hook(0x4c3560,before,after);
    hook(0x4c4530,gridBefore,gridAfter);
    hook(0x4beda0,nullptr,paletteRow);
    hook(0x3902f0,nullptr,paletteRow);
    hook(0x505ab0,glyph,nullptr);
}
}
