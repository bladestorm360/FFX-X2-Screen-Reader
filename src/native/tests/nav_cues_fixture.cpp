#include "../nav_audio.hpp"
#include "../navigation_scene.hpp"
#include "../navigation_observations.hpp"
#include "../settings.hpp"
#include "ready_speech_adapter.hpp"
#include "test_speech_domains.hpp"
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>
namespace {thread_local bool observingLeg=false;std::atomic<unsigned> legAllocations{0},legFrees{0};}
namespace {static ffx::nav::Scene sceneFacts; static ffx::speechq::Header sceneHeader;}
void* operator new(size_t size) {if(observingLeg) ++legAllocations;auto* p=std::malloc(size?size:1);if(!p) throw std::bad_alloc();return p;}
void operator delete(void* p) noexcept {if(p && observingLeg) ++legFrees;std::free(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
void prepareNavRetirement();
unsigned epoch=1,arrivals=0,publications=0,pauses=0;
unsigned floorReads=0;
unsigned wallCaptures=0,fixtureWallMask=0;
std::atomic<int> scene{0};
bool focused=true,menu=false,battle=false,beacon=true,arrival=true,autoWalk=false;
Point player;
NavAudioState published;
std::vector<std::wstring> said;
Mesh fixtureFloor;
std::vector<Entity> fixtureButterflies;
std::vector<Entity> butterflyTargets() { return fixtureButterflies; }
void refreshButterflyTarget(Entity& target) {
    for(const auto& fresh:fixtureButterflies) if(fresh.id==target.id) { target=fresh; return; }
    target.inactive=true;
}
void refreshButterflyStarter(Entity& target) { refreshButterflyTarget(target); }
unsigned generation() { return epoch; }
bool inBattle() { return battle; }
bool gameFocused() { return focused; }
bool modMenuActive() { return menu; }
bool utilityConsoleActive() { return false; }
int setting(Setting id) { return id==Setting::Beacon?beacon:id==Setting::Arrival?arrival:id==Setting::AutoWalk?autoWalk:id==Setting::Walls?1:id==Setting::WallRange?2:0; }
bool output(const std::wstring& text,bool,const char*) { said.push_back(text); return true; }
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) { return output(text,interrupt,producer); }
bool hook(unsigned,Handler,Handler) { return true; }
bool nativeGameThread() { return false; }
Point playerPosition() { return player; }
std::wstring pedestalInstruction(const Entity&) { return L"Fixture push action"; }
Mesh readMesh() { ++floorReads; return fixtureFloor; }
void installWallCues() { fixtureWallMask=0; }
void captureWallCues() { ++wallCaptures; fixtureWallMask=nearbyWalls(fixtureFloor,player,{1,0,0},{0,0,-1},2*11.13); }
void cancelWallCues() { fixtureWallMask=0; }
unsigned copiedWallCues() { return fixtureWallMask; }
bool cameraBasis(Point& forward,Point& right) { forward={1,0,0}; right={0,0,-1}; return true; }
bool relativeBearing(Point delta,double& pan,double& front) {
    const auto length=distanceXZ({},delta); pan=-delta.z/length; front=delta.x/length; return true;
}
void publishNavAudio(const NavAudioState& state) { published=state; published.walls|=fixtureWallMask; ++publications; }
void pauseNavAudio() { ++pauses; cancelWallCues(); }
void pauseNavRouteAudio() { ++pauses; }
void stopNavPreview() {}
void navAudioArrival() { ++arrivals; }
void publishPositionalCue(float,float,bool,unsigned) {}
}
namespace ffx::speechq { void notifyPreparation() noexcept {} }
namespace ffx::nav { void requestNavigationScene() {} }
using namespace ffx;
void check(bool ok,const char* message) { if(!ok) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
namespace {
Entity fixtureTarget;
void pump() {
    sceneFacts.epoch=epoch;sceneFacts.audioContext=navAudioContextSerial();sceneFacts.collisionSerial=navCollisionSerial(); sceneFacts.player=player; sceneFacts.forward={1,0,0}; sceneFacts.right={0,0,-1};
    sceneFacts.cameraValid=true; sceneFacts.allowed=focused&&!menu&&!battle;
    sceneHeader={}; sceneHeader.generation=epoch; sceneHeader.cancellation=0;
    captureWallCues();
    const auto live=fixtureTarget.butterfly?fixtureButterflies:std::vector<Entity>{fixtureTarget};
    updateOwnedNavAudio(sceneFacts,fixtureFloor,live,sceneHeader,L"Fixture push action");
}
void fixtureRouteNavAudio(const std::vector<Point>& points,const Entity& target) {
    fixtureTarget=target; pump(); routeNavAudio(points,target);
}
void fixtureUpdateNavTarget(const Entity& target) {
    const bool same=target.actor==fixtureTarget.actor && target.worker==fixtureTarget.worker && target.id==fixtureTarget.id &&
        target.pushControl==fixtureTarget.pushControl && target.pushLocal==fixtureTarget.pushLocal;
    if(same) fixtureTarget=target;
    pump(); updateNavTarget(target);
}
void dynamicGateFixtures() {
    // Connected two-row floor: the middle lower cell is a native mutable gate.
    // Closing it leaves a real detour through the upper row, with a fixed goal.
    Mesh grid;
    for(unsigned z=0;z<2;++z) for(unsigned x=0;x<3;++x) {
        const int a=static_cast<int>(z*4+x),b=a+1,c=a+4,d=c+1;
        for(const auto& vertices: {std::array<int,3>{a,b,c},std::array<int,3>{b,d,c}}) {
            Triangle t;t.valid=true;t.collisionGroup=x==1 && z==0?48u:0u;
            for(unsigned j=0;j<3;++j) {t.vertices[j]=vertices[j];t.neighbours[j]=-1;
                t.points[j]={double(vertices[j]%4)*10,0,double(vertices[j]/4)*10};
                t.center.x+=t.points[j].x/3;t.center.z+=t.points[j].z/3;}
            grid.triangles.push_back(t);
        }
    }
    for(auto& t:grid.triangles) for(unsigned e=0;e<3;++e) for(size_t i=0;i<grid.triangles.size();++i) {
        const auto& other=grid.triangles[i];if(&t==&other) continue;
        for(unsigned j=0;j<3;++j) if(t.vertices[e]==other.vertices[(j+1)%3] && t.vertices[(e+1)%3]==other.vertices[j]) t.neighbours[e]=static_cast<int>(i);
    }
    fixtureFloor=grid;sceneFacts.geometry.collision={};player={4,0,4};Entity goal;goal.name=L"Fixed gate goal";goal.worker=3;goal.id=47;goal.position={26,0,4};
    fixtureRouteNavAudio(routePoints(fixtureFloor,player,goal.position),goal);NavLeg leg;
    check(navLeg(leg) && !leg.pendingCollision && distanceXZ(leg.goal,goal.position)<.01,"open gate uses direct leg");
    static nav::Scene oldScene;oldScene=sceneFacts;navCollisionRevision.fetch_add(1);
    check(navLeg(leg) && leg.pendingCollision,"native gate event immediately retires copied leg before worker update");
    updateOwnedNavAudio(oldScene,fixtureFloor,{goal},sceneHeader,L"");
    check(navLeg(leg) && leg.pendingCollision,"late pre-gate scene cannot republish old movement");
    for(auto& t:fixtureFloor.triangles) if(t.collisionGroup==48) t.collision=1;
    sceneFacts.geometry.collision.remaps[0]=1;pump();
    check(navLeg(leg) && !leg.pendingCollision && std::abs(leg.goal.z-player.z)>1,"unchanged target replans around newly closed gate");
    // Opening and closing back to the same state is still a new native serial.
    navCollisionRevision.fetch_add(2);check(navLeg(leg) && leg.pendingCollision,"ABA gate changes cannot revive a stale leg");pump();
    check(navLeg(leg) && !leg.pendingCollision,"fresh same-state scene recovers the detour");
    for(auto& t:fixtureFloor.triangles) t.collision=0;
    sceneFacts.geometry.collision.remaps[0]=0;navCollisionRevision.fetch_add(1);pump();
    check(navLeg(leg) && !leg.pendingCollision && distanceXZ(leg.goal,goal.position)<.01,"reopened gate restores direct route without destination movement");
    // Also reconcile copied remaps even if a future writer has no hook event.
    for(auto& t:fixtureFloor.triangles) if(t.collisionGroup==48) t.collision=1;
    sceneFacts.geometry.collision.remaps[0]=1;pump();
    check(navLeg(leg) && std::abs(leg.goal.z-player.z)>1,"changed copied remap independently invalidates retained route");
    for(auto& t:fixtureFloor.triangles) if(t.center.x>10 && t.center.x<20) t.collision=1;
    navCollisionRevision.fetch_add(1);pump();check(!navRouteKnown() && !navLeg(leg),"fully closed connection removes synthetic steering");
    clearNavRoute();sceneFacts.geometry.collision={};
}
}
int main() {
    // Match installNavigation's bootstrap ownership setup. Retired containers
    // remain held by the fixture's prepared pools; no native-side destruction.
    prepareNavRetirement();
    dynamicGateFixtures();
    Triangle a; a.valid=true; a.points[0]={0,0,0}; a.points[1]={100,0,0}; a.points[2]={0,0,100};
    a.vertices[0]=0; a.vertices[1]=1; a.vertices[2]=2; for(auto& n:a.neighbours)n=-1;
    fixtureFloor.triangles={a}; player={5,0,5}; pump();
    check(floorReads==0,"worker pump uses the copied fixture floor without Native mesh getters");
    Entity target; target.name=L"Fixture target"; target.actor=4; target.worker=3; target.id=2; target.position={45,0,5};
    fixtureRouteNavAudio({player,{20,0,5},target.position},target);
    check(published.beacon && published.front>0 && published.pan==0,"copied next route leg controls bearing");
    // Keep the target fixed while a returning edge becomes active. A stale
    // beacon/native leg must retire even though the physical floor is unchanged.
    fixtureFloor.events.kind=1;fixtureFloor.events.count=1;
    fixtureFloor.events.rows[0]={{25,0,0},{25,0,100},{5,0,5},40};
    pump();NavLeg staleCrossing;
    check(!navRouteKnown() && !navLeg(staleCrossing),"changed callback retires retained reverse ledge route with unchanged target");
    fixtureFloor.events={};fixtureRouteNavAudio({player,{20,0,5},target.position},target);
    check(navLeg(staleCrossing) && published.beacon,"disabled returning crossing permits a fresh route");
    player={11,0,5}; pump(); check(published.steps<1,"a turn waypoint is not skipped one walking step early");
    player={22,0,5}; pump(); check(published.beacon && published.steps>2,"movement passing a waypoint advances without exact pixel contact");
    player={45,0,5}; pump(); pump();
    check(arrivals==1 && said.size()==1 && !published.beacon,"arrival sounds and speech happen once");
    player={5,0,5}; fixtureRouteNavAudio({player,target.position},target);
    target.position={5,0,50}; fixtureUpdateNavTarget(target); pump();
    check(published.pan<-.9f,"moving destination recomputes connected route");
    target.position={1000,0,1000}; fixtureUpdateNavTarget(target);
    check(said.back().find(L"route lost")!=std::wstring::npos,"disconnected moving target reports route loss");
    const auto before=publications; focused=false; pump();
    check(publications==before && pauses,"foreign focus pauses cues");
    focused=true; menu=true; pump(); check(publications==before,"mod menu pauses game cues");
    menu=false; ++epoch; pump(); check(!published.beacon,"new map cannot reuse old route");
    resetNavAudio(); pump(); check(!published.beacon,"teardown clears copied route");
    const auto cachedFloorReads=floorReads,cachedWallCaptures=wallCaptures;
    clearNavRoute(); pump();
    check(published.walls && floorReads==0 && floorReads==cachedFloorReads && wallCaptures>cachedWallCaptures,"clearing selection preserves wall audio without legacy mesh reads");
    const auto beforeWallRefresh=wallCaptures;
    refreshNavCollision(); check(floorReads==0 && floorReads==cachedFloorReads && wallCaptures>beforeWallRefresh && published.walls,
        "wall-only collision refresh captures updated wall facts without reading the legacy mesh");
    target.position={40,0,5}; target.zone.valid=true; target.zone.shape=1;
    target.zone.center=target.position; target.zone.range=5; target.zone.height=2;
    player={34,0,5}; const auto priorArrival=arrivals;
    fixtureRouteNavAudio({player,{36,0,5}},target);
    check(published.beacon && arrivals==priorArrival,"one-step proximity outside actual talk zone cannot claim arrival");
    target.position={60,0,5}; target.zone.center=target.position;
    fixtureUpdateNavTarget(target); player={36,0,5}; pump();
    check(published.beacon && arrivals==priorArrival,"moving NPC updates arrival zone as well as route");
    target.zone.range=10; fixtureUpdateNavTarget(target); player={52,0,5}; pump();
    check(arrivals==priorArrival+1 && !published.beacon,"geometry-only range change refreshes the strict arrival zone");
    target.zone.range=5; player={36,0,5}; fixtureRouteNavAudio({player,{56,0,5}},target);
    player={56,0,5}; pump();
    check(arrivals==priorArrival+2 && !published.beacon,"arrival occurs inside moved native interaction zone");
    target.inactive=false; player={36,0,5}; fixtureRouteNavAudio({player,{56,0,5}},target);
    target.inactive=true; fixtureUpdateNavTarget(target); NavLeg inactiveLeg;
    check(!navLeg(inactiveLeg) && said.back().find(L"Currently inactive")!=std::wstring::npos,
        "availability-only changes invalidate the route instead of retaining stale push eligibility");
    target.inactive=false;
    target.zone={}; target.position={45,0,5}; player={5,0,5};
    const auto priorSilent=arrivals; const auto priorSpeech=said.size(); beacon=false;
    fixtureRouteNavAudio({player,target.position},target);
    player=target.position; pump();
    check(arrivals==priorSilent && said.size()==priorSpeech+1,"beacon off keeps spoken arrival but suppresses arrival sound");
    beacon=true; pump();
    check(arrivals==priorSilent,"enabling after arrival cannot replay a suppressed arrival");
    player={5,0,5}; arrival=false; fixtureRouteNavAudio({player,target.position},target);
    player=target.position; pump();
    check(arrivals==priorSilent,"arrival setting off suppresses sound with beacon enabled");
    arrival=true; player={5,0,5}; fixtureRouteNavAudio({player,target.position},target);
    player=target.position; pump(); pump();
    check(arrivals==priorSilent+1,"both toggles on produce one arrival sound for a new route");
    autoWalk=true; player={5,0,5}; fixtureRouteNavAudio({player,target.position},target);
    const auto preciseArrival=said.size(); player={35,0,5}; pump();
    check(said.size()==preciseArrival,"automatic point route cannot finish a walking step before a floor control");
    player={44.5,0,5}; pump(); NavLeg leg;
    check(said.size()==preciseArrival+1 && navLeg(leg) && leg.arrived,"auto walk snapshot retains exact arrival after audio retires its epoch");
    player={5,0,5}; target.zone.valid=true; target.zone.shape=1; target.zone.center={40,0,5};
    target.zone.range=20; target.zone.height=5; target.zone.talk=true; target.position=target.zone.center;
    InteractionShape recess; recess.valid=true; recess.shape=3; recess.center={43,0,5};
    recess.width=20; recess.depth=5; recess.range=10; recess.height=5;
    target.zone.competing={recess};
    fixtureRouteNavAudio(interactionRoute(fixtureFloor,player,target.zone),target);
    check(navLeg(leg) && leg.final.x>30,"overlapping recess prevents an early pedestal arrival");
    const auto oldEndpoint=leg.final;
    target.zone.competing[0].range=5; fixtureUpdateNavTarget(target);
    check(navLeg(leg) && inInteractionZone(target.zone,leg.final) && distanceXZ(oldEndpoint,leg.final)>1 && leg.target.zone.competing[0].range==5,
        "changing only a competing control replans the selected pedestal route");
    target.actor=-1; fixtureRouteNavAudio(interactionRoute(fixtureFloor,player,target.zone),target);
    target.zone.competing[0].range=20; fixtureUpdateNavTarget(target);
    check(navLeg(leg) && leg.target.zone.competing[0].range==20,"non-actor recess routes also refresh their competing controls");
    target.pushControl=3; target.pushLocal=1;
    fixtureRouteNavAudio(interactionRoute(fixtureFloor,player,target.zone),target);
    auto socket=target; socket.pushControl=0; socket.pushLocal=0; socket.zone.competing.clear();
    fixtureUpdateNavTarget(socket);
    check(navLeg(leg) && leg.target.pushControl==3,"same actor's sphere recess cannot replace the selected push route");
    player={5,0,5}; Entity blue;
    blue.butterfly=true; blue.id=900; blue.name=L"Blue butterfly"; blue.position={70,0,5};
    blue.zone.valid=true; blue.zone.shape=1; blue.zone.center=blue.position; blue.zone.range=3;
    blue.zone.contactMinY=-1; blue.zone.contactMaxY=1;
    fixtureButterflies={blue}; fixtureRouteNavAudio({player,blue.position},blue);
    Entity red=blue; red.id=901; red.hazardous=true; red.position=red.zone.center={35,0,5}; red.zone.range=4;
    fixtureButterflies.push_back(red); pump();
    check(navLeg(leg) && (std::abs(leg.goal.z-player.z)>.1 || leg.goal.x<31),"new red hazard replans before following a retained straight leg");
    fixtureButterflies[1].zone.valid=false; pump();
    check(!navRouteKnown() && !navLeg(leg),"unknown red body bounds invalidate the butterfly route");
    fixtureButterflies={blue}; fixtureRouteNavAudio({player,blue.position},blue);
    fixtureButterflies.clear(); pump();
    check(!navRouteKnown(),"collection or color change removes a stale blue route");
    resetNavAudio();fixtureFloor.fixedHeight=true;fixtureFloor.swimmingFloor=true;fixtureFloor.planeY=-50;
    for(auto& t:fixtureFloor.triangles) {for(auto& p:t.points)p.y=-50;t.center.y=-50;}
    player={5,-50,5};refreshNavCollision();target={};target.worker=25;target.swimApproach=true;target.name=L"Swimming event";
    target.zone.valid=true;target.zone.shape=2;target.zone.center={70,-90,0};target.zone.end={70,-90,20};target.zone.range=5;target.zone.height=5;
    target.position=target.zone.center;fixtureRouteNavAudio({player,{20,-50,5},{66,-50,5}},target);
    const auto swimArrivals=arrivals;player={22,-90,5};pump();
    check(navLeg(leg) && leg.goal.x==66 && published.beacon && published.steps>2,"native swim depth changes cannot stall advancement at a cached turn waypoint");
    target.zone.range=6;fixtureUpdateNavTarget(target);
    check(navLeg(leg) && leg.final.y==-90 && !said.empty() && said.back().find(L"route lost")==std::wstring::npos,"swim target geometry updates replan with current depth from the owned cached mesh");
    player=leg.final;player.y=-50;pump();
    check(arrivals==swimArrivals && navLeg(leg) && !leg.arrived,"swimming approach alone cannot report arrival outside native height bounds");
    player.y=-90;pump();
    check(arrivals==swimArrivals+1 && navLeg(leg) && leg.arrived,"live native trigger height controls swimming arrival despite stale waypoint height");
    resetNavAudio();fixtureFloor.fixedHeight=fixtureFloor.swimmingFloor=false;
    for(auto& t:fixtureFloor.triangles) {for(auto& p:t.points)p.y=0;t.center.y=0;}
    target={}; target.swimDepth=true; target.name=L"Depth exit"; player={5,59,5};
    fixtureRouteNavAudio({player,{5,251,5}},target);
    check(!published.beacon,"vertical target never emits misleading horizontal here beacon");
    const auto depthArrivals=arrivals; player.y=250; pump();
    check(arrivals==depthArrivals && navLeg(leg) && !leg.arrived,"depth equality and matching X/Z cannot claim arrival");
    player.y=250.01; pump();
    check(arrivals==depthArrivals+1 && navLeg(leg) && leg.arrived,"depth crossing owns arrival speech/audio");
    // Actual production navLeg copy: long UTF16 label and complete competitors.
    Entity copiedTarget;copiedTarget.name=std::wstring(600,L'x');copiedTarget.actor=3;copiedTarget.worker=7;copiedTarget.id=0x5678;
    copiedTarget.position={45,0,5};copiedTarget.zone.valid=false;copiedTarget.zone.competing.resize(500);
    for(unsigned i=0;i<500;++i) {copiedTarget.zone.competing[i].worker=static_cast<int>(i);copiedTarget.zone.competing[i].range=i+.25;}
    fixtureRouteNavAudio({{5,0,5},{45,0,5}},copiedTarget);
    NavLeg copiedLeg;const auto copiedStarted=std::chrono::steady_clock::now();observingLeg=true;
    for(unsigned i=0;i<1000;++i) check(navLeg(copiedLeg),"actual complete POD native leg");
    observingLeg=false;const auto copiedCost=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-copiedStarted).count();
    check(!legAllocations && !legFrees && std::wcslen(copiedLeg.target.name.data())==600 && copiedLeg.target.zone.competing.count==500 &&
        copiedLeg.target.zone.competing[499].worker==499 && copiedLeg.target.zone.competing[499].range==499.25,"long name/all500 competitor facts copied with Native heap0/0");
    std::printf("METRIC actual POD NavLeg1000 warm copies=%0.3fus, Native heap0/0; component fixture only.\n",copiedCost);
    puts("PASS: event-driven route legs, strict endpoints, moving targets, butterfly hazard deltas, depth transitions, route loss, focus and teardown.");
}
