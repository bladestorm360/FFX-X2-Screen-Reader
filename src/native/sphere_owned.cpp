#include "sphere_owned.hpp"
#include "copied_controls.hpp"
#include "process_storage.hpp"
#include "navigation_cancel.hpp"
#include "events.hpp"
#include "controller.hpp"
#include "nav_audio.hpp"
#include <atomic>
#include <cstring>
#include <algorithm>
namespace ffx {
namespace {
constexpr unsigned Domain=13,Slots=4,Writing=0x80000000u;
template<unsigned N> struct Encoded {std::array<uint8_t,N> bytes;unsigned length=0;};
struct NodeFact {
    int x=0,y=0,type=-1;unsigned flags=0,ability=0,activated=0,key=0;bool valid=false;
    Encoded<256> name,abilityName;Encoded<1024> description,abilityDescription;
};
struct ItemFact {unsigned key=0,stock=0;Encoded<256> name;};
struct Facts {
    CopiedControls controls;std::array<NodeFact,1024> nodes;std::array<GridLink,1024> links;
    std::array<ItemFact,256> items;std::array<Encoded<40>,7> people;std::array<Encoded<256>,8> stats;
    std::array<int,7> positions;std::array<unsigned,7> levels;
    int selected=-1,character=-1;unsigned nodeCount=0,linkCount=0;bool valid=false;
    uintptr_t grid=0,definitions=0,itemDefinitions=0;unsigned epoch=0,language=0,revision=0;
};
struct Frame {Facts facts;unsigned revision=0;};
struct Packet {SphereWork work;unsigned epoch=0,revision=0,slot=Slots,token=0,key=0;bool shift=false,cursor=false,open=false;};
struct Owned {
    GridSnapshot grid;std::array<std::wstring,7> people;std::array<std::wstring,8> stats;
    std::array<std::wstring,256> itemNames;std::array<unsigned,256> stock{};unsigned epoch=0,revision=0;bool open=false;
};
ProcessStorage<std::array<Frame,Slots>> frames;
ProcessStorage<Facts> immutable;
ProcessStorage<Owned> state;
std::array<std::atomic<unsigned>,Slots> tickets{};
std::atomic<unsigned> revision{1},nextToken{0};
std::atomic<unsigned> clearRevision{0};
unsigned clearedRevision=0;
std::atomic<bool> started{false};
std::atomic<bool> opening{false};
std::atomic<DWORD> preparationThread{0};
thread_local const Owned* context=nullptr;
thread_local const speechq::Header* origin=nullptr;
template<unsigned N> void text(Encoded<N>& to,uintptr_t from) {to.length=copyReadyGameText(from,to.bytes.data(),N);}
uintptr_t record(unsigned getter,unsigned id,uintptr_t& strings) {
    using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);strings=0;return reinterpret_cast<Getter>(address(getter))(id,&strings);
}
bool capture(Facts& out) {
    out.valid=false;const auto grid=read<uintptr_t>(address(0x1f05834));
    const int count=read<int16_t>(grid+2,-1),links=read<int16_t>(grid+4,-1);
    if(!grid || !sphereGridOpen() || count<1 || count>1024 || links<0 || links>1024) return false;
    out.selected=read<int16_t>(grid+0x112fc,-1);out.character=read<uint8_t>(grid+0x115bc,255);
    if(out.selected<0 || out.selected>=count || out.character<0 || out.character>=7) return false;
    out.epoch=generation();out.revision=revision.load(std::memory_order_acquire);out.grid=grid;out.definitions=read<uintptr_t>(address(0x16860e0));out.itemDefinitions=read<uintptr_t>(address(0x16860e4));
    const auto nativeLanguage=reinterpret_cast<unsigned(__cdecl*)()>(address(0x3852b0))();out.language=nativeLanguage&1;
    if(!out.definitions || !out.itemDefinitions) return false;
    prepareCopiedControlSnapshot(out.controls);
    auto& cache=immutable.get();
    // The static catalog is reused only for the exact physical layout, language
    // and resource generation. Live positions/activation/history/inventory below
    // are always copied for this request in one owner observation.
    bool same=cache.valid && cache.epoch==out.epoch && cache.revision==out.revision && cache.grid==grid && cache.definitions==out.definitions &&
        cache.itemDefinitions==out.itemDefinitions && cache.language==out.language && cache.nodeCount==static_cast<unsigned>(count);
    if(same) for(int i=0;i<count;++i) if(cache.nodes[i].type!=read<int16_t>(grid+0x80e+i*0x28,-1)) {same=false;break;}
    using Definition=uintptr_t(__cdecl*)(unsigned,uintptr_t,uintptr_t*);
    const auto definition=reinterpret_cast<Definition>(address(0x3ab890));
    for(unsigned i=0;i<out.items.size();++i) {
        auto& item=out.items[i];if(same) item=cache.items[i];else {
            item.key=0;uintptr_t strings=0;const auto native=record(0x390a40,i,strings);
            text(item.name,native && strings?strings+read<uint16_t>(native+out.language*4):0);
            const int type=read<int8_t>(native+0x5d,-1);const auto material=type<0?0:definition(type,out.itemDefinitions,nullptr);
            if(material) {const auto special=read<uint8_t>(material+0xd);item.key=special>=1 && special<=4?0x10000u+special:!special?read<uint16_t>(material+0xa):0;}
        }
        item.stock=quantity(0x2000u+i);
    }
    for(int i=0;i<count;++i) {
        auto& node=out.nodes[i];const auto live=grid+0x808+i*0x28;
        if(same) node=cache.nodes[i];else {
            node.valid=false;node.flags=node.ability=node.key=0;node.type=read<int16_t>(live+6,-1);node.name.length=node.description.length=node.abilityName.length=node.abilityDescription.length=0;
            uintptr_t strings=0;const auto native=node.type<0?0:definition(node.type,out.definitions,&strings);
            if(native && strings) {
                node.valid=true;node.flags=read<uint16_t>(native+0x10);node.ability=read<uint16_t>(native+0x12);
                text(node.name,strings+read<uint16_t>(native+out.language*4));text(node.description,strings+read<uint16_t>(native+8+out.language*4));
                if(node.ability) {uintptr_t commandStrings=0;const auto command=record(0x39a4c0,0x3000u|(node.ability&0xfff),commandStrings);
                    if(command && commandStrings) {text(node.abilityName,commandStrings+read<uint16_t>(command+out.language*4));text(node.abilityDescription,commandStrings+read<uint16_t>(command+8+out.language*4));}}
                const auto lock=read<uint8_t>(native+0x16,255);node.key=node.flags&0x8000?lock==0x12?0x10001:lock==0x11?0x10002:lock==0?0x10003:lock==0x10?0x10004:0:node.flags?node.flags&(~node.flags+1u):0;
            }
        }
        node.x=read<int16_t>(live);node.y=read<int16_t>(live+2);node.activated=read<uint8_t>(live+0x21);
    }
    out.nodeCount=count;out.linkCount=links;
    for(int i=0;i<links;++i) {const auto link=grid+0xa808+i*0x14;auto& row=out.links[i];row={read<int16_t>(link,-1),read<int16_t>(link+2,-1),read<uint8_t>(link+0xc)};
        if(row.a<0 || row.b<0 || row.a>=count || row.b>=count || row.a==row.b) return false;}
    using Saved=uintptr_t(__cdecl*)(unsigned);using Person=uintptr_t(__cdecl*)(unsigned);using Menu=uintptr_t(__cdecl*)(int,unsigned,unsigned);
    for(unsigned i=0;i<7;++i) {out.positions[i]=read<int16_t>(grid+0x110cc+i*0x50,-1);const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(i);out.levels[i]=saved?read<uint8_t>(saved+0x3b):0;text(out.people[i],reinterpret_cast<Person>(address(0x385070))(i));}
    for(unsigned i=0;i<8;++i) text(out.stats[i],reinterpret_cast<Menu>(address(0x38fd40))(11,i+7,nativeLanguage));
    out.valid=out.nodes[out.selected].valid && out.epoch==generation();
    if(out.valid && !same) {cache.epoch=out.epoch;cache.revision=out.revision;cache.grid=grid;cache.definitions=out.definitions;cache.itemDefinitions=out.itemDefinitions;cache.language=out.language;cache.nodeCount=out.nodeCount;
        std::copy(out.nodes.begin(),out.nodes.begin()+out.nodeCount,cache.nodes.begin());cache.items=out.items;cache.valid=true;}
    return out.valid;
}
template<unsigned N> std::wstring decode(const Encoded<N>& encoded,const OwnedTextContext& controls,bool lines=false) {
    if(encoded.length>N) return {};const auto parts=decodeOwnedBytes(encoded.bytes.data(),encoded.length,OwnedTextMode::Terminated,controls);return lines?join(parts,L" "):parts.empty()?std::wstring{}:parts.front();
}
void restore(const Facts& facts,Owned& to) {
    if(facts.nodeCount>facts.nodes.size() || facts.linkCount>facts.links.size() || facts.character<0 || facts.character>=7 || facts.selected<0 || static_cast<unsigned>(facts.selected)>=facts.nodeCount) throw nav::ComputationCancelled{};
    to.grid={};to.grid.selected=facts.selected;to.grid.character=facts.character;to.grid.positions=facts.positions;to.grid.levels=facts.levels;
    const auto controls=copiedControlContext(facts.controls);
    for(unsigned i=0;i<facts.items.size();++i) {to.itemNames[i]=decode(facts.items[i].name,controls);to.stock[i]=facts.items[i].stock;}
    for(unsigned i=0;i<7;++i) to.people[i]=decode(facts.people[i],controls);for(unsigned i=0;i<8;++i) to.stats[i]=decode(facts.stats[i],controls);
    to.grid.nodes.reserve(facts.nodeCount);
    for(unsigned i=0;i<facts.nodeCount;++i) {nav::checkComputation();const auto& row=facts.nodes[i];GridNode node;node.x=row.x;node.y=row.y;node.type=row.type;node.valid=row.valid;
        node.flags=row.flags;node.locked=(row.flags&0x8000)!=0;node.ability=row.ability;node.activated=row.activated;
        node.name=decode(row.name,controls);node.description=decode(row.description,controls,true);
        const auto ability=decode(row.abilityName,controls),detail=decode(row.abilityDescription,controls,true);if(!ability.empty()) node.name=ability;if(!detail.empty()) node.description=detail;
        for(unsigned item=0;item<facts.items.size();++item) {const auto key=facts.items[item].key;
            if(row.key && (row.key>=0x10000?key==row.key:key<0x10000 && (key&row.key))) {node.sphere=0x2000u+item;break;}}
        to.grid.nodes.push_back(std::move(node));}
    for(unsigned i=0;i<facts.linkCount;++i) {const auto& link=facts.links[i];if(link.a<0 || link.b<0 || static_cast<unsigned>(link.a)>=facts.nodeCount || static_cast<unsigned>(link.b)>=facts.nodeCount) throw nav::ComputationCancelled{};if(to.grid.nodes[link.a].valid && to.grid.nodes[link.b].valid) to.grid.links.push_back(link);}
}
void release(unsigned slot,unsigned token) {if(slot<Slots) tickets[slot].compare_exchange_strong(token,0,std::memory_order_release);}
void retire() {
    preparationThread.store(GetCurrentThreadId(),std::memory_order_release);
    const auto current=revision.load(std::memory_order_acquire);
    for(unsigned i=0;i<Slots;++i) {const auto token=tickets[i].load(std::memory_order_acquire);if(token && !(token&Writing) && (frames.get()[i].revision!=current || frames.get()[i].facts.epoch!=generation())) release(i,token);}
    auto& owned=state.get();if(owned.revision && (owned.revision!=current || owned.epoch!=generation())) {owned.revision=0;resetSpherePlanner();owned.grid={};owned.open=false;}
    const auto clear=clearRevision.load(std::memory_order_acquire);if(clear && clear!=clearedRevision) {clearSphereTargets();clearedRevision=clear;}
}
void consume(const speechq::Header& header,const std::wstring& data) {
    preparationThread.store(GetCurrentThreadId(),std::memory_order_release);
    retire(); // Apply same-generation reset/clear before admitting fresh work.
    if(data.size()*sizeof(wchar_t)!=sizeof(Packet)) return;Packet packet;std::memcpy(&packet,data.data(),sizeof(packet));
    struct Return {unsigned slot,token;~Return(){release(slot,token);}} returned{packet.slot,packet.token};
    if(packet.epoch!=generation() || packet.revision!=revision.load(std::memory_order_acquire)) return;
    auto& owned=state.get();nav::ComputationScope cancellation(revision,packet.revision);
    if(packet.slot<Slots) {if(tickets[packet.slot].load(std::memory_order_acquire)!=packet.token) return;const auto& frame=frames.get()[packet.slot];if(!frame.facts.valid || frame.revision!=packet.revision) return;restore(frame.facts,owned);}
    owned.epoch=packet.epoch;owned.revision=packet.revision;owned.open=packet.open;
    struct Scope {const Owned* previous;const speechq::Header* prior;Scope(Owned& value,const speechq::Header& h):previous(context),prior(origin){context=&value;origin=&h;}~Scope(){context=previous;origin=prior;}} scope(owned,header);
    if(packet.work==SphereWork::Abilities || packet.work==SphereWork::Paths) opening=false;
    switch(packet.work) {
    case SphereWork::Abilities:openSpherePlanner(packet.shift);break;
    case SphereWork::Paths:openSpherePathMenu(packet.shift);break;
    case SphereWork::Key:spherePlannerKey(packet.key,packet.shift);break;
    case SphereWork::Section:sphereSectionKey(packet.shift,packet.cursor);break;
    case SphereWork::Orientation:sphereOrientationKey(packet.shift);break;
    case SphereWork::Clear:clearSphereTargets();break;
    case SphereWork::Reset:resetSpherePlanner();break;
    }
}
}
void startSphereWork() {if(started.load()) return;frames.prepare();immutable.prepare();state.prepare();speechq::registerDomain(Domain,consume,[](unsigned){retire();});speechq::registerDomainMaintenance(Domain,retire);started.store(true,std::memory_order_release);}
bool queueSphereWork(SphereWork work,bool shift,unsigned key,bool cursor) {
    if(!nativeGameThread()) return !spherePreparationOwner();
    if(!started.load(std::memory_order_acquire)) {output(L"Sphere Grid preparation is unavailable",true,"sphere-pressure");return true;}
    Packet packet;packet.work=work;packet.epoch=generation();packet.revision=revision.load();packet.shift=shift;packet.key=key;packet.cursor=cursor;packet.open=sphereGridOpen();
    if(work==SphereWork::Abilities || (work==SphereWork::Paths && !shift) || work==SphereWork::Section || work==SphereWork::Orientation) {
        unsigned token=(nextToken.fetch_add(1)+1)&~Writing;if(!token) token=(nextToken.fetch_add(1)+1)&~Writing;
        for(unsigned i=0;i<Slots;++i) {unsigned empty=0;if(tickets[i].compare_exchange_strong(empty,token|Writing,std::memory_order_acquire)) {packet.slot=i;break;}}
        if(packet.slot==Slots) {output(L"Sphere Grid preparation is busy. Try the command again after it recovers.",true,"sphere-pressure");return true;}
        auto& frame=frames.get()[packet.slot];frame.revision=packet.revision;
        if(!capture(frame.facts)) {release(packet.slot,token|Writing);output(L"Open the Sphere Grid for path guidance.",true,"sphere-section");return true;}
        packet.token=token;tickets[packet.slot].store(token,std::memory_order_release);
    }
    const bool reserving=(work==SphereWork::Abilities || (work==SphereWork::Paths && !shift)) && !spherePlannerActive();
    if(reserving) {opening=true;captureSpeech(true);drainController();pauseNavAudio();}
    if(!speechq::captureDomain(Domain,packet,packet.epoch)) {release(packet.slot,packet.token);if(reserving) {opening=false;captureSpeech(false);}output(L"Sphere Grid preparation is busy. Try the command again after it recovers.",true,"sphere-pressure");}
    return true;
}
void cancelSphereWork(bool clear) {opening=false;const auto next=revision.fetch_add(1,std::memory_order_acq_rel)+1;if(clear) clearRevision.store(next,std::memory_order_release);speechq::notifyPreparation();}
bool spherePreparationOwner() {return !nativeGameThread() && preparationThread.load(std::memory_order_acquire)==GetCurrentThreadId();}
bool sphereOpeningPending() {return opening.load(std::memory_order_acquire);}
bool sphereCopiedSnapshot(GridSnapshot& grid) {if(!context) return false;grid=context->grid;return context->open;}
bool sphereCopiedOpen(bool& open) {if(!context) return false;open=context->open;return true;}
std::wstring spherePlayerLabel(int id) {return context?id>=0 && id<7?context->people[id]:std::wstring{}:playerName(id);}
std::wstring sphereItemLabel(unsigned id) {return context?id>=0x2000 && id<0x2100?context->itemNames[id-0x2000]:std::wstring{}:recordName(0x390a40,id);}
unsigned sphereStock(unsigned id) {return context?id>=0x2000 && id<0x2100?context->stock[id-0x2000]:0:quantity(id);}
std::wstring sphereStatLabel(int stat) {return context?stat>=0 && stat<8?context->stats[stat]:stat==8?L"HP":L"MP":stat<8?menuText(11,stat+7):stat==8?L"HP":L"MP";}
bool sphereEmit(std::wstring text,bool interrupt,const char* producer) {return origin?speechq::emitOwned(std::move(text),*origin,producer,false,interrupt):output(text,interrupt,producer);}
#ifdef FFX_READER_TEST
bool captureSphereFactsForTest() {if(!nativeGameThread() || !started.load()) return false;for(const auto& ticket:tickets) if(ticket.load()) return false;return capture(frames.get()[0].facts);}
bool sphereOwnedGridForTest(GridSnapshot& grid) {if(!spherePreparationOwner()) return false;grid=state.get().grid;return true;}
#endif
}
