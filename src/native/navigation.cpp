#include "navigation.hpp"
#include "airship_map.hpp"
#include "navigation_scene.hpp"
#include "navigation_targets.hpp"
#include "navigation_observations.hpp"
#include "navigation_intents.hpp"
#include "navigation_cancel.hpp"
#include "camera_direction.hpp"
#include "process_storage.hpp"
#include "reachability.hpp"
#include "reachability_shared.hpp"
#include "nav_format.hpp"
#include "nav_audio.hpp"
#include "auto_walk.hpp"
#include "sphere_grid.hpp"
#include "settings.hpp"
#include "pedestal.hpp"
#include "butterfly_navigation.hpp"
#include "swimming_navigation.hpp"
#include "map_transition.hpp"
#include "wall_cues.hpp"
#include "text_plan.hpp"
#include "puzzle_guide_owned.hpp"
#include "puzzle_guide_targets.hpp"
#include "utilities.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <optional>
namespace ffx {
namespace nav {
void stageAirshipPassages(const Scene& copiedScene,const Mesh& mesh,std::vector<Entity>& entities) {
    if(nativeGameThread() || copiedScene.epoch!=generation() || copiedScene.count>SceneWorkers || copiedScene.partCount>SceneParts ||
        !copiedScene.allowed || !copiedScene.control || !copiedScene.controlled || (mesh.triangles.empty() && !mesh.openPlane)) return;
    struct Context {const Scene& scene;const Mesh& mesh;std::vector<Entity>& entities;unsigned part;};
    for(unsigned part=0;part<copiedScene.partCount;++part) {
        Context context{copiedScene,mesh,entities,part};
        visitReadyAtelScript(copiedScene.parts[part].span,[](const AtelScript& script,void* opaque) {
            auto& c=*static_cast<Context*>(opaque);if(!navigationCatalog(script)) return;
            const unsigned deckLocal=script.name=="hiku0000"?25:script.name=="hiku2000"?13:~0u;
            if(deckLocal==~0u) return;
            int deck=-1,up=-1;
            for(unsigned i=0;i<c.scene.count;++i) {const auto& worker=c.scene.workers[i];if(worker.valid && worker.global==i && worker.part==c.part) {
                if(worker.local==deckLocal) deck=worker.global;
                if(worker.local==deckLocal+4) up=worker.global;
            }}
            auto& rows=c.entities;
            const auto door=std::find_if(rows.begin(),rows.end(),[&](const Entity& e){return e.worker==deck && e.category==L"Exit" && !e.inactive && e.zone.valid;});
            const auto lift=std::find_if(rows.begin(),rows.end(),[&](const Entity& e){return e.worker==up && e.category==L"Exit" && !e.inactive && e.zone.valid;});
            if(door==rows.end() || lift==rows.end()) return;
            // Stage only when the copied floor actually reaches the live lift
            // but cannot reach the deck trigger. No inferred floor/height or
            // synthetic cross-floor path; every consumer gets the same lift.
            if(!navigationRouteOwned(c.mesh,c.scene.player,*door,rows).empty() ||
                navigationRouteOwned(c.mesh,c.scene.player,*lift,rows).empty()) return;
            lift->name=L"Elevator up toward the deck. Confirm to ride, then select the deck exit again";
            rows.erase(door);
        },&context);
    }
}
}
namespace {
constexpr double UnitsPerStep=11.13;
const wchar_t* categories[]={L"All",L"Exit",L"Event",L"Save Sphere",L"Treasure Chest",L"Shop",L"NPC",L"Object"};
struct EntityKey {
    bool valid=false,workerKind=false;int index=-1;unsigned id=0,pushControl=0;
    bool operator==(const EntityKey& b) const {return valid==b.valid && (!valid || (workerKind==b.workerKind && index==b.index && id==b.id && pushControl==b.pushControl));}
    bool operator<(const EntityKey& b) const {return std::tie(valid,workerKind,index,id,pushControl)<std::tie(b.valid,b.workerKind,b.index,b.id,b.pushControl);}
};
EntityKey key(const Entity& e) {
    // Cid's dedicated model identifies one live actor in the scene. Retain
    // selection when native story staging transfers him to another worker or
    // actor slot; route/intention ownership still revalidates the fresh worker.
    if(e.actor>=0 && e.id==0x2101 && e.category==L"NPC") return {true,false,-1,e.id,0};
    return {true,e.worker>=0,e.worker>=0?e.worker:e.actor,e.id,e.pushControl};
}
struct Pending {speechq::Header header;nav::QueryObservation query;std::optional<QueuedPuzzleTarget> direct;};
struct Owned {
    nav::Scene observation;Mesh mesh;std::vector<Entity> entities;std::deque<Pending> pending;
    std::map<int,std::wstring> areas;nav::AreaNameDemand names;
    EntityKey selection;unsigned epoch=0,revision=0,requested=0,category=0,selected=0;bool ready=false,haveMesh=false;
    nav::NativeGeometryContext meshContext;
};
ProcessStorage<Owned> storage;
std::atomic<bool> interested{false};
bool blocked=false;
Owned& owned() {return storage.get();}
std::vector<const Entity*> filtered() {
    std::vector<const Entity*> rows;const auto& s=owned();
    for(const auto& e:s.entities) if((!setting(Setting::HideUnreachable) || !e.unreachable) && (!s.category || e.category==categories[s.category])) rows.push_back(&e);
    return rows;
}
std::wstring label(const Entity& e) {return e.name.empty()?e.category:e.name;}
std::wstring decimal(double value) {wchar_t text[40]{};swprintf_s(text,L"%.1f",value);return text;}
void emit(const Pending& event,std::wstring text,const char* producer,bool interrupt=true) {speechq::emitOwned(std::move(text),event.header,producer,false,interrupt);}
void lockSelected() {
    auto& s=owned();const auto rows=filtered();s.selection=s.selected<rows.size()?key(*rows[s.selected]):EntityKey{};interested.store(true,std::memory_order_release);
}
std::wstring instruction(const Entity& target) {
    auto& s=owned();PedestalPlan plan;if(!nav::scenePedestalPlan(s.observation,target,plan)) return L"The current pedestal state is unavailable.";
    auto controls=s.observation.controls;controls.swap=false;const auto context=copiedControlContext(controls);
    const bool single=!plan.moves.empty() && plan.moves.front().mode==PushMode::Single;
    const unsigned raw=single?s.observation.confirmMask:0x40;
    static constexpr unsigned masks[]={0x10,0x20,0x40,0x80,4,8,1,2,0x800,0x100};
    std::vector<std::wstring> names;for(unsigned i=0;i<_countof(masks);++i) if(raw&masks[i]) {const auto& name=context.glyphs[0x30+i];if(!name.empty() && name!=L"unassigned control") names.push_back(name);}
    for(unsigned i=0;i<4;++i) if(raw&(0x1000u<<i)) {const auto& name=context.glyphs[0x40+(1u<<i)];if(!name.empty() && name!=L"unassigned control") names.push_back(name);}
    const auto binding=join(names,L" or ");
    const auto action=binding.empty() || binding==L"unassigned control"?single?L"Confirm":L"the configured push control":binding;
    const auto side=describePedestalApproach(plan,target.position,s.observation.player);
    return (side.empty()?L"":side+L" ")+describePedestalPlan(plan,target.position,action);
}
void announce(const Pending& event,bool interrupt=true,bool index=true) {
    auto& s=owned();const auto rows=filtered();if(rows.empty()) return;if(s.selected>=rows.size()) s.selected=0;
    const auto& target=*rows[s.selected];const auto player=s.observation.player;
    auto text=label(target)+(target.swimDepth?L". "+nav::sceneSwimmingInstruction(s.observation):(index?L", ":L": ")+decimal(distanceXZ(player,target.position)/UnitsPerStep)+L" steps "+direction({target.position.x-player.x,0,target.position.z-player.z}));
    if(target.inactive) text+=L", currently inactive";if(target.pushControl) {const auto side=describePedestalSide(target.position,player);if(!side.empty()) text+=L". "+side;}
    if(index) text+=menuIndex(s.selected,static_cast<unsigned>(rows.size()));emit(event,std::move(text),"navigation-selection",interrupt);
}
void intent(const Pending& event,const Entity& target,nav::Intent kind) {
    if(!nav::submitNavigationIntent(owned().observation,target,kind,event.query.manual,event.query.intentRevision))
        emit(event,L"Navigation action is unavailable or busy. Request the route again after it recovers.","navigation-pressure",false);
}
void beacon(const Pending&) {
    auto& s=owned();if(!setting(Setting::Beacon) || navRouteKnown() || !s.selection.valid || !s.observation.allowed) return;
    const auto rows=filtered();if(s.selected>=rows.size()) return;const auto& target=*rows[s.selected];if(target.inactive) return;
    const auto points=navigationRouteOwned(s.mesh,s.observation.player,target,s.entities);if(!points.empty()) routeNavAudio(points,target);
}
void pathfind(const Pending& event,bool assist=false) {
    auto& s=owned();const auto rows=filtered();const Entity* selected=nullptr;
    if(assist) {for(const auto& e:s.entities) if(s.observation.pedestal.armed && e.worker==s.observation.pedestal.worker && e.id==s.observation.pedestal.id && e.pushControl==s.observation.pedestal.control && e.pushLocal==s.observation.pedestal.local) {selected=&e;break;}}
    else if(s.selected<rows.size()) selected=rows[s.selected];
    if(!selected) {if(!assist) emit(event,L"No entity selected","navigation-path");return;}
    const auto target=*selected;const auto player=s.observation.player;
    if(!s.mesh.events.ready) {resetNavAudio();emit(event,L"Navigation data is unavailable. Waiting for the field to become ready.","navigation-data-unavailable");return;}
    if(!assist) lockSelected();if(target.hazardous) {emit(event,label(target)+L": avoid this butterfly","navigation-path");return;}
    if(target.swimDepth) {
        const auto points=navigationRouteOwned(s.mesh,player,target,s.entities);auto text=nav::sceneSwimmingInstruction(s.observation);
        if(!points.empty()) {routeNavAudio(points,target);const auto directions=routeDirectionsText(points,UnitsPerStep,setting(Setting::PartialSteps)!=0);if(!directions.empty()) text+=L". "+directions;}
        else {resetNavAudio();if(target.swimReady) text+=L". No connected underwater route is available";}
        emit(event,std::move(text),"navigation-path");if(setting(Setting::AutoWalk) && !points.empty()) intent(event,target,nav::Intent::Start);return;
    }
    if(target.pushControl && !target.zone.valid) {if(!assist) emit(event,label(target)+L": pedestal collision data unavailable","navigation-path");else intent(event,target,nav::Intent::AssistFailed);return;}
    if(target.zone.valid && !target.zone.crossing && navigationTargetReached(target,player)) {
        if(assist) return;
        if(target.butterfly) {emit(event,label(target)+(target.butterflyStart?L": within start range":L": within catching range"),"navigation-facing");return;}
        if(setting(Setting::AutoWalk)) intent(event,target,target.pushControl?nav::Intent::ArmAndFace:nav::Intent::Face);
        if(target.pushControl) {emit(event,label(target)+L": at the push position. "+instruction(target),"navigation-facing");return;}
        const auto facing=direction({target.zone.center.x-player.x,0,target.zone.center.z-player.z});
        if(facing==L"here" || facing==L"unknown") emit(event,label(target)+L": within interaction range. Facing direction is unavailable at this position","navigation-facing");
        else emit(event,label(target)+L": within interaction range. "+(target.inactive?L"Currently inactive. ":L"")+L"Face "+facing,"navigation-facing");return;
    }
    if(s.mesh.triangles.empty() && !s.mesh.openPlane) {
        resetNavAudio();emit(event,label(target)+L": collision data unavailable. Direct distance "+routeSegmentText(distanceXZ(player,target.position)/UnitsPerStep,direction({target.position.x-player.x,0,target.position.z-player.z}),setting(Setting::PartialSteps)!=0),"navigation-path");
        if(assist) intent(event,target,nav::Intent::AssistFailed);return;
    }
    const int start=findTriangle(s.mesh,player),end=target.zone.valid?start:findTriangle(s.mesh,target.position);
    if(start<0 || end<0) {resetNavAudio();emit(event,label(target)+(start<0?L": player is outside the mapped floor":L": destination is outside the mapped floor"),"navigation-path");if(assist) intent(event,target,nav::Intent::AssistFailed);return;}
    const auto points=navigationRouteOwned(s.mesh,player,target,s.entities);
    if(points.empty()) {resetNavAudio();emit(event,label(target)+(target.zone.valid?L": no reachable interaction zone on the connected floor":L": no connected collision route"),"navigation-path");if(assist) intent(event,target,nav::Intent::AssistFailed);return;}
    routeNavAudio(points,target);
    if(!assist && (target.butterfly || !setting(Setting::AutoWalk) || setting(Setting::AutoWalkPath))) {const auto directions=routeDirectionsText(points,UnitsPerStep,setting(Setting::PartialSteps)!=0);emit(event,directions.empty()?L"Inside the trigger zone":directions,"navigation-path");}
    if(setting(Setting::AutoWalk)) intent(event,target,assist?nav::Intent::AssistStart:target.pushControl?nav::Intent::ArmAndStart:nav::Intent::Start);
}
void execute(const Pending& event) {
    auto& s=owned();const auto query=event.query.query;
    // Log the exact unresolved Baaj state only for explicit list/distance/path
    // commands. Background pose/resource refreshes never produce this dump.
    // All data is already owned; no getter or native pointer is used here.
    if(s.observation.room==196 && (query==nav::Query::Scan || query==nav::Query::Next || query==nav::Query::Previous ||
        query==nav::Query::Distance || query==nav::Query::Path || query==nav::Query::Filter)) {
        const auto& observation=s.observation;
        recordLog("Baaj state query=%u epoch=%u copied-bank=%u copied-flags=%d native-ready=%u native-flags=%d activated-count=%d",
            event.query.serial,observation.epoch,observation.saved.valid,observation.saved.valid?static_cast<int>(observation.saved.bytes[0xa70]):-1,
            observation.baajFactsReady,observation.baajTreasureFlags,observation.baajActivatedCount);
        for(const auto& entity:s.entities) if(entity.worker>=0 && static_cast<unsigned>(entity.worker)<observation.count) {
            const auto& fact=observation.workers[entity.worker];if(fact.type!=6 || fact.local<30 || fact.local>35) continue;
            recordLog("Baaj seal local=%u global=%u captured-channels=%u inactive=%u zone-valid=%u x=%.3f y=%.3f z=%.3f",
                fact.local,fact.global,fact.interactions.channels,entity.inactive,entity.zone.valid,entity.position.x,entity.position.y,entity.position.z);
        }
    }
    switch(query) {
    case nav::Query::Scan: {
        s.selection={};s.selected=0;clearNavRoute();const auto rows=filtered();
        emit(event,std::to_wstring(rows.size())+L" "+categories[s.category]+(s.category?L"s":L""),"navigation-scan");
        if(!rows.empty()) {announce(event,false);lockSelected();beacon(event);}break;
    }
    case nav::Query::Next:case nav::Query::Previous: {
        const int delta=event.query.query==nav::Query::Next?1:-1;clearNavRoute();
        if(event.query.shift) {s.category=(s.category+delta+_countof(categories))%_countof(categories);s.selected=0;const auto rows=filtered();emit(event,L"category: "+std::wstring(categories[s.category])+L", "+std::to_wstring(rows.size()),"navigation-category");if(!rows.empty()) announce(event,false);}
        else {const auto rows=filtered();if(rows.empty()) break;s.selected=(s.selected+delta+rows.size())%rows.size();announce(event);}
        lockSelected();beacon(event);break;
    }
    case nav::Query::Distance:announce(event,true,false);break;
    case nav::Query::Path:pathfind(event);break;
    case nav::Query::Filter:
        adjustSetting(Setting::HideUnreachable,1);requestOwnedReachability(s.observation,s.entities);
        emit(event,L"Hide unreachable destinations: "+settingValue(Setting::HideUnreachable)+(setting(Setting::HideUnreachable)?(ownedReachabilityChecking()?L". Checking reachability":L". Reachability unavailable; destinations remain listed"):L". "+std::to_wstring(filtered().size())+L" destinations"),"navigation-filter");
        if(!filtered().empty()) {announce(event,false);lockSelected();}break;
    case nav::Query::Beacon:beacon(event);break;
    case nav::Query::AssistRoute:pathfind(event,true);break;
    case nav::Query::AssistInstruction: {
        for(const auto& e:s.entities) if(e.worker==s.observation.pedestal.worker && e.pushControl==s.observation.pedestal.control && e.id==s.observation.pedestal.id) {emit(event,instruction(e),"pedestal-push-turn",false);break;}break;
    }
    case nav::Query::GuideStep:case nav::Query::GuideNext:case nav::Query::GuidePrevious:case nav::Query::GuideExpected:case nav::Query::GuidePath:case nav::Query::GuideCompleted:case nav::Query::GuideTileCompleted: {
        if(event.query.policyRevision!=ownedPuzzleGuideRevision()) break;
        const auto action=static_cast<OwnedGuideAction>(static_cast<unsigned>(event.query.query)-static_cast<unsigned>(nav::Query::GuideStep));
        try {const auto text=puzzleGuideOwned(action,s.observation,s.mesh,s.entities,event.header,event.query.manual,event.query.intentRevision,event.query.local,event.query.chunk,event.query.resourceSerial);if(!text.empty()) {consoleReplyReady(text.c_str(),text.size());emit(event,text,"utility-reply");}}
        catch(const std::exception& error) {const auto* reason=error.what();emit(event,L"Utility stopped: "+std::wstring(reason,reason+std::strlen(reason))+L".","utility-reply");}
        break;
    }
    case nav::Query::GuideTarget: {
        if(!event.direct || event.query.policyRevision!=ownedPuzzleGuideRevision()) break;
        bool matching=false;for(unsigned part=0;part<s.observation.partCount;++part) if(s.observation.parts[part].span.chunk==event.direct->chunk && s.observation.parts[part].span.serial==event.direct->serial) {matching=true;break;}
        if(!matching) {emit(event,L"Command cancelled because the field script changed.","utility-reply");break;}
        const auto route=puzzleTargetPathOwned(s.observation,s.mesh,s.entities,event.direct->target,event.direct->finish,event.query.manual,event.query.intentRevision);
        const auto text=event.direct->prefix.empty()?route:event.direct->prefix+(route.empty()?L"":L" ")+route;
        if(!text.empty()) {consoleReplyReady(text.c_str(),text.size());emit(event,text,"utility-reply");}break;
    }
    default:break;
    }
}
void processPending() {
    auto& s=owned();if(!s.ready) return;
    if(s.observation.collisionSerial!=navCollisionSerial()) {nav::requestNavigationScene();nav::navigationWaitingForData(true);return;}
    nav::CameraScope camera(s.observation.forward,s.observation.right,s.observation.cameraValid);
    nav::ComputationScope computation(nav::navigationRevisionCounter(),s.revision);
    for(unsigned count=0;count<128 && !s.pending.empty();++count) {
        const auto event=s.pending.front();if(event.query.epoch!=generation() || event.query.revision!=nav::navigationObservationRevision()) {s.pending.pop_front();continue;}
        if(event.query.requested>s.requested) break;s.pending.pop_front();execute(event);
    }
    nav::navigationWaitingForData(!s.pending.empty());
}
void resetOwned(unsigned) {
    releaseOwnedGuideRefresh();
    if(!storage.ready()) return;auto& s=owned();s.entities.clear();s.pending.erase(std::remove_if(s.pending.begin(),s.pending.end(),[&](const Pending& event) {return event.query.epoch!=generation() || event.query.revision!=nav::navigationObservationRevision();}),s.pending.end());s.areas.clear();s.names={};s.selection={};s.epoch=0;s.revision=0;s.ready=s.haveMesh=false;s.selected=0;s.mesh={};interested=false;retireOwnedReachability();
}
void receiveQuery(const speechq::Header& header,const nav::QueryObservation& query) {
    auto& s=owned();if(s.pending.size()>=128) {if(query.query==nav::Query::GuideTarget) {QueuedPuzzleTarget retired;takePuzzleGuideTarget(query.local,query.resourceSerial,retired);}speechq::emitOwned(L"Navigation is busy. Try the command again after it recovers.",header,"navigation-pressure",false,false);return;}
    Pending event{header,query,{}};
    if(query.query==nav::Query::GuideTarget) {event.direct.emplace();if(!takePuzzleGuideTarget(query.local,query.resourceSerial,*event.direct)) {speechq::emitOwned(L"Navigation action is unavailable or retired. Request the route again.",header,"utility-reply",false,false);return;}}
    s.pending.push_back(std::move(event));processPending();
}
void receiveScene(const speechq::Header& header,const nav::SceneObservation& captured) {
    auto& s=owned();const auto& observation=captured.scene;
    if(observation.collisionSerial!=navCollisionSerial()) {nav::requestNavigationScene();return;}
    if((s.revision && s.revision!=observation.revision) || (s.epoch && s.epoch!=observation.epoch)) resetOwned(observation.epoch);
    s.observation=observation;s.epoch=observation.epoch;s.revision=observation.revision;s.requested=captured.requested;s.ready=false;
    for(unsigned part=0;part<observation.partCount;++part) {
        const auto& fact=observation.parts[part];AtelIdentity identity;
        if(fact.bindingsExpected && !fact.identityReady && readyAtelIdentity(fact.span.chunk,identity) && identity.span.serial==fact.span.serial) {nav::requestNavigationScene();nav::navigationWaitingForData(true);return;}
    }
    nav::ComputationScope computation(nav::navigationRevisionCounter(),observation.revision);
    nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);const auto controls=copiedControlContext(observation.controls);
    for(unsigned i=0;i<observation.areaNameCount;++i) {const auto& name=observation.areaNames[i];const auto text=decodeOwnedBytes(name.bytes.data(),name.length,OwnedTextMode::Terminated,controls);s.areas[name.index]=text.empty()?std::wstring{}:text.front();}
    bool missing=false,overflow=false;auto names=s.names;names.epoch=s.epoch;names.revision=s.revision;
    std::vector<Entity> found;
    const bool ready=nav::scanNavigationScene(observation,found,[&](int index) {
        if(index<0) return std::wstring{};const auto present=s.areas.find(index);if(present!=s.areas.end()) return present->second;
        missing=true;for(unsigned i=0;i<names.count;++i) if(names.indices[i]==index) return std::wstring{};
        if(names.count<names.indices.size()) names.indices[names.count++]=index;else overflow=true;return std::wstring{};
    },[&](int index) {return nav::scenePersonName(observation,index,controls);});
    if(!ready) {nav::navigationWaitingForData(true);return;}
    if(overflow) {for(const auto& event:s.pending) emit(event,L"Navigation names exceed the supported observation capacity. The command is unavailable.","navigation-data-unavailable",false);s.pending.clear();nav::navigationWaitingForData(false);return;}
    if(missing) {if(names.count!=s.names.count) {s.names=names;nav::requestNavigationAreaNames(names);}nav::navigationWaitingForData(true);return;}
    const auto& geometry=observation.geometry;
    const bool sameMesh=s.haveMesh && reach::sameGeometry(s.meshContext.geometry,geometry.geometry) && s.meshContext.swimming==geometry.swimming && s.meshContext.waterBake==geometry.waterBake && s.meshContext.waterCrossings==geometry.waterCrossings && nav::sameRouteEvents(s.meshContext.events,geometry.events) &&
        s.meshContext.collision.actorFlags==geometry.collision.actorFlags && s.meshContext.collision.actorIsControlled==geometry.collision.actorIsControlled &&
        s.meshContext.collision.remaps==geometry.collision.remaps && (!geometry.swimming || (s.meshContext.floor==geometry.floor && s.meshContext.player.y==geometry.player.y));
    if(!sameMesh) {s.mesh=nav::decodeNavigationSceneMesh(observation);s.haveMesh=true;}
    s.mesh.player=geometry.player;s.mesh.playerTriangle=geometry.floor;
    nav::stageAirshipPassages(observation,s.mesh,found);
    bool same=s.entities.size()==found.size() && distanceXZ(s.observation.player,s.meshContext.player)<.1 && std::abs(s.observation.player.y-s.meshContext.player.y)<.1;
    std::map<EntityKey,unsigned> order;if(same) {for(unsigned i=0;i<s.entities.size();++i) order[key(s.entities[i])]=i;for(const auto& e:found) if(!order.count(key(e))) {same=false;break;}}
    if(same) std::stable_sort(found.begin(),found.end(),[&](const Entity& a,const Entity& b) {return order.at(key(a))<order.at(key(b));});
    else std::stable_sort(found.begin(),found.end(),[](const Entity& a,const Entity& b) {return a.distance<b.distance;});
    s.entities=std::move(found);s.selected=0;
    s.meshContext=geometry;requestOwnedReachability(observation,s.entities);
    const auto rows=filtered();bool kept=false;for(unsigned i=0;i<rows.size();++i) if(s.selection.valid && key(*rows[i])==s.selection) {s.selected=i;kept=true;break;}
    if(s.selection.valid && !kept) {s.selection={};clearNavRoute();}
    s.ready=true;
    const std::wstring pedestal=s.selected<rows.size() && rows[s.selected]->pushControl?instruction(*rows[s.selected]):std::wstring{};
    updateOwnedNavAudio(observation,s.mesh,s.entities,header,pedestal);
    if(observation.pedestal.armed && observation.control) for(const auto& e:s.entities) if(e.worker==observation.pedestal.worker && e.id==observation.pedestal.id && e.pushControl==observation.pedestal.control) {nav::submitNavigationIntent(observation,e,nav::Intent::UpdatePedestal,autoWalkInputRevision(),nav::navigationIntentRevision());break;}
    processPending();
}
void maintenance() {
    retirePuzzleGuideTargets();
    if(!storage.ready()) return;auto& s=owned();if(s.revision && s.revision!=nav::navigationObservationRevision()) {resetOwned(generation());return;}
    if(s.ready && applyOwnedReachability(s.observation,s.entities)) {
        const auto rows=filtered();s.selected=0;bool kept=false;for(unsigned i=0;i<rows.size();++i) if(s.selection.valid && key(*rows[i])==s.selection) {s.selected=i;kept=true;break;}
        if(s.selection.valid && !kept) {s.selection={};clearNavRoute();}
    }
}
void enqueue(nav::Query query,bool shift=false,bool cancel=false) {
    if(cancel) {nav::invalidateNavigationIntents();cancelPedestal();clearNavRoute();stopAutoWalk();}
    nav::submitNavigationQuery(query,shift);
}
void pathKey(bool shift) {
    if(inBattle()) output(L"Field navigation is unavailable in battle",true,"navigation-path");
    else if(!interested.load(std::memory_order_acquire) && !nav::navigationDataPending()) output(L"No entity selected",true,"navigation-path");
    else enqueue(nav::Query::Path,shift,true);
}
void gilKey(bool) {using Getter=unsigned(__cdecl*)();const auto value=reinterpret_cast<Getter>(address(0x384f40))();TextPlan plan;prepareTextPlan(plan,"current-gil");plan.headline=planConcat(plan,planInteger(plan,value),planLiteral(plan,L" Gil"));commitTextPlan(plan);}
void capture(Event&) {
    // The same free-search flag used by M owns movement on every entry route.
    // Field loops cannot use its cursor as a walking actor. Other mod sounds
    // (including explicit F8 previews) retain their independent event gates.
    const bool now=airshipNavigationUiActive();
    setFieldAudioAllowed(!now);
    if(now) return;
    nav::captureRequestedNavigationScene(true);captureWallCues();nav::dispatchNavigationIntents();
}
bool observationNeeded() {return interested.load(std::memory_order_acquire) || nav::navigationDataPending() || navRouteKnown();}
void actorEvent(Event&) {finishMapArrival();if(observationNeeded()) nav::requestNavigationScene();}
void movementBefore(Event& e) {
    if(inBattle() || scene || airshipNavigationUiActive()) return;const auto player=playerActor();if(!player || !read<uint16_t>(player+2)) return;
    e.before[0]=player;e.values[0]=read<float>(player+0xc);e.values[1]=read<float>(player+0x10);e.values[2]=read<float>(player+0x14);e.values[3]=read<float>(player+0x4c);e.values[4]=read<float>(player+0x54);e.before[1]=(read<unsigned>(player+0x194)&0x200)!=0;
}
void movementAfter(Event& e) {
    if(inBattle() || scene || airshipNavigationUiActive() || !e.before[0] || static_cast<uintptr_t>(e.before[0])!=playerActor()) return;
    const auto player=playerPosition();const double requested=std::hypot(e.values[3],e.values[4]),accepted=distanceXZ(player,{e.values[0],e.values[1],e.values[2]});
    const bool obstructed=e.before[1] && requested>.05 && accepted<std::min(.05,requested*.02);if(obstructed && !blocked) output(L"Movement blocked",false,"navigation-blocked");blocked=obstructed;
    if(observationNeeded()) nav::requestNavigationScene();captureWallCues();
}
void wallBefore(Event& e) {if(e.args[0]>64) return;using Getter=unsigned(__cdecl*)(unsigned);e.before[0]=reinterpret_cast<Getter>(address(0x43e960))(static_cast<unsigned>(e.args[0]));e.before[1]=1;}
void collisionChanged(Event&) {
    // Invalidate copied legs before another input poll can reuse their mask.
    // Only a fresh scene can publish a route for this new collision state.
    navCollisionRevision.fetch_add(1,std::memory_order_acq_rel);pauseNavRouteAudio();refreshNavCollision();
}
void wallAfter(Event& e) {if(!e.before[1] || inBattle()) return;using Getter=unsigned(__cdecl*)(unsigned);if(static_cast<unsigned>(e.before[0])!=reinterpret_cast<Getter>(address(0x43e960))(static_cast<unsigned>(e.args[0]))) collisionChanged(e);}
void interactionBefore(Event& e) {e.before[0]=read<uint8_t>(e.args[0]+0xab);}
void interactionAfter(Event& e) {if(observationNeeded() && e.before[0]!=read<uint8_t>(e.args[0]+0xab)) nav::requestNavigationScene();}
}
void navigationReachabilityChanged() {if(interested.load()) nav::requestNavigationScene();}
void refreshNavigation() {enqueue(nav::Query::Refresh);}
void refreshSelectedBeacon() {enqueue(nav::Query::Beacon);}
bool requestNavigationAssistRoute() {return nav::submitNavigationQuery(nav::Query::AssistRoute);}
void requestNavigationAssistInstruction() {enqueue(nav::Query::AssistInstruction);}
void resetNavigation() {nav::invalidateNavigationIntents();nav::resetNavigationObservations();resetReachability();cancelPedestal();stopAutoWalk();resetNavigationScripts();resetNavAudio();interested=false;}
void prepareNavigationScriptRetirement();void prepareNavRetirement();
void installNavigation() {
    storage.prepare();startPuzzleGuideTargets();prepareNavigationScriptRetirement();prepareNavRetirement();installReachability();installAutoWalk();installPedestal();nav::startNavigationIntents();
    nav::startNavigationObservations(receiveQuery,receiveScene,resetOwned);speechq::registerDomainMaintenance(12,maintenance);onReachabilityPrepared(speechq::notifyPreparation);
    hotkey('U',[](bool shift) {enqueue(nav::Query::Filter,shift);});hotkey('G',gilKey);hotkey('M',[](bool) {announceCurrentMap();});
    hook(map_transition::EnableControl,nullptr,actorEvent);hook(0x225930,nullptr,capture);
    for(unsigned rva:{0x425010u,0x42b500u,0x426770u}) hook(rva,nullptr,actorEvent);
    hook(0x382730,nullptr,[](Event&) {resetBattle();if(interested.load()) nav::requestNavigationScene();});
    hook(0x43e9e0,wallBefore,wallAfter);hook(0x43d3b0,movementBefore,movementAfter);hook(0x45bf70,interactionBefore,interactionAfter);hook(0x45c140,interactionBefore,interactionAfter);
    hook(0x43ea00,nullptr,collisionChanged);
    hotkey(VK_OEM_3,[](bool shift) {enqueue(nav::Query::Scan,shift,true);});hotkey(VK_OEM_6,[](bool shift) {if(!inBattle()) enqueue(nav::Query::Next,shift,true);});hotkey(VK_OEM_4,[](bool shift) {if(!inBattle()) enqueue(nav::Query::Previous,shift,true);});
    hotkey('P',[](bool shift) {if(!inBattle()) enqueue(nav::Query::Distance,shift);});hotkey(VK_OEM_5,[](bool shift) {if(sphereGridOpen()) spherePathKey(shift);else pathKey(shift);});
}
#ifdef FFX_READER_TEST
bool navigationSelectedForTest(Entity& target) {if(nativeGameThread() || !owned().ready) return false;const auto rows=filtered();if(owned().selected>=rows.size()) return false;target=*rows[owned().selected];return true;}
unsigned navigationPendingForTest() {return static_cast<unsigned>(owned().pending.size());}
#endif
}
