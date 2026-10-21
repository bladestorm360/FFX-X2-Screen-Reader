#include "owned_retirement.hpp"
#include "navigation.hpp"
#include "butterfly.hpp"
#include "atel.hpp"
#include "navigation_targets.hpp"
#include "room_areas.hpp"
#include "swimming_navigation.hpp"
#include "exit_labels.hpp"
#include "process_storage.hpp"
#include "text_plan.hpp"
#include "nonblocking_snapshots.hpp"
#include <map>
#include <cmath>

namespace ffx {
namespace {
using ScriptMap = std::map<uintptr_t, AtelScript>;
ProcessStorage<ScriptMap> scriptsStorage;
ScriptMap& scripts() { return scriptsStorage.get(); }
unsigned scriptEpoch=0;
struct Arrival { unsigned epoch=0,room=0,entrance=0; uintptr_t chunk=0;unsigned serial=0,revision=0;bool submitted=false; };
Arrival pendingArrival;
std::atomic<unsigned> arrivalRevision{1};
struct ArrivalReply {unsigned epoch=0,serial=0,named=0;};
async::AtomicSnapshot<ArrivalReply> arrivalReply;
unsigned nextArrivalSerial=0;
constexpr unsigned ArrivalReceiver=7;
std::wstring areaNameIndex(int index) {
    if (index < 0) return {};
    using Table = uintptr_t(__cdecl*)(unsigned);
    using Name = uintptr_t(__cdecl*)(unsigned, unsigned);
    const auto table = reinterpret_cast<Table>(address(0x4b7f10))(0);
    if (!table || index >= (read<uint16_t>(table) >> 2)) return {};
    const auto encoded = reinterpret_cast<Name>(address(0x46c3c0))(index, 0);
    if (!encoded) return {};
    return clean(first(encoded,256));
}
std::wstring areaName(int room) { return areaNameIndex(areaIndex(room)); }
int currentAreaIndex() {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    if(!controller) return -1;
    // Native loader 86EDA0 publishes the primary field script chunk at +48;
    // +1F8 belongs to the separate segmented-script pointer array.
    const auto chunk=read<uintptr_t>(controller+0x48);
    AtelSpan span;if(!nativeAtelSpan(chunk,span) || span.size<0x2c) return -1;
    const auto begin=read<unsigned>(chunk+4),end=read<unsigned>(chunk+8);
    const unsigned entrance=read<uint8_t>(address(0xd2ca90)+0xc);
    if(!begin || end<begin || end>span.size || (end-begin)%32 || entrance>=(end-begin)/32) return -1;
    // 46BD20 returns geometry MapNo, not an area-name ID. Native 470E90
    // publishes the actual collision/ATEL-selected area in saved field +E.
    const auto index=read<uint16_t>(address(0xd2ca90)+0xe);
    const auto count=read<uint16_t>(chunk+0x1e);
    if(!(count&0x8000)) return count==index?index:-1;
    const auto offset=read<unsigned>(chunk+0x28);
    if(!offset || offset>span.size || !(count&0x7fff) || (count&0x7fff)>256 || (count&0x7fffu)*2>span.size-offset) return -1;
    for(unsigned i=0;i<(count&0x7fffu);++i) if(read<uint16_t>(chunk+offset+i*2)==index) return index;
    return -1;
}
TextRef planAreaName(TextPlan& p,int index) {
    if(index<0) return 0;
    using Table=uintptr_t(__cdecl*)(unsigned);using Name=uintptr_t(__cdecl*)(unsigned,unsigned);
    const auto table=reinterpret_cast<Table>(address(0x4b7f10))(0);
    if(!table || index>=(read<uint16_t>(table)>>2)) return 0;
    return planEncoded(p,reinterpret_cast<Name>(address(0x46c3c0))(static_cast<unsigned>(index),0),256);
}
void receiveArrival(const speechq::Header& header,const TextPlan& p,std::wstring name,std::wstring) {
    Arrival copied;static_assert(sizeof(copied)<=sizeof(p.context));std::memcpy(&copied,p.context.data(),sizeof(copied));
    if(copied.epoch!=generation() || p.epoch!=generation() || copied.revision!=arrivalRevision.load(std::memory_order_acquire)) return;
    if(!name.empty()) speechq::emitOwned(L"Entering "+name,header,"map-transition",false,false);
    arrivalReply.publish({copied.epoch,copied.serial,name.empty()?0u:1u});
}
}
std::wstring nativeAreaName(int index) { return areaNameIndex(index); }
void announceCurrentMap() {
    TextPlan p;prepareTextPlan(p,"current-map");
    const auto fallback=planAreaName(p,areaIndex(read<uint16_t>(address(0xd2ca90))));
    const auto primary=planAreaName(p,currentAreaIndex());
    p.headline=planPair(p,TextPlanOp::Or,planPair(p,TextPlanOp::Or,primary,fallback),planLiteral(p,L"No native map name is available."));
    commitTextPlan(p);
}
void announceMapArrival(Event& e) {
    pendingArrival={};
    const auto room=read<uint16_t>(address(0xd2ca90),0xffff);
    // Room 23 is the title/save-load scene. A request is not an arrival; read
    // the completed scene's actual identity, including its current entrance.
    if(room==23 || e.args[0]!=room || inBattle()) return;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const auto chunk=read<uintptr_t>(controller+0x48);
    if(!controller || !chunk) return;
    pendingArrival={generation(),room,read<uint8_t>(address(0xd2ca90)+0xc),chunk};
    pendingArrival.revision=arrivalRevision.load(std::memory_order_acquire);
    finishMapArrival();
}
void finishMapArrival() {
    if(!nativeGameThread()) return;
    if(pendingArrival.epoch && pendingArrival.revision!=arrivalRevision.load(std::memory_order_acquire)) {pendingArrival={};return;}
    if(!pendingArrival.epoch) return;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    if(pendingArrival.epoch!=generation() || inBattle() ||
        pendingArrival.room!=read<uint16_t>(address(0xd2ca90)) ||
        pendingArrival.entrance!=read<uint8_t>(address(0xd2ca90)+0xc) ||
        pendingArrival.chunk!=read<uintptr_t>(controller+0x48)) { pendingArrival={}; return; }
    if(!activeField()) return;
    if(pendingArrival.submitted) {
        ArrivalReply reply;
        if(!arrivalReply.tryRead(reply) || reply.epoch!=pendingArrival.epoch || reply.serial!=pendingArrival.serial) return;
        if(reply.named) {pendingArrival={};return;}
        pendingArrival.submitted=false;
    }
    // A saved room fallback can name a map even at the title screen.
    TextPlan p;prepareTextPlan(p,"map-transition",false);p.receiver=ArrivalReceiver;
    p.headline=planAreaName(p,currentAreaIndex());if(!p.headline) return;
    pendingArrival.serial=++nextArrivalSerial?nextArrivalSerial:++nextArrivalSerial;
    pendingArrival.revision=arrivalRevision.load(std::memory_order_acquire);
    std::memcpy(p.context.data(),&pendingArrival,sizeof(pendingArrival));
    pendingArrival.submitted=commitTextPlan(p);
}
void scanExits(Point player, std::vector<Entity>& entities) {
    const auto controller = read<uintptr_t>(address(0xf26ae8));
    const auto count = read<uint16_t>(controller + 0xc);
    if (!controller || !count || count > 500) return;
    if(scriptEpoch!=generation()) { scripts().clear(); scriptEpoch=generation(); }
    const auto actors = read<uintptr_t>(address(0x1fc44e4));
    const unsigned capacity = std::min(128u, read<unsigned>(address(0x1fc44e0), 128));
    const wchar_t* categories[] = {L"",L"Exit",L"Event",L"Save Sphere",L"Treasure Chest",L"NPC",L"Object",L"Shop"};
    unsigned excluded = 0;
    using Worker = uintptr_t(__cdecl*)(int);
    for (unsigned i = 0; i < count; ++i) try {
        const auto worker = reinterpret_cast<Worker>(address(0x46a7c0))(i);
        const auto header = read<uintptr_t>(worker);
        if (!worker || !header) continue;
        const auto type = read<uint8_t>(header);
        if (type != 1 && type != 2 && type != 3 && type != 4 && type != 5 && type != 6) continue;
        const auto chunk = read<uintptr_t>(worker + 4);
        if (!scripts().count(chunk)) {
            if(scripts().size()>=4) scripts().erase(scripts().begin());
            scripts().emplace(chunk, readScript(chunk));
        }
        const auto& script = scripts().at(chunk);
        unsigned index = 0;
        while (index < script.workers.size() && script.workers[index].header != header - chunk) ++index;
        if (index == script.workers.size()) continue;
        const auto catalog = navigationCatalog(script);
        if(type==4) {
            Entity goal; goal.worker=i;
            if(configureSwimmingGoal(goal,catalog?catalog->name:nullptr,index,worker)) {
                const auto name=areaName(288);
                if(!name.empty()) goal.name=L"Exit to "+name;
                entities.push_back(std::move(goal));
            }
            continue;
        }
        const auto target = navigationTarget(catalog, index);
        const auto actor = type == 1 ? read<uintptr_t>(worker + 0x9c) : 0;
        const bool bound = actors && actor >= actors && actor!=playerActor() && (actor - actors) % 0x880 == 0 && (actor - actors) / 0x880 < capacity;
        const unsigned model = bound ? read<uint16_t>(actor) : 0;
        const bool presence=navigationActorPresence(catalog,model,bound);
        const bool saveSphere=bound && navigationSaveSphereModel(model);
        const bool nativeObject=saveSphere || (bound && model==0x5002);
        unsigned channels=0;
        const auto usefulEntries=saveSphere || presence?EntryMask{}:target?target->entries:EntryMask{};
        const auto entries=interactionEntries(worker,usefulEntries,&channels);
        if(presence) channels&=1u; // Only Cid's actual current Talk channel is an interaction.
        const bool inactive=presence?!channels:entries.empty();
        const bool trialControl=target && target->identity==2 && target->category==6 &&
            (script.name=="bsyt0100" || script.name=="klyt1000" || script.name=="djyt0600" ||
             script.name=="mcyt0500" || script.name=="bvyt1100" || script.name=="dome0700");
        const bool entryRegion=target && target->identity==3 && type==6;
        if(inactive && !trialControl && !entryRegion && !presence) { ++excluded; continue; }
        Entity entity; entity.worker = i; entity.inactive=inactive;
        // Only these hash-qualified underwater controls use a horizontal
        // approach while native swimming physics supplies the final height.
        entity.swimApproach=catalog && script.name=="stbv0000" && (index==16 || index==17 || index==25 || index==26);
        entity.usefulEntries=usefulEntries; entity.authoredTrialControl=trialControl;
        if (bound && (!read<uint16_t>(actor + 2) || read<unsigned>(actor + 0x824) == 0xcdcdcdcd)) continue;
        if (type == 1 && !bound) continue;
        int room = -1;
        if (nativeObject) {
            if(model==0x5002 && !target) {
                const auto result=scanWorker(script,index,&entries);
                if(!result.interactive && !result.request && !result.exit && !result.truncated && !result.unresolved) { ++excluded; continue; }
            }
            entity.category = entity.name = saveSphere ? L"Save Sphere" : L"Treasure Chest";
        } else if (target || presence) {
            entity.category = categories[navigationEntityCategory(target,model,bound)]; room = target?target->room:-1;
            entity.name=navigationEntityName(target,model,bound);
        } else if (!catalog) {
            // Unknown/changed scripts retain proven direct interactions.
            // An arbitrary REQ alone never creates a navigation event.
            const auto result = scanWorker(script, index, &entries);
            if (result.exit && type != 1) { entity.category = L"Exit"; entity.name = L"Exit"; room = result.room; }
            else if (result.interactive) { entity.category = categories[navigationEntityCategory(nullptr,model,bound)]; entity.name = navigationEntityName(nullptr,model,bound); }
            else { ++excluded; continue; }
        } else { ++excluded; continue; }
        const unsigned geometry = type == 5 || type == 6 ? 0x284 : 0x558;
        const auto coordinates = bound ? actor + 0xc : worker + geometry;
        Point p{read<float>(coordinates),read<float>(coordinates + 4),read<float>(coordinates + 8)};
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) continue;
        entity.position = p; entity.distance = distanceXZ(player,p);
        // Disabled authored trial controls and present dedicated airship Cid
        // retain Talk geometry for identification; inactivity still blocks Talk.
        // Qualified entry regions instead retain their native crossing geometry.
        configurePedestalTarget(entity,catalog?catalog->name:nullptr,index);
        refreshTargetZone(entity,worker,channels?channels:trialControl || presence?1u:entryRegion?0x20u:0u);
        entity.actor = bound ? (actor - actors) / 0x880 : -1;
        entity.id = bound ? model : entity.category == L"Exit" ? 0xffff : 0xfffe;
        configureButterflyStarter(entity,catalog?catalog->name:nullptr,index,worker);
        if (entity.category == L"Exit") {
            if (room != -2) {
                if (room < 0) room = scanWorker(script,index,&entries).room;
                entity.name=navigationExitName(script,index,room,entity.name);
            }
            // room -2 denotes a verified same-map airship passage; keep its stable authored label.
            recordLog("exit worker=%u room=%d",i,room);
        }
        bool duplicate = false;
        for (const auto& previous : entities)
            if (bound && previous.actor == entity.actor) { duplicate = true; break; }
        if (duplicate) continue;
        Entity recess;
        if(pedestalRecessTarget(entity,worker,recess)) entities.push_back(std::move(recess));
        entities.push_back(std::move(entity));
    } catch (...) { recordLog("navigation worker scan failed idx=%u", i); }
    recordLog("navigation ambient workers omitted=%u",excluded);
}
void resetNavigationScripts() { arrivalRevision.fetch_add(1,std::memory_order_acq_rel);if(!nativeGameThread()) return;if(scriptsStorage.ready()) retirement::retire(scripts()); scriptEpoch=0; pendingArrival={}; }
void prepareNavigationScriptRetirement() {
    scriptsStorage.prepare();retirement::prepare<ScriptMap>();registerTextPlanReceiver(ArrivalReceiver,receiveArrival);
}
}
