#include "nav_audio.hpp"
#include "navigation_scene.hpp"
#include "navigation_observations.hpp"
#include "owned_nav_target.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include "camera_direction.hpp"
#include "navigation_events.hpp"
#include "settings.hpp"
#include "butterfly_paths.hpp"
#include "butterfly_navigation.hpp"
#include "wall_cues.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
namespace ffx {
namespace {
constexpr unsigned Domain=11,Slots=4,PointLimit=65536,Writing=0x80000000u;
struct RouteCapture {OwnedNavTarget target;std::array<Point,PointLimit> points;unsigned count=0,epoch=0,revision=0,collisionSerial=0;};
struct Packet {unsigned slot=0,ticket=0,kind=0;};
ProcessStorage<std::array<RouteCapture,Slots>> captures;
std::array<std::atomic<unsigned>,Slots> tickets{};
std::atomic<unsigned> nextTicket{0},clearRevision{1};
std::atomic<bool> known{false};
std::atomic<DWORD> preparationOwner{0};
bool onOwner() {return !nativeGameThread() && preparationOwner.load(std::memory_order_acquire)==GetCurrentThreadId();}
struct LegSnapshot {NavLeg leg;unsigned revision=0,valid=0;};
async::AtomicSnapshot<LegSnapshot> publishedLeg;
struct State {
    Mesh collision;
    std::vector<Entity> live;
    std::vector<Point> legs;
    size_t nextLeg=1;
    std::wstring destination,pedestal;
    Entity target;
    Point player;
    unsigned epoch=0,revision=0,routeEpoch=0,audioContext=0,collisionSerial=0,routeCollisionSerial=0;
    nav::CollisionSnapshot copiedCollision,routeCollision;
    bool arrived=false,allowed=false,ready=false;
    speechq::Header header{};
    nav::CopiedCamera camera;
} ;
ProcessStorage<State> state;
State& owned() {return state.get();}
void release(unsigned slot,unsigned ticket) {if(slot<Slots) tickets[slot].compare_exchange_strong(ticket,0,std::memory_order_release,std::memory_order_relaxed);}
void clearOwned() {
    auto& s=owned();s.legs.clear();s.destination.clear();s.target={};s.routeEpoch=0;s.nextLeg=1;s.arrived=false;
    s.revision=clearRevision.load(std::memory_order_acquire);known=false;publishedLeg.publish({});
}
bool current() {return state.ready() && owned().epoch==generation() && owned().revision==clearRevision.load(std::memory_order_acquire);}
void reconcile() {if(state.ready() && !current()) {clearOwned();owned().ready=false;}}
void publishLeg() {
    auto& s=owned();LegSnapshot p;p.revision=s.revision;
    if(s.legs.size()>1 && s.nextLeg<s.legs.size() && !s.destination.empty() && (s.routeEpoch==s.epoch || s.arrived)) {
        p.valid=1;p.leg.start=s.legs[s.nextLeg-1];p.leg.goal=s.legs[s.nextLeg];p.leg.final=s.legs.back();p.leg.epoch=s.epoch;p.leg.arrived=s.arrived;p.leg.collisionSerial=s.routeCollisionSerial;
        OwnedNavTarget copied;if(!captureOwnedNavTarget(s.target,copied)) {publishedLeg.publish({});return;}p.leg.target=copied.native;
    }
    publishedLeg.publish(p);
}
void emit(std::wstring text,const char* producer) {speechq::emitOwned(std::move(text),owned().header,producer,false,false);}
bool sameShape(const InteractionShape& a,const InteractionShape& b) {
    return a.valid==b.valid && a.shape==b.shape && a.crossing==b.crossing && a.talk==b.talk && a.worker==b.worker &&
        distanceXZ(a.center,b.center)<.1 && std::abs(a.center.y-b.center.y)<.1 && distanceXZ(a.end,b.end)<.1 &&
        std::abs(a.end.y-b.end.y)<.1 && std::abs(a.range-b.range)<.1 && std::abs(a.height-b.height)<.1 &&
        std::abs(a.width-b.width)<.1 && std::abs(a.depth-b.depth)<.1 && std::abs(a.angle-b.angle)<.0001 && std::abs(a.viewAngle-b.viewAngle)<.0001;
}
bool sameTarget(const Entity& a,const Entity& b) {
    if(a.actor!=b.actor || a.worker!=b.worker || a.id!=b.id || a.pushControl!=b.pushControl || a.pushLocal!=b.pushLocal) return false;
    const auto& x=a.zone;const auto& y=b.zone;
    bool equal=sameShape(x,y) && x.competing.size()==y.competing.size() && std::abs(x.contactRadius-y.contactRadius)<.1 &&
        distanceXZ(x.approach,y.approach)<.1 && distanceXZ(x.obstacle,y.obstacle)<.1 && std::abs(x.obstacleRadius-y.obstacleRadius)<.1 &&
        std::abs(x.contactMinY-y.contactMinY)<.1 && std::abs(x.contactMaxY-y.contactMaxY)<.1;
    for(size_t i=0;equal && i<x.competing.size();++i) equal=sameShape(x.competing[i],y.competing[i]);
    return equal && a.swimReady==b.swimReady && std::abs(a.swimClearance-b.swimClearance)<.1 && a.swimApproach==b.swimApproach &&
        a.inactive==b.inactive && distanceXZ(a.position,b.position)<.1 && std::abs(a.position.y-b.position.y)<.1;
}
void setRoute(const std::vector<Point>& points,const Entity& target) {
    reconcile();auto& s=owned();s.epoch=generation();s.legs=points;s.nextLeg=1;s.routeEpoch=s.epoch;s.arrived=false;
    s.routeCollisionSerial=s.collisionSerial;s.routeCollision=s.copiedCollision;
    s.destination=target.name.empty()?target.category:target.name;s.target=target;known=!s.destination.empty();publishLeg();
}
void advance() {
    reconcile();auto& s=owned();if(!s.ready || !s.allowed || !current() || s.collisionSerial!=navCollisionSerial()) {pauseNavRouteAudio();return;}
    nav::CameraScope camera(s.camera.forward,s.camera.right,s.camera.valid);
    NavAudioState audio;audio.contextSerial=s.audioContext;audio.active=true;
    if(s.routeEpoch==s.epoch && s.target.butterfly && !s.arrived) {
        if(s.legs.size()<2) {clearOwned();return;}
        auto fresh=std::find_if(s.live.begin(),s.live.end(),[&](const Entity& e) {return e.id==s.target.id && e.actor==s.target.actor && e.worker==s.target.worker;});
        if(fresh==s.live.end() || fresh->inactive || fresh->hazardous) {clearOwned();pauseNavRouteAudio();return;}
        std::vector<Entity> red;for(const auto& e:s.live) if(e.hazardous) red.push_back(e);
        std::vector<Point> remaining{s.player};remaining.insert(remaining.end(),s.legs.begin()+std::min(s.nextLeg,s.legs.size()),s.legs.end());
        if(!butterflyRouteClear(remaining,red) || !butterflyInTarget(*fresh,s.legs.back())) {
            const auto route=navigationRouteOwned(s.collision,s.player,*fresh,s.live);
            if(route.empty()) {clearOwned();pauseNavRouteAudio();emit(L"Butterfly route unavailable","navigation-route-lost");return;}
            s.legs=route;s.nextLeg=1;
        }
        s.target=*fresh;
    }
    if(s.routeEpoch==s.epoch && s.legs.size()>1 && !s.arrived) {
        const bool horizontal=s.collision.fixedHeight && !s.target.swimDepth;
        while(s.nextLeg+1<s.legs.size() && (horizontal || std::abs(s.player.y-s.legs[s.nextLeg].y)<=8)) {
            const auto goal=s.legs[s.nextLeg],prior=s.legs[s.nextLeg-1];const auto x=goal.x-prior.x,z=goal.z-prior.z,length=std::hypot(x,z);
            const auto dx=s.player.x-goal.x,dz=s.player.z-goal.z;const bool passed=length>.001 && dx*x+dz*z>=0 && std::abs(dx*z-dz*x)/length<=1;
            if(distanceXZ(s.player,goal)>1 && !passed) break;++s.nextLeg;
        }
        const auto goal=s.legs[s.nextLeg];const double steps=distanceXZ(s.player,goal)/11.13;
        const auto pointRange=setting(Setting::AutoWalk)?1.0:11.13;
        if(s.nextLeg+1==s.legs.size() && (s.target.swimDepth?navigationTargetReached(s.target,s.player):
            (s.target.zone.valid?navigationTargetReached(s.target,s.player):distanceXZ(s.player,goal)<=pointRange) && (horizontal || std::abs(s.player.y-goal.y)<=8))) {
            s.arrived=true;emit(s.destination+L": route destination reached"+(s.target.pushControl?L". "+s.pedestal:L""),"navigation-arrival");
        } else if(!s.target.swimDepth || distanceXZ(s.player,goal)>.1) {
            double pan=0,front=1;
            if(relativeBearing({goal.x-s.player.x,0,goal.z-s.player.z},pan,front)) {audio.beacon=true;audio.pan=static_cast<float>(pan);audio.front=static_cast<float>(front);audio.steps=static_cast<float>(steps);}
        }
    }
    if(!current()) return;
    publishLeg();publishNavAudio(audio);
    if(s.arrived && s.routeEpoch) {if(setting(Setting::Beacon) && setting(Setting::Arrival)) navAudioArrival();s.routeEpoch=0;}
}
void process(const speechq::Header& header,const std::wstring& bytes) {
    preparationOwner.store(GetCurrentThreadId(),std::memory_order_release);
    if(bytes.size()*sizeof(wchar_t)!=sizeof(Packet)) return;Packet packet;std::memcpy(&packet,bytes.data(),sizeof(packet));
    if(packet.slot>=Slots || tickets[packet.slot].load(std::memory_order_acquire)!=packet.ticket) return;
    struct Return {unsigned slot,ticket;~Return(){release(slot,ticket);}} returned{packet.slot,packet.ticket};
    const auto& c=captures.get()[packet.slot];if(c.epoch!=generation() || c.revision!=clearRevision.load(std::memory_order_acquire) || c.collisionSerial!=navCollisionSerial()) return;
    reconcile();auto& s=owned();s.header=header;Entity target;if(!restoreOwnedNavTarget(c.target,target)) return;
    if(packet.kind==0) {std::vector<Point> points(c.points.begin(),c.points.begin()+c.count);setRoute(points,target);}
    else if(packet.kind==1) updateNavTarget(target);
    if(s.ready) advance();else nav::requestNavigationScene();
}
void maintenance() {
    preparationOwner.store(GetCurrentThreadId(),std::memory_order_release);reconcile();if(!captures.ready()) return;
    for(unsigned slot=0;slot<Slots;++slot) {const auto token=tickets[slot].load(std::memory_order_acquire);if(!token || (token&Writing)) continue;
        const auto& c=captures.get()[slot];if(c.epoch!=generation() || c.revision!=clearRevision.load(std::memory_order_acquire)) release(slot,token);}
}
void reset(unsigned) {maintenance();if(state.ready()) {clearOwned();owned().collision={};owned().live.clear();owned().ready=false;}}
bool captureRoute(unsigned kind,const std::vector<Point>& points,const Entity& target) {
    if(!captures.ready() || points.size()>PointLimit) return false;
    unsigned slot=Slots,token=(nextTicket.fetch_add(1)+1)&~Writing;if(!token) token=(nextTicket.fetch_add(1)+1)&~Writing;
    for(unsigned i=0;i<Slots;++i) {unsigned empty=0;if(tickets[i].compare_exchange_strong(empty,token|Writing,std::memory_order_acquire)) {slot=i;break;}}
    if(slot==Slots) return false;auto& c=captures.get()[slot];c.epoch=generation();c.revision=clearRevision.load(std::memory_order_acquire);c.count=static_cast<unsigned>(points.size());c.collisionSerial=navCollisionSerial();
    if(!captureOwnedNavTarget(target,c.target)) {release(slot,token|Writing);return false;}
    std::copy(points.begin(),points.end(),c.points.begin());tickets[slot].store(token,std::memory_order_release);
    if(!speechq::captureDomain(Domain,Packet{slot,token,kind},c.epoch)) {release(slot,token);return false;}return true;
}
}
bool navRouteKnown() {return known.load(std::memory_order_acquire);}
bool navLeg(NavLeg& leg) {
    LegSnapshot p;if(!publishedLeg.tryRead(p) || p.revision!=clearRevision.load(std::memory_order_acquire) || p.leg.epoch!=generation() || !p.valid) return false;
    leg=p.leg;leg.pendingCollision=leg.collisionSerial!=navCollisionSerial();return true;
}
void prepareNavRetirement() {
    captures.prepare();state.prepare();owned().revision=clearRevision.load(std::memory_order_acquire);
    speechq::registerDomain(Domain,process,reset);speechq::registerDomainMaintenance(Domain,maintenance);installWallCues();
}
void clearNavRoute() {clearRevision.fetch_add(1,std::memory_order_acq_rel);known=false;pauseNavRouteAudio();stopNavPreview();if(onOwner() && state.ready()) clearOwned();}
void resetNavAudio() {pauseNavAudio();clearNavRoute();publishPositionalCue(0,0,false,generation());}
void refreshNavCollision() {nav::requestNavigationScene();captureWallCues();}
void routeNavAudio(const std::vector<Point>& points,const Entity& target) {
    if(!onOwner()) {if(!captureRoute(0,points,target)) output(L"Navigation route is busy. Try again after it recovers.",false,"navigation-pressure");nav::requestNavigationScene();return;}
    setRoute(points,target);if(owned().ready) advance();
}
void updateNavTarget(const Entity& target) {
    if(!onOwner()) {if(!captureRoute(1,{},target)) output(L"Navigation updates are busy. Updates resume when it recovers.",false,"navigation-pressure");return;}
    reconcile();auto& s=owned();if(s.routeEpoch!=s.epoch || target.actor!=s.target.actor || target.worker!=s.target.worker || target.id!=s.target.id || target.pushControl!=s.target.pushControl || target.pushLocal!=s.target.pushLocal || sameTarget(s.target,target)) return;
    s.target=target;if(target.inactive) {pauseNavRouteAudio();s.routeEpoch=0;publishedLeg.publish({});emit(s.destination+L": route unavailable. Currently inactive","navigation-route-lost");return;}
    const auto route=navigationRouteOwned(s.collision,s.player,target,s.live);
    if(route.empty()) {pauseNavRouteAudio();s.routeEpoch=0;publishedLeg.publish({});emit(s.destination+L": route lost. The destination moved outside the connected floor","navigation-route-lost");}
    else {s.legs=route;s.nextLeg=1;s.arrived=false;s.routeCollisionSerial=s.collisionSerial;s.routeCollision=s.copiedCollision;publishLeg();}
}
void updateNavAudio(Point,bool) {nav::requestNavigationScene();captureWallCues();}
void updateOwnedNavAudio(const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& targets,const speechq::Header& header,const std::wstring& pedestal) {
    if(observation.collisionSerial!=navCollisionSerial()) {nav::requestNavigationScene();return;}
    if(nativeGameThread()) return;preparationOwner.store(GetCurrentThreadId(),std::memory_order_release);reconcile();auto& s=owned();s.epoch=observation.epoch;s.revision=clearRevision.load(std::memory_order_acquire);s.player=observation.player;s.audioContext=observation.audioContext;s.allowed=observation.allowed;s.ready=true;s.header=header;s.pedestal=pedestal;
    s.collisionSerial=observation.collisionSerial;s.copiedCollision=observation.geometry.collision;
    s.collision=mesh;s.live=targets;s.camera={observation.forward,observation.right,observation.cameraValid};nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);
    if(s.routeEpoch==s.epoch) {
        const auto target=std::find_if(targets.begin(),targets.end(),[&](const Entity& e) {return e.actor==s.target.actor && e.worker==s.target.worker && e.id==s.target.id && e.pushControl==s.target.pushControl && e.pushLocal==s.target.pushLocal;});
        // Guide point routes have no entity owner; they retain their copied goal.
        if(s.target.worker>=0) {if(target==targets.end()) {clearOwned();pauseNavRouteAudio();return;}updateNavTarget(*target);}
        // A native ledge sequence may relocate the player or change crossing
        // ownership without moving the destination. Retire/replan the actual
        // remaining legs; never keep steering back across a returning ledge.
        // Collision gates may change without any destination movement. Rebuild
        // the complete route once for a new native serial/remap snapshot. Do
        // not scan every retained leg against every floor triangle on movement.
        bool valid=s.collision.events.ready && s.routeCollisionSerial==s.collisionSerial &&
            s.routeCollision.actorFlags==s.copiedCollision.actorFlags && s.routeCollision.actorIsControlled==s.copiedCollision.actorIsControlled &&
            s.routeCollision.remaps==s.copiedCollision.remaps;
        Point previous=s.player;
        if(s.collision.events.count) for(size_t i=s.nextLeg;valid && i<s.legs.size();++i) {nav::checkComputation();valid=routeSegmentAllowed(s.collision,previous,s.legs[i]);previous=s.legs[i];}
        if(!valid) {
            const auto route=navigationRouteOwned(s.collision,s.player,s.target,s.live);
            if(route.empty()) {clearOwned();pauseNavRouteAudio();return;}
            setRoute(route,s.target);
        }
    }
    advance();
}
}
