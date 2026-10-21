// Read the focused, shown airship image row from accepted native layer updates.
// Native owns the script/graphics pointers; preparation receives scalar IDs only.
// Evidence, language assets and extension rules: Docs/features/puzzles/airship.md.
#include "airship_destinations.hpp"
#include "airship_destinations_data.hpp"
#include "airship_map.hpp"
#include "events.hpp"
#include "atel_resources.hpp"
#include "speech_domains.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include <cstring>

namespace ffx {
namespace {
constexpr unsigned Domain=14,Cursor=26,ConfirmationLayer=29,FirstRow=11,VisibleRows=15;
struct Menu {
    AtelIdentity identity;
    uintptr_t worker=0,header=0,declarations=0,layers=0;
    unsigned count=0;
};
struct Selection {
    unsigned epoch=0,allocation=0;
    uintptr_t chunk=0;
    int index=-1,id=-1,language=-1;
    unsigned layer=0;
    bool active=false;
} selected;
std::atomic<unsigned> revision{1};
// Layer29 is one image containing both choices. Its native texture variant is
// the focused choice. Showing it arms a lifetime but must not speak the previous
// opening's leftover variant: hiku2100 sets the initial texture one frame later.
struct Confirmation {
    unsigned epoch=0;
    uintptr_t layers=0;
    int option=-1,language=-1;
    bool armed=false;
} confirmation;
std::atomic<unsigned> confirmationRevision{1};
struct Packet {
    unsigned epoch=0,revision=0,index=0,count=0,id=0,language=0,opening=0,layer=0;
};

// One scalar diagnostic per outcome/epoch, through the nonblocking logger.
// It distinguishes native-event refusal from output failure in the next own
// session log, without a per-frame dump or any speech-content suppression.
void diagnostic(unsigned outcome,const Menu& menu) {
    static unsigned epoch=0,seen=0;
    if(epoch!=generation()) {epoch=generation();seen=0;}
    const unsigned bit=1u<<outcome;if(seen&bit) return;seen|=bit;
    recordLog("airship-focus outcome=%u map=%d layers=%u epoch=%u",outcome,
              read<int>(address(0xf30804),-1),menu.count,epoch);
}

void close() {
    if(selected.active) {selected={};revision.fetch_add(1,std::memory_order_acq_rel);}
}
void closeConfirmation() {
    if(confirmation.armed) {confirmation={};confirmationRevision.fetch_add(1,std::memory_order_acq_rel);}
}

// Script bindings are optional absolute-index metadata. The accepted visible
// image/cursor is the speech authority, even when script preparation is pending
// or the native scheduler has moved to another worker. Never make renderer text
// depend on an unrelated current-worker/getter readiness assumption.
bool metadataContext(Menu& menu) {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const auto count=read<uint16_t>(controller+0xc),active=read<uint16_t>(controller+0xa,0xffff);
    if(!controller || !count || count>500 || active>=count || read<uint16_t>(controller+0x1a,0xffff)) return false;
    using Worker=uintptr_t(__cdecl*)(int);
    menu.worker=reinterpret_cast<Worker>(address(0x46a7c0))(active);
    const auto chunk=read<uintptr_t>(menu.worker+4);
    if(!menu.worker || read<uint16_t>(menu.worker+0x2e,0xffff)!=9 ||
       chunk!=read<uintptr_t>(controller+0x48) || !readyAtelIdentity(chunk,menu.identity)) return false;
    const auto& id=menu.identity;
    if(id.span.epoch!=generation() || id.length!=24785 || id.checksum!=0xde956a03 ||
       id.workers!=41 || id.codeOffset!=0xe6c || std::strcmp(id.name,"hiku2100")) return false;
    unsigned header=0;
    if(!readyAtelWorkerHeader(chunk,9,header) || header>id.span.size || id.span.size-header<0x28 ||
       read<uintptr_t>(menu.worker)!=chunk+header) return false;
    menu.header=chunk+header;
    const auto offset=read<unsigned>(menu.header+0x14);
    const auto variables=read<uint16_t>(menu.header+2);
    if(variables<30 || variables>1024 || offset>id.span.size || variables*8ull>id.span.size-offset) return false;
    menu.declarations=chunk+offset;

    return true;
}

bool context(Menu& menu) {
    if(!airshipMapActive() || scene || inBattle() || !gameFocused() ||
       modMenuActive() || utilityConsoleActive()) return false;
    // Original 91A3D0/91A830/91AB30 share this native layer owner. These reads
    // stay inside the callback; a queued packet never receives the table address.
    const auto root=read<uintptr_t>(address(0x153bf80));
    const auto graphic=read<uintptr_t>(root),table=read<uintptr_t>(graphic+8);
    menu.count=read<unsigned>(table+4);menu.layers=read<uintptr_t>(table+8);
    return root && graphic && table && menu.layers && menu.count>=48 && menu.count<=128;
}

bool value(const Menu& menu,unsigned index,unsigned low,unsigned high,unsigned element,int& result) {
    unsigned preparedLow=0,preparedHigh=0,live[2]{};
    const auto chunk=menu.identity.span.chunk;
    if(element>=high || !readyAtelDeclarationAt(chunk,static_cast<unsigned>(menu.header-chunk),index,preparedLow,preparedHigh) ||
       preparedLow!=low || preparedHigh!=high || !copy(menu.declarations+index*8,live,sizeof(live)) ||
       live[0]!=low || live[1]!=high) return false;
    using Getter=uintptr_t(__cdecl*)(uintptr_t,int,int);
    const auto operand=reinterpret_cast<Getter>(address(0x46c500))(menu.worker,index,element);
    return operand && copy(operand,&result,sizeof(result));
}

bool shown(const Menu& menu,unsigned layer) {
    return layer<menu.count && !(read<unsigned>(menu.layers+layer*0xc0,1)&1);
}

void confirmationShown(Event& e) {
    if(e.rva!=0x51a830) return;
    Menu menu;
    if(!context(menu) || !shown(menu,ConfirmationLayer)) {closeConfirmation();return;}
    closeConfirmation();
    confirmation={generation(),menu.layers,-1,-1,true};
}

void confirmationChanged(Event& e) {
    if(e.args[0]!=ConfirmationLayer || e.epoch!=generation()) return;
    Menu menu;
    if(!confirmation.armed) return;
    if(!context(menu) || confirmation.epoch!=generation() || confirmation.layers!=menu.layers ||
       !shown(menu,ConfirmationLayer) || shown(menu,10) || shown(menu,Cursor)) {closeConfirmation();return;}
    // Read the accepted WORD written by original91A930, not the setter argument.
    // All engine pointers stay on this native owner; domain14 receives scalars.
    const auto option=read<uint16_t>(menu.layers+ConfirmationLayer*0xc0+0x96,0xffff);
    if(option>1) {closeConfirmation();return;}
    using Language=unsigned(__cdecl*)();
    const auto language=reinterpret_cast<Language>(address(0x241290))();
    if(confirmation.option==option && confirmation.language==static_cast<int>(language)) return;
    Packet packet;packet.epoch=generation();packet.revision=confirmationRevision.load(std::memory_order_acquire);
    packet.id=option;packet.language=language;packet.opening=confirmation.option<0;packet.layer=ConfirmationLayer;
    if(!speechq::captureDomain(Domain,packet,packet.epoch)) return;
    confirmation.option=option;confirmation.language=static_cast<int>(language);
}

bool snapshot(const Menu& menu,Packet& packet) {
    if(!shown(menu,10) || !shown(menu,Cursor)) return false;
    const auto actualY=read<int>(menu.layers+Cursor*0xc0+0x10,0x7fffffff);
    // Original hiku2100 draws rows at84+18*r; 91AB30 moves the first/last
    // cursor one pixel. Require an actual row, not an arbitrary nearby image.
    if(actualY<83 || actualY>337) return false;
    const unsigned row=static_cast<unsigned>((actualY-83)/18),layer=FirstRow+row;
    const int y=84+18*static_cast<int>(row);
    if(row>=VisibleRows || actualY<y-1 || actualY>y+1 || !shown(menu,layer)) return false;
    const auto id=read<uint16_t>(menu.layers+layer*0xc0+0x96,0xffff);
    if(id<1 || id>26) return false;
    using Language=unsigned(__cdecl*)();
    packet.epoch=generation();packet.revision=revision.load(std::memory_order_acquire);
    packet.index=row;packet.count=0;packet.id=id;packet.layer=layer;
    packet.language=reinterpret_cast<Language>(address(0x241290))();
    return true;
}

void addMetadata(Menu& menu,Packet& packet) {
    if(!metadataContext(menu)) return;
    int mode=0,index=-1,last=-1,scroll=-1,x=0,y=0,id=0;
    if(!value(menu,29,0x5c0000e8,1,0,mode) || mode!=1 ||
       !value(menu,7,0x5c000008,1,0,index) || !value(menu,16,0x5c00002c,1,0,last) ||
       !value(menu,25,0x5c0000c8,1,0,scroll) || index<0 || last<0 || last>=31 || index>last ||
       scroll<0 || scroll>index || index-scroll>=static_cast<int>(VisibleRows) ||
       !value(menu,18,0x5d000034,31,static_cast<unsigned>(index),id) || id<1 || id>26 ||
       !value(menu,10,0x5c000014,1,0,x) || !value(menu,11,0x5c000018,1,0,y)) return;
    const auto row=static_cast<unsigned>(index-scroll),layer=FirstRow+row;
    if(layer!=packet.layer || id!=static_cast<int>(packet.id) || y!=84+18*static_cast<int>(row)) return;
    const auto cursor=menu.layers+Cursor*0xc0;
    const auto actualY=read<int>(cursor+0x10,0x7fffffff);
    // Native room382 adjusts the top/bottom cursor by one pixel (91AB30).
    if(read<int>(cursor+0xc,0x7fffffff)!=x || actualY<y-1 || actualY>y+1) return;
    packet.index=static_cast<unsigned>(index);packet.count=static_cast<unsigned>(last+1);
}

void observed(Event& e) {
    if(e.args[0]==ConfirmationLayer && e.epoch==generation()) {confirmationShown(e);return;}
    // Only accepted cursor show/position events carry selection. Background10
    // show events precede row/cursor assembly and cannot announce a ready focus.
    if(e.args[0]!=Cursor || e.epoch!=generation()) return;
    Menu menu;Packet packet;
    if(!context(menu)) {diagnostic(1,menu);close();return;}
    if(!snapshot(menu,packet)) {diagnostic(2,menu);close();return;}
    addMetadata(menu,packet);
    if(selected.active && (selected.epoch!=packet.epoch || selected.chunk!=menu.layers)) close();
    if(selected.active && selected.index==static_cast<int>(packet.index) && selected.id==static_cast<int>(packet.id) &&
       selected.language==static_cast<int>(packet.language)) return; // Native focus identity, never text equality.
    packet.revision=revision.load(std::memory_order_acquire);packet.opening=!selected.active;
    if(!speechq::captureDomain(Domain,packet,packet.epoch)) {diagnostic(3,menu);return;} // Retry at a later native cursor event.
    diagnostic(packet.count?4:5,menu);
    selected={packet.epoch,0,menu.layers,static_cast<int>(packet.index),
              static_cast<int>(packet.id),static_cast<int>(packet.language),packet.layer,true};
}

void hidden(Event& e) {
    if(nativeGameThread() && e.args[0]==ConfirmationLayer) closeConfirmation();
    if(nativeGameThread() && (e.args[0]==Cursor || e.args[0]==10 ||
       (selected.active && e.args[0]==selected.layer))) close();
}

void receive(const speechq::Header& header,const std::wstring& bytes) {
    if(bytes.size()*sizeof(wchar_t)!=sizeof(Packet)) return;
    Packet packet;std::memcpy(&packet,bytes.data(),sizeof(packet));
    const bool isConfirmation=packet.layer==ConfirmationLayer;
    const auto liveRevision=isConfirmation?confirmationRevision.load(std::memory_order_acquire):revision.load(std::memory_order_acquire);
    if(packet.epoch!=generation() || packet.revision!=liveRevision ||
       !(header.context&speechq::SpeechEnabled)) return;
    const auto label=isConfirmation?airshipLabels::confirmation(packet.language,packet.id):airshipLabels::label(packet.language,packet.id);
    if(!label || !*label) return;
    // Opening appends after the native Select destination/New location prompt.
    // Native TK text remains its sole producer; only the focused image is added.
    speechq::emitOwned(std::wstring(label)+(packet.count?menuIndex(packet.index,packet.count):L""),header,
                      isConfirmation?"airship-confirmation":"airship-destination",false,!packet.opening);
}
}
void resetAirshipDestinations() {
    selected={};revision.fetch_add(1,std::memory_order_acq_rel);
    confirmation={};confirmationRevision.fetch_add(1,std::memory_order_acq_rel);
}
void installAirshipDestinations() {
    speechq::registerDomain(Domain,receive,nullptr);
    hook(0x51a830,nullptr,observed);
    hook(0x51ab30,nullptr,observed);
    hook(0x51a7c0,nullptr,hidden);
    hook(0x51a930,nullptr,confirmationChanged);
}
}
