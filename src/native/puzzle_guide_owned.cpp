#include "puzzle_guide_owned.hpp"
#include "puzzle_guide.hpp"
#include "puzzle_guide_data.hpp"
#include "puzzle_tiles.hpp"
#include "puzzle_tiles_data.hpp"
#include "zanarkand_patterns.hpp"
#include "utility_trials_data.hpp"
#include "navigation_intents.hpp"
#include "nav_audio.hpp"
#include "nav_format.hpp"
#include "butterfly_navigation.hpp"
#include "settings.hpp"
#include "process_storage.hpp"
#include "navigation_cancel.hpp"
#include "camera_direction.hpp"
#include <atomic>
#include <cmath>
#include <set>
#include <limits>
#include <stdexcept>
#include <cstring>
namespace ffx {
namespace {
struct State {unsigned epoch=0,revision=0,step=0,expected=0,part=0,begin=0,end=0;const PuzzleRoute* route=nullptr;int pattern=-1,progress=-1;std::set<unsigned> visited;};
ProcessStorage<State> storage;
std::atomic<unsigned> revision{1};std::atomic<bool> enabled{false};
std::atomic<bool> refreshPending{false};
int value(const nav::Scene& observation,const State& state,unsigned low,unsigned high=1,unsigned element=0) {
    const auto& facts=observation.parts[state.part];if(!facts.guideReady) throw std::runtime_error("The puzzle observations are unavailable");
    for(unsigned i=0;i<facts.bindings.count;++i) {const auto& row=facts.bindings.rows[i];if(row.binding==low && row.high==high && row.element==element) return row.value;}
    throw std::runtime_error("Required puzzle observation is missing");
}
bool done(const nav::Scene& observation,const State& state,const PuzzleStep& step) {if(!step.check) return false;const auto current=value(observation,state,step.done.low,step.done.high,step.done.element);return step.done.low==0x01000a70?(current&step.done.value)!=0:current==step.done.value;}
void advance(const nav::Scene& observation,State& state) {
    for(unsigned i=state.expected;i+1<state.end;++i) if(state.route->steps[i].checkpoint && done(observation,state,state.route->steps[i])) state.expected=i+1;
    while(state.expected+1<state.end && done(observation,state,state.route->steps[state.expected])) ++state.expected;
}
bool context(const nav::Scene& observation,State& state) {
    const auto current=revision.load(std::memory_order_acquire);const PuzzleRoute* route=nullptr;unsigned selected=observation.partCount;
    for(unsigned part=0;part<observation.partCount;++part) {const auto& fact=observation.parts[part];if(fact.span.chunk!=observation.primary) continue;AtelIdentity identity;
        if(!readyAtelIdentity(fact.span.chunk,identity) || identity.span.serial!=fact.span.serial) continue;
        bool verified=false;for(const auto& trial:trialUtilities) if(identity.length==trial.length && identity.checksum==trial.checksum && !std::strcmp(identity.name,trial.script)) {verified=true;break;}
        if(!verified) continue;for(const auto& candidate:puzzleRoutes) if(!std::strcmp(candidate.script,identity.name)) {route=&candidate;selected=part;break;}}
    if(!route || route->steps.empty()) return false;
    state.part=selected;unsigned begin=0,end=static_cast<unsigned>(route->steps.size());
    if(route->returnStep) {
        const auto visit=zanarkandPhase(value(observation,state,0x20000a00));
        if(visit==ZanarkandPhase::FirstVisit) end=route->returnStep;
        else if(visit==ZanarkandPhase::GateOpen) {begin=route->returnStep-1;end=route->returnStep;}
        else if(visit==ZanarkandPhase::ReturnVisit) begin=route->returnStep;
        else end=0;
    }
    if(state.epoch!=observation.epoch || state.revision!=current || state.route!=route || state.begin!=begin || state.end!=end) {state.visited.clear();state.step=state.expected=begin;state.pattern=state.progress=-1;state.epoch=observation.epoch;state.revision=current;state.route=route;state.begin=begin;state.end=end;}
    return true;
}
void synchronizeTiles(const nav::Scene& observation,State& state) {
    const int pattern=value(observation,state,0x5c000004),progress=value(observation,state,pattern==0?0x0c000022:pattern==5?0x0c000029:0x0c000024);
    if(state.pattern!=pattern || progress<state.progress) state.visited.clear();state.pattern=pattern;state.progress=progress;
}
bool tile(const nav::Scene& observation,State& state,Point& point,std::wstring& label) {
    if(std::strcmp(state.route->script,"dome0700")) return false;synchronizeTiles(observation,state);const PuzzleTile* target=nullptr;double best=std::numeric_limits<double>::infinity();
    for(const auto& candidate:puzzleTiles) {nav::checkComputation();if(static_cast<int>(candidate.stage)!=state.pattern || state.visited.count(candidate.worker)) continue;bool blocked=false;
        for(const auto& guard:candidate.blocked) if(value(observation,state,guard.low,guard.high,guard.element)==guard.value) blocked=true;if(blocked) continue;
        const auto distance=distanceXZ(observation.player,candidate.point);if(distance<best) {best=distance;target=&candidate;}}
    if(!target) {label=L"No remaining permitted tile for this pattern. Activate the next pedestal or follow the current sphere step.";return false;}
    point=target->point;label=L"Pattern "+std::to_wstring(state.pattern)+L". Next permitted floor panel: "+zanarkandGlyphLabel(target->worker)+L". Step onto it; a wrong panel resets the pattern. Press F10 for its path.";return true;
}
Entity target(const nav::Scene& observation,const State& state,const PuzzleStep& step,const std::vector<Entity>& entities) {
    Entity result;result.name=step.label;
    const bool push=step.platform || (!std::strcmp(state.route->script,"dome0700") && step.worker>=44 && step.worker<=47);
    if(push) configurePedestalTarget(result,state.route->script,step.worker);
    for(const auto& found:entities) if(found.worker>=0 && static_cast<unsigned>(found.worker)<observation.count) {
        const auto& fact=observation.workers[found.worker];if(fact.part==state.part && fact.local==step.worker && (found.pushControl!=0)==(result.pushControl!=0)) {result=found;result.name=step.label;return result;}}
    for(unsigned i=0;i<observation.count;++i) {const auto& fact=observation.workers[i];if(!fact.valid || fact.part!=state.part || fact.local!=step.worker) continue;
        bool positioned=(fact.type>=1 && fact.type<=3) || fact.type==5 || fact.type==6;
        if(fact.type==1) {uint16_t alive=0;if(fact.actor<0) positioned=false;else std::memcpy(&alive,observation.actors[fact.actor].bytes.data()+2,2);positioned=positioned && alive;}
        const auto p=fact.position;if(positioned && std::abs(p.x)<50000 && std::abs(p.z)<50000 && (p.x || p.y || p.z)) {result.position=p;result.worker=i;result.actor=fact.actor;result.id=0xfffe;if(fact.actor>=0) {uint16_t model=0;std::memcpy(&model,observation.actors[fact.actor].bytes.data(),2);result.id=model;}result.zone=nav::sceneInteractionZone(fact,fact.interactions.channels&1,observation.extraTalkRange);return result;}}
    if(!result.pushControl && (step.fallback.x || step.fallback.y || step.fallback.z)) {result.position=step.fallback;return result;}
    throw std::runtime_error("This control is not positioned yet; perform the preceding step first");
}
std::wstring instruction(const nav::Scene& observation,const Entity& target) {
    if(!target.pushControl) return {};PedestalPlan plan;if(!nav::scenePedestalPlan(observation,target,plan)) return L"The current pedestal state is unavailable.";
    auto controls=observation.controls;controls.swap=false;const auto names=copiedControlContext(controls);const bool single=!plan.moves.empty() && plan.moves.front().mode==PushMode::Single;
    const unsigned raw=single?observation.confirmMask:0x40;const unsigned masks[]={0x10,0x20,0x40,0x80,4,8,1,2,0x800,0x100};std::vector<std::wstring> choices;
    for(unsigned i=0;i<_countof(masks);++i) if(raw&masks[i]) {const auto& name=names.glyphs[0x30+i];if(!name.empty() && name!=L"unassigned control") choices.push_back(name);}
    for(unsigned i=0;i<4;++i) if(raw&(0x1000u<<i)) {const auto& name=names.glyphs[0x40+(1u<<i)];if(!name.empty() && name!=L"unassigned control") choices.push_back(name);}
    const auto binding=join(choices,L" or ");const auto consent=binding.empty()?single?std::wstring(L"Confirm"):std::wstring(L"the configured push control"):binding;
    const auto side=describePedestalApproach(plan,target.position,observation.player);return (side.empty()?L"":side+L" ")+describePedestalPlan(plan,target.position,consent);
}
std::wstring path(const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& entities,const Entity& target,bool platform,unsigned manual,unsigned intentRevision,const std::wstring& finish={}) {
    constexpr double UnitsPerStep=11.13;resetNavAudio();
    if(target.pushControl && !target.zone.valid) return L"The pedestal collision data is unavailable.";
    if(mesh.triangles.empty() && !mesh.openPlane) return L"The current collision map is unavailable. Read the puzzle step or use the pathfinder.";
    const auto points=navigationRouteOwned(mesh,observation.player,target,entities);
    if(points.empty()) return platform?L"No walking path to that platform in the current map state. Use its lift, pedestal, or rail control first.":L"No walking path in the current map state. Complete the preceding sphere or platform action first.";
    routeNavAudio(points,target);
    if(setting(Setting::AutoWalk)) {
        const bool reached=target.zone.valid && inInteractionZone(target.zone,observation.player);
        const auto kind=target.pushControl?reached?nav::Intent::ArmAndFace:nav::Intent::ArmAndStart:nav::Intent::Start;
        if(!nav::submitNavigationIntent(observation,target,kind,manual,intentRevision)) return L"Navigation action is unavailable or busy. Request the route again after it recovers.";
    }
    double total=0;std::vector<std::wstring> segments;
    for(unsigned i=0;i+1<points.size();++i) {const double length=distanceXZ(points[i],points[i+1])/UnitsPerStep;total+=length;if(length<=.005) continue;segments.push_back(routeSegmentText(length,direction({points[i+1].x-points[i].x,points[i+1].y-points[i].y,points[i+1].z-points[i].z}),setting(Setting::PartialSteps)!=0));}
    if((target.zone.valid && inInteractionZone(target.zone,observation.player)) || total<.05) return target.name+(target.pushControl?L": at the push position. "+instruction(observation,target):L": within interaction range. Follow the current step.");
    if(setting(Setting::AutoWalk) && !setting(Setting::AutoWalkPath)) return {};
    if(target.pushControl) return join(segments,L", then ")+L". "+instruction(observation,target);
    return join(segments,L", then ")+L". "+(!finish.empty()?finish:platform?L"Interact with the platform control; riding is a separate step.":L"Interact with the puzzle control.");
}
}
std::wstring puzzleTargetPathOwned(const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& entities,Entity target,const std::wstring& finish,unsigned manual,unsigned intentRevision) {
    if(nativeGameThread()) return {};nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);
    if(target.worker>=0) {const unsigned local=static_cast<unsigned>(target.worker);bool mapped=false,positioned=false;
        const auto name=target.name;const auto fallback=target.position;const bool talk=target.zone.talk;
        for(unsigned i=0;i<observation.count;++i) {const auto& row=observation.workers[i];if(!row.valid || row.part>=observation.partCount || row.local!=local || observation.parts[row.part].span.chunk!=observation.primary) continue;
            mapped=true;
            for(const auto& current:entities) if(current.worker==static_cast<int>(i) && current.zone.valid && distanceXZ(current.zone.center,fallback)<75 && current.zone.talk==talk) {target=current;target.name=name;positioned=true;break;}
            if(!positioned) {
                const auto zone=nav::sceneInteractionZone(row,talk?1u:16u,observation.extraTalkRange);
                if(zone.valid && (!talk || distanceXZ(zone.center,fallback)<75)) {
                    target.position=talk?zone.center:fallback;target.zone=zone;target.worker=i;target.actor=row.actor;target.id=0xfffe;
                    if(row.actor>=0 && static_cast<unsigned>(row.actor)<observation.actorCount) {uint16_t model=0;std::memcpy(&model,observation.actors[row.actor].bytes.data(),2);target.id=model;}
                    positioned=true;
                }
            }
            break;}
        if(!mapped) return L"This control is not positioned yet; perform the preceding step first";
        // Preserve the original authored-coordinate fallback without assigning
        // it a false actor identity or stale collision shape.
        if(!positioned) {target.worker=target.actor=-1;target.id=0;target.position=fallback;target.zone={};}
    }
    return path(observation,mesh,entities,target,false,manual,intentRevision,finish);
}
void resetOwnedPuzzleGuide() {enabled=false;refreshPending=false;revision.fetch_add(1,std::memory_order_acq_rel);}
bool ownedPuzzleGuideActive() {return enabled.load(std::memory_order_acquire);}
unsigned ownedPuzzleGuideRevision() {return revision.load(std::memory_order_acquire);}
bool claimOwnedGuideRefresh() {bool empty=false;return refreshPending.compare_exchange_strong(empty,true,std::memory_order_acq_rel);}
void releaseOwnedGuideRefresh() {refreshPending=false;}
std::wstring puzzleGuideOwned(OwnedGuideAction action,const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& entities,const speechq::Header&,unsigned manual,unsigned intentRevision,unsigned local,uintptr_t chunk,unsigned serial) {
    if(nativeGameThread()) return {};nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);
    if(!storage.ready()) storage.prepare();auto& state=storage.get();if(!context(observation,state)) {enabled=false;if(action==OwnedGuideAction::Completed) refreshPending=false;return action==OwnedGuideAction::Completed || action==OwnedGuideAction::TileCompleted?std::wstring{}:L"No supported trial is active";}
    if(state.begin==state.end) {enabled=false;refreshPending=false;return action==OwnedGuideAction::Completed || action==OwnedGuideAction::TileCompleted?std::wstring{}:L"Finish Zanarkand's native story and encounter sequence before the return-visit puzzle guide becomes available.";}
    if(action==OwnedGuideAction::Completed) {refreshPending=false;try {if(enabled.load()) advance(observation,state);}catch(...) {enabled=false;}return {};}
    if(action==OwnedGuideAction::TileCompleted) {if(!std::strcmp(state.route->script,"dome0700") && chunk==observation.parts[state.part].span.chunk && serial==observation.parts[state.part].span.serial && local>=57 && local<=83) {synchronizeTiles(observation,state);for(const auto& candidate:puzzleTiles) if(candidate.worker==local && static_cast<int>(candidate.stage)==state.pattern) state.visited.insert(local);}return {};}
    enabled=true;if(action==OwnedGuideAction::Next) {if(state.step+1<state.end) ++state.step;}else if(action==OwnedGuideAction::Previous) {if(state.step>state.begin) --state.step;}else if(action==OwnedGuideAction::Expected) advance(observation,state);
    const unsigned selected=action==OwnedGuideAction::Expected?state.expected:state.step;if(action==OwnedGuideAction::Expected && !state.route->steps[selected].check && selected+1<state.end) return L"The current trial state does not identify an exact puzzle step. Use F9 and F10 to select the step you have reached.";
    const auto& current=state.route->steps[selected];const auto prefix=(state.route->returnStep?state.begin>=state.route->returnStep?std::wstring(L"Return-visit treasure. "):std::wstring(L"First-visit trial. "):std::wstring{})+L"Step "+std::to_wstring(selected-state.begin+1)+L" of "+std::to_wstring(state.end-state.begin)+L". ";
    if(!std::strcmp(state.route->script,"dome0700") && ((selected>0 && selected<=5) || selected==11)) {
        const int stage=value(observation,state,0x5c000004);if(selected==11 || (selected==1 && stage==0) || (selected>=2 && stage==static_cast<int>(selected-1))) {Point point;std::wstring label;if(tile(observation,state,point,label)) {Entity panel;panel.name=L"Floor puzzle panel";panel.position=point;return prefix+label+(action==OwnedGuideAction::Path?L" "+path(observation,mesh,entities,panel,false,manual,intentRevision):L"");}}}
    if(!std::strcmp(state.route->script,"bsyt0100") && current.worker==7 && value(observation,state,0x5c0000cc)!=1) return prefix+L"The Besaid Sphere must be in the pedestal before the final push. Insert it into the pedestal sphere recess first."+(action==OwnedGuideAction::Path?L" "+path(observation,mesh,entities,target(observation,state,state.route->steps[8],entities),false,manual,intentRevision):L"");
    std::wstring push;if(current.platform || (!std::strcmp(state.route->script,"dome0700") && current.worker>=44 && current.worker<=47)) {try {push=instruction(observation,target(observation,state,current,entities));}catch(...) {}}
    return prefix+current.instruction+(action==OwnedGuideAction::Path?L" "+path(observation,mesh,entities,target(observation,state,current,entities),current.platform,manual,intentRevision):push.empty()?L"":L" "+push);
}
}
