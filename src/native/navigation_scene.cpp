#include "navigation_scene.hpp"
#include "airship_map.hpp"
#include "navigation_targets.hpp"
#include "interaction_region.hpp"
#include "exit_labels.hpp"
#include "room_areas.hpp"
#include "utility_memory.hpp"
#include "swimming_navigation.hpp"
#include "butterfly_processing.hpp"
#include "mod_menu.hpp"
#include "settings.hpp"
#include "nav_audio.hpp"
#include "utilities.hpp"
#include "owned_mesh_decoder.hpp"
#include "process_storage.hpp"
#include "puzzle_guide_bindings.hpp"
#include <algorithm>
#include <cstring>
#include <cmath>
namespace ffx::nav {
namespace {
struct CachedAreaNames {AreaNameDemand demand;uintptr_t table=0;unsigned available=0;bool ready=false;std::array<SceneAreaName,AreaNameLimit> rows;};
ProcessStorage<CachedAreaNames> cachedAreaNames;
template<class T,class Body> T field(const Body& body,unsigned offset) {
    T value{};if(offset<=body.size() && sizeof(value)<=body.size()-offset) std::memcpy(&value,body.data()+offset,sizeof(value));return value;
}
Point bodyPoint(const ActorBody& body) {return {field<float>(body.bytes,0xc),field<float>(body.bytes,0x10),field<float>(body.bytes,0x14)};}
void captureBaajFacts(Scene& s,const WorkerFact& fact,uintptr_t worker,const AtelSpan& span) {
    if(s.room!=196 || fact.local!=30 || fact.type!=6 || s.baajFactsReady) return;
    AtelIdentity identity;
    if(!readyAtelIdentity(span.chunk,identity) || identity.span.serial!=span.serial ||
        std::strcmp(identity.name,"bjyt1000") || identity.length!=7530 || identity.checksum!=0xd13cfb8c || identity.workers!=37) return;
    UtilityScript script;script.controller=s.controller;script.chunk=span.chunk;script.worker=worker;
    script.header=span.chunk+fact.header;script.allocationSerial=span.serial;script.workers=identity.workers;
    script.length=identity.length;script.codeOffset=identity.codeOffset;script.name=identity.name;
    // W30 declarations independently expose the saved treasure byte and the
    // map-local successful-activation counter. Read only two qualified scalars.
    s.baajFactsReady=captureUtilityValue(script,0x01000a70,1,0,s.baajTreasureFlags) &&
        captureUtilityValue(script,0x0c000000,1,0,s.baajActivatedCount);
}
double radius(const ActorBody& body) {return (field<float>(body.bytes,0x5c)+field<float>(body.bytes,0x64))*.5*field<float>(body.bytes,0x4e4);}
double bottom(const ActorBody& body) {const auto extent=field<float>(body.bytes,0x51c);return extent==0?field<float>(body.bytes,0x6a8):bodyPoint(body).y-extent;}
void captureBindings(PartFact& part,uintptr_t controller,uintptr_t workerZero) {
    part.bindings={};part.bindingsReady=part.guideReady=part.bindingsExpected=part.identityReady=false;
    const auto& span=part.span;if(span.size<0x38) return;
    const auto nameAt=read<unsigned>(span.chunk+0xc);if(nameAt>span.size-8) return;
    char name[9]{};if(!copy(span.chunk+nameAt,name,8)) return;
    const unsigned* rows=nullptr;unsigned count=0;
    static constexpr unsigned besaid[]={0x5c0000cc,0x5c000100},kilika[]={0x0c00005a,0x0c000064};
    static constexpr unsigned djose[]={0x0c000099,0x0c00009a,0x0c00009b,0x0c00009d,0x0c00009e,0x0c0000d9,0x0c0000db,0x0c0000de,0x0c0000df,0x0c0000e0,0x0c0000e1,0x0c0000e2};
    static constexpr unsigned macalania[]={0x0c000096,0x0c000097,0x0c0000a3,0x0c0000c6},bevelle[]={0x0c000007,0x0c000008,0x0c00001f,0x0c000020,0x0c000024,0x0c000029};
    if(!std::strcmp(name,"bsyt0100")) {rows=besaid;count=_countof(besaid);}
    else if(!std::strcmp(name,"klyt1000")) {rows=kilika;count=_countof(kilika);}
    else if(!std::strcmp(name,"djyt0600")) {rows=djose;count=_countof(djose);}
    else if(!std::strcmp(name,"mcyt0500")) {rows=macalania;count=_countof(macalania);}
    else if(!std::strcmp(name,"bvyt1100")) {rows=bevelle;count=_countof(bevelle);}
    static constexpr unsigned zanarkand[]={0x0c000021},butterfly[]={0x0c000041};
    if(!std::strcmp(name,"dome0700")) {rows=zanarkand;count=_countof(zanarkand);}
    else if(!std::strcmp(name,"mcfr0100") || !std::strcmp(name,"mcfr0200")) {rows=butterfly;count=_countof(butterfly);}
    if(!rows) return;
    part.bindingsExpected=true;AtelIdentity identity;part.identityReady=readyAtelIdentity(span.chunk,identity) && identity.span.serial==span.serial;
    if(!part.identityReady) return;
    if(!workerZero || read<uintptr_t>(workerZero+4)!=span.chunk || read<uint16_t>(workerZero+0x2e,0xffff)!=0) return;
    UtilityScript script;script.controller=controller;script.chunk=span.chunk;script.worker=workerZero;script.header=read<uintptr_t>(workerZero);
    script.allocationSerial=span.serial;script.workers=read<uint16_t>(span.chunk+0x34);script.length=read<unsigned>(span.chunk);script.codeOffset=read<unsigned>(span.chunk+0x30);script.name=name;
    if(script.header<span.chunk || script.header-span.chunk>span.size || span.size-(script.header-span.chunk)<0x28) return;
    for(unsigned i=0;i<count;++i) {int value=0;if(!captureUtilityValue(script,rows[i],1,0,value)) return;part.bindings.rows[part.bindings.count++]={rows[i],value};}
    part.bindingsReady=true;
    const GuideBinding* guide=nullptr;unsigned guideCount=0;
    if(!std::strcmp(name,"bsyt0100")) {guide=guide_bsyt0100;guideCount=_countof(guide_bsyt0100);}
    else if(!std::strcmp(name,"klyt1000")) {guide=guide_klyt1000;guideCount=_countof(guide_klyt1000);}
    else if(!std::strcmp(name,"djyt0600")) {guide=guide_djyt0600;guideCount=_countof(guide_djyt0600);}
    else if(!std::strcmp(name,"mcyt0500")) {guide=guide_mcyt0500;guideCount=_countof(guide_mcyt0500);}
    else if(!std::strcmp(name,"bvyt1100")) {guide=guide_bvyt1100;guideCount=_countof(guide_bvyt1100);}
    else if(!std::strcmp(name,"dome0700")) {guide=guide_dome0700;guideCount=_countof(guide_dome0700);}
    if(!guide) return;
    for(unsigned i=0;i<guideCount;++i) {
        const auto& wanted=guide[i];bool have=false;for(unsigned row=0;row<part.bindings.count;++row) if(part.bindings.rows[row].binding==wanted.low && part.bindings.rows[row].high==wanted.high && part.bindings.rows[row].element==wanted.element) {have=true;break;}
        if(have) continue;int value=0;if(part.bindings.count==part.bindings.rows.size() || !captureUtilityValue(script,wanted.low,wanted.high,wanted.element,value)) return;
        part.bindings.rows[part.bindings.count++]={wanted.low,value,wanted.high,wanted.element};
    }
    part.guideReady=true;
}
}
void prepareNavigationSceneStorage() {cachedAreaNames.prepare();}
bool captureNavigationScene(Scene& s,unsigned revision,bool needGeometry) {
    if(!nativeGameThread()) return false;
    s.capturedAt=GetTickCount64();s.audioContext=navAudioContextSerial();s.collisionSerial=navCollisionSerial();
    s.epoch=generation();s.revision=revision;s.count=s.actorCount=s.partCount=s.areaNameCount=0;s.playerIndex=-1;s.geometryValid=s.surfaceReady=false;
    s.baajFactsReady=false;s.baajTreasureFlags=s.baajActivatedCount=-1;
    s.controller=read<uintptr_t>(address(0xf26ae8));const auto count=read<uint16_t>(s.controller+0xc);
    const auto actors=read<uintptr_t>(address(0x1fc44e4));const auto actorCount=read<unsigned>(address(0x1fc44e0));const auto player=playerActor();
    if(!s.controller || read<uint16_t>(s.controller+0x1a,0xffff)!=0 || !count || count>SceneWorkers || !actors || !actorCount || actorCount>SceneActors ||
       player<actors || (player-actors)%0x880 || (player-actors)/0x880>=actorCount || !nativeRegion(actors,actorCount*0x880)) return false;
    s.primary=read<uintptr_t>(s.controller+0x48);s.room=read<uint16_t>(address(0xd2ca90));s.area=read<uint16_t>(address(0xd2ca90)+0xe);
    s.player=playerPosition();if(!validNativePoint(s.player)) return false;
    if(s.room==71 && s.area==20) {
        AtelSpan physical;if(nativeAtelSpan(s.primary,physical) && physical.size>=12) {
            const auto begin=read<unsigned>(s.primary+4),end=read<unsigned>(s.primary+8);
            if(begin && begin<=physical.size && end<=physical.size && end>=begin && end-begin>=64) {
                s.surface=read<float>(s.primary+begin+48,std::numeric_limits<float>::quiet_NaN());s.surfaceReady=std::isfinite(s.surface);
            }
        }
    }
    s.controlled=player==read<uintptr_t>(address(0xf00740));
    s.allowed=!scene && !inBattle() && !airshipNavigationUiActive() && gameFocused() && !modMenuActive() && !utilityConsoleActive();s.control=(read<uint8_t>(s.controller)&2)!=0;
    s.playerIndex=static_cast<int>((player-actors)/0x880);s.actorCount=actorCount;s.count=count;
    s.swimming=read<uint8_t>(player+0x186)==2;s.dive=read<uint8_t>(address(swimming_rva::DiveEnabled))!=0;
    s.cameraValid=cameraBasis(s.forward,s.right);s.extraTalkRange=read<float>(s.controller+0x1f0);prepareCopiedControlSnapshot(s.controls);
    s.confirmMask=pedestalConfirmMask();
    using Person=uintptr_t(__cdecl*)(unsigned);const auto person=reinterpret_cast<Person>(address(0x385070));
    for(unsigned i=0;i<s.people.size();++i) s.people[i].length=copyReadyGameText(person(i),s.people[i].bytes.data(),s.people[i].bytes.size());
    s.pedestal=capturePedestalNavigationState();
    if(!copyReadyRegion(actors,s.actors.data(),actorCount*0x880,actorCount*0x880)) return false;
    const auto& playerBody=s.actors[s.playerIndex].bytes;const auto enabled=field<uint16_t>(playerBody,2);
    if(!enabled || enabled>255 || field<unsigned>(playerBody,0x824)==0xcdcdcdcd || s.player.x==-431602080) return false;
    const auto saved=address(0xd2ca90)+0x1ec;s.saved.valid=read<uintptr_t>(s.controller+0x2c)==saved && copy(saved,s.saved.bytes.data(),s.saved.bytes.size());
    std::array<uintptr_t,SceneParts> workerZeros{};
    using Getter=uintptr_t(__cdecl*)(int);
    for(unsigned i=0;i<count;++i) {
        auto& fact=s.workers[i];fact.valid=false;fact.global=i;fact.part=SceneParts;fact.actor=-1;fact.interactions={};
        const auto worker=reinterpret_cast<Getter>(address(0x46a7c0))(i);if(!worker) continue;
        const auto chunk=read<uintptr_t>(worker+4),header=read<uintptr_t>(worker);if(!chunk || header<chunk) continue;
        fact.type=read<uint8_t>(header);if(fact.type<1 || fact.type>6) continue;
        unsigned part=0;for(;part<s.partCount && s.parts[part].span.chunk!=chunk;++part) {}
        if(part==s.partCount) {if(part==SceneParts || !nativeAtelSpan(chunk,s.parts[part].span)) return false;s.parts[part].bindings={};s.parts[part].bindingsReady=false;++s.partCount;}
        const auto& span=s.parts[part].span;
        if(header-chunk>span.size || span.size-(header-chunk)<0x28) continue;
        fact.local=read<uint16_t>(worker+0x2e,0xffff);fact.header=static_cast<unsigned>(header-chunk);fact.type=read<uint8_t>(header);
        if(fact.type<1 || fact.type>6 || 0x38ull+(fact.local+1ull)*4>span.size || read<unsigned>(chunk+0x38+fact.local*4)!=fact.header) continue;
        if(!fact.local) workerZeros[part]=worker;
        fact.part=part;const auto actor=fact.type==1?read<uintptr_t>(worker+0x9c):0;
        if(actor>=actors && (actor-actors)%0x880==0 && (actor-actors)/0x880<actorCount && actor!=player) fact.actor=static_cast<int>((actor-actors)/0x880);
        if(!copy(worker+(fact.type==5 || fact.type==6?0x284:0x558),fact.geometry.data(),fact.geometry.size())) continue;
        fact.position=fact.actor>=0?bodyPoint(s.actors[fact.actor]):Point{field<float>(fact.geometry,0),field<float>(fact.geometry,4),field<float>(fact.geometry,8)};
        fact.interactions=captureNativeInteractionFacts(worker,EntryMask{},span);fact.valid=validNativePoint(fact.position);
        if(fact.valid) captureBaajFacts(s,fact,worker,span);
    }
    for(unsigned part=0;part<s.partCount;++part) {captureBindings(s.parts[part],s.controller,workerZeros[part]);s.parts[part].butterflyExiting=butterflyNavigationLeaving(s.parts[part].span);}
    if(needGeometry && captureNativeGeometry(s.geometry,revision,false)) {
        const auto& g=s.geometry.geometry;
        s.geometryValid=(!g.count || copyReadyRegion(g.triangles,s.triangles.data(),g.count*16,g.count*16)) &&
            (!g.vertexCount || copyReadyRegion(g.vertices,s.vertices.data(),g.vertexCount*8,g.vertexCount*8));
    }
    return s.epoch==generation();
}
void captureNavigationAreaNames(Scene& s,const AreaNameDemand& demand) {
    if(!nativeGameThread() || !cachedAreaNames.ready() || !demand.count || demand.epoch!=s.epoch || demand.revision!=s.revision || demand.count>AreaNameLimit) return;
    using Table=uintptr_t(__cdecl*)(unsigned);using Name=uintptr_t(__cdecl*)(unsigned,unsigned);
    const auto table=reinterpret_cast<Table>(address(0x4b7f10))(0);
    const unsigned count=table?read<uint16_t>(table)>>2:0;
    auto& cache=cachedAreaNames.get();
    const bool same=cache.ready && cache.table==table && cache.available==count && cache.demand.epoch==demand.epoch && cache.demand.revision==demand.revision;
    for(unsigned i=0;i<demand.count;++i) {
        auto& row=s.areaNames[i];row.index=demand.indices[i];row.length=0;
        const bool copied=same && i<cache.demand.count && cache.rows[i].index==row.index;if(copied) row=cache.rows[i];
        if(copied) continue;
        if(row.index>=0 && static_cast<unsigned>(row.index)<count)
            row.length=copyReadyGameText(reinterpret_cast<Name>(address(0x46c3c0))(static_cast<unsigned>(row.index),0),row.bytes.data(),row.bytes.size());
    }
    s.areaNameCount=demand.count;
    cache.table=table;cache.available=count;cache.demand=demand;cache.ready=table!=0;
    std::copy(s.areaNames.begin(),s.areaNames.begin()+s.areaNameCount,cache.rows.begin());
}
std::wstring sceneSwimmingInstruction(const Scene& s) {
    if(nativeGameThread()) return {};
    if(!s.swimming || !s.dive) return L"Dive first to enter the water, then request the route again";
    auto controls=s.controls;controls.swap=false;const auto context=copiedControlContext(controls);const auto& name=context.glyphs[0x32];
    auto text=L"Dive first. Hold "+(name.empty() || name==L"unassigned control"?std::wstring(L"the configured Dive button"):name)+L" and follow the route until the map changes";
    if(s.room==71 && s.area==20 && s.surfaceReady && s.surface<250) {
        const auto remaining=static_cast<unsigned>(std::ceil(std::clamp((250-s.player.y)/(250-s.surface),0.0,1.0)*100));
        text+=L". "+std::to_wstring(remaining)+L" percent of the descent remains";
    }return text;
}
std::wstring scenePersonName(const Scene& s,int index,const OwnedTextContext& controls) {
    if(nativeGameThread() || index<0 || static_cast<unsigned>(index)>=s.people.size()) return {};
    const auto& name=s.people[index];const auto parts=decodeOwnedBytes(name.bytes.data(),name.length,OwnedTextMode::Terminated,controls);
    return parts.empty()?std::wstring{}:parts.front();
}
InteractionZone sceneInteractionZone(const WorkerFact& w,unsigned channels,float extraTalkRange) {
    if(w.type==6) {auto z=playerEntryRegion(w.geometry,channels);z.worker=w.global;return z;}
    InteractionZone z;if(w.type<1 || w.type>3 || !channels) return z;const auto& g=w.geometry;
    z.worker=w.global;z.shape=w.type;z.center={field<float>(g,0),field<float>(g,4),field<float>(g,8)};
    z.end={field<float>(g,0x30),field<float>(g,0x34),field<float>(g,0x38)};z.angle=field<float>(g,0x34);
    z.width=std::abs(field<float>(g,0x30));z.depth=std::abs(field<float>(g,0x38));z.height=field<float>(g,w.type==1 && (channels&1)?0x2c:0x1c);
    z.range=channels&1?field<float>(g,0x40)+extraTalkRange:(channels&8) || (w.type==2 && (channels&0x20))?field<float>(g,0x48):0;
    z.talk=(channels&1)!=0;if(z.talk) z.viewAngle=field<float>(g,0x3c);z.crossing=!(channels&1) && (channels&16);
    z.valid=validNativePoint(z.center) && std::isfinite(z.range) && std::isfinite(z.height) && std::isfinite(z.angle) && std::isfinite(z.width) && std::isfinite(z.depth) && std::isfinite(z.viewAngle) && z.range>=0;return z;
}
Mesh decodeNavigationSceneMesh(const Scene& s) {
    if(nativeGameThread() || !s.geometryValid || s.geometry.geometry.count>s.triangles.size() || s.geometry.geometry.vertexCount>s.vertices.size()) return {};
    Mesh mesh;const auto& c=s.geometry;const auto& g=c.geometry;
    if(!g.count) return decodeOwnedNavigationMesh({}, {},g.scale,c.collision,c.swimming,c.floor,c.player,c.waterBake,c.waterCrossings,c.events);
    std::vector<RawNavigationTriangle> triangles(s.triangles.begin(),s.triangles.begin()+g.count);
    std::vector<RawNavigationVertex> vertices(s.vertices.begin(),s.vertices.begin()+g.vertexCount);
    return decodeOwnedNavigationMesh(triangles,vertices,g.scale,c.collision,c.swimming,c.floor,c.player,c.waterBake,c.waterCrossings,c.events);
}
namespace {
struct OwnedRow {bool valid=false;unsigned channels=0;};
struct ScanContext {const Scene& scene;unsigned part;std::array<OwnedRow,SceneWorkers>& rows;};
void inspectPart(const OwnedAtelGuardView& view,void* opaque) {
    auto& context=*static_cast<ScanContext*>(opaque);const auto& s=context.scene;
    for(unsigned i=0;i<s.count;++i) {
        const auto& fact=s.workers[i];if(!fact.valid || fact.part!=context.part || fact.local>=view.script.workers.size() || view.script.workers[fact.local].header!=fact.header) continue;
        auto& row=context.rows[i];row.valid=true;
        for(unsigned channel=0;channel<7;++channel) if(fact.interactions.channels&(1u<<channel))
            if(view.callbackAvailable(fact.local,fact.interactions.entries[channel],s.saved)) row.channels|=1u<<channel;
    }
}
std::vector<unsigned> entries(const WorkerFact& w,unsigned channels,EntryMask mask,unsigned* used=nullptr) {
    std::vector<unsigned> out;if(used) *used=0;
    for(unsigned c=0;c<7;++c) if((channels&(1u<<c)) && mask.contains(w.interactions.entries[c])) {
        out.push_back(w.interactions.entries[c]);if(used) *used|=1u<<c;
    }return out;
}
void competitors(Entity& entity,const Scene& s,const std::array<OwnedRow,SceneWorkers>& rows) {
    auto& z=entity.zone;z.worker=entity.worker;if(!z.valid || !z.talk) return;
    for(unsigned i=0;i<s.count;++i) {
        const auto& fact=s.workers[i];if(static_cast<int>(i)==entity.worker || !fact.valid || !(rows[i].channels&1)) continue;
        const auto other=sceneInteractionZone(fact,1,s.extraTalkRange);if(!other.valid || other.viewAngle<=0) continue;
        const double reach=z.range+z.width+z.depth+other.range+other.width+other.depth;
        if(distanceXZ(z.center,other.center)<=reach) z.competing.push_back(static_cast<const InteractionShape&>(other));
    }
}
bool activePedestal(const Scene& s,const Entity& e) {
    const auto& p=s.pedestal;return p.armed && p.epoch==s.epoch && p.control==e.pushControl && p.local==e.pushLocal && p.worker==e.worker && p.id==e.id;
}
PedestalPlan copiedPedestalPlan(const Scene& s,const WorkerFact& w,const Entity& e,const AtelScript& script) {
    const bool qualified=navigationCatalog(script) && (script.name=="bsyt0100" || script.name=="klyt1000" || script.name=="djyt0600" ||
        script.name=="mcyt0500" || script.name=="bvyt1100" || script.name=="dome0700");
    if(!qualified || !s.parts[w.part].bindingsReady) throw std::runtime_error("Pedestal observations are not ready");
    PedestalSnapshot snapshot;snapshot.position=e.position;const auto& bindings=s.parts[w.part].bindings;
    const auto value=[&](unsigned low) {for(unsigned i=0;i<bindings.count;++i) if(bindings.rows[i].binding==low) {snapshot.values[low]=bindings.rows[i].value;return;}throw std::runtime_error("Required pedestal observation is missing");};
    switch(e.pushControl) {
    case 1:value(0x5c0000cc);value(0x5c000100);break;
    case 2:value(0x0c00005a);value(0x0c000064);break;
    case 3:for(unsigned low:{0x99u,0x9au,0x9bu,0x9du,0x9eu,0xd9u,0xdbu}) value(0x0c000000+low);break;
    case 4:for(unsigned low:{0x96u,0x97u,0xa3u,0xc6u}) value(0x0c000000+low);break;
    case 5:value(0x0c0000de+e.pushLocal-56);break;
    case 6:value(0x0c000021);break;
    case 7:for(unsigned low:{7u,8u,0x1fu,0x20u,0x24u,0x29u}) value(0x0c000000+low);break;
    }
    auto plan=pedestalPlan(e.pushControl,e.pushLocal,snapshot);
    if(!plan.complete && plan.blocked.empty() && activePedestal(s,e)) plan=s.pedestal.plan;return plan;
}
void contactBounds(InteractionZone& zone,const ActorBody& actor,const ActorBody& player) {
    if((field<unsigned>(actor.bytes,0x194)&field<unsigned>(player.bytes,0x194)&0x40000000)!=0) {
        const double y=bodyPoint(player).y;zone.contactMinY=bottom(actor)-(field<float>(player.bytes,0x698)-y);
        zone.contactMaxY=field<float>(actor.bytes,0x698)-(bottom(player)-y);
    }
}
void pedestalZone(Entity& e,const Scene& s,const WorkerFact& w,const std::array<OwnedRow,SceneWorkers>& rows,const AtelScript& script) {
    if(!e.pushControl) return;
    const auto& player=s.actors[s.playerIndex];const auto* actor=w.actor>=0?&s.actors[w.actor]:nullptr;
    if(e.pushControl<=2) {
        e.inactive=(rows[w.global].channels&4)==0;auto& z=e.zone;z={};z.worker=e.worker;z.shape=1;z.center=e.position;z.height=-1;
        const double ar=actor?radius(*actor):0,pr=radius(player);z.contactRadius=ar+pr;
        if(actor) contactBounds(z,*actor,player);z.approach=e.pushControl==1?Point{0,0,1}:Point{};
        z.valid=actor && std::isfinite(ar) && ar>0 && std::isfinite(pr) && pr>0 && validNativePoint(z.center) &&
            std::isfinite(z.contactMinY) && std::isfinite(z.contactMaxY) && z.contactMinY<=z.contactMaxY;
        if(!z.valid) e.inactive=true;
    }
    const WorkerFact* edge=nullptr;
    if(e.pushControl==7) for(unsigned i=0;i<s.count;++i) if(s.workers[i].valid && s.workers[i].part==w.part && s.workers[i].local==39) {edge=&s.workers[i];break;}
    if(activePedestal(s,e) && s.pedestal.crossing && edge) {
        e.zone=sceneInteractionZone(*edge,16,s.extraTalkRange);e.zone.crossing=true;e.zone.talk=false;e.zone.height=2;
        e.inactive=!e.zone.valid || entries(*edge,rows[edge->global].channels,EntryMask{1u<<6}).empty();return;
    }
    if(activePedestal(s,e) && s.pedestal.contact) {
        const auto radius=e.zone.contactRadius,height=e.zone.height;e.zone={};e.zone.shape=1;e.zone.center=e.position;
        e.zone.range=radius-.15;e.zone.height=height;e.zone.valid=std::isfinite(radius) && radius>.5;e.inactive=e.inactive || !e.zone.valid;return;
    }
    try {
        const auto plan=copiedPedestalPlan(s,w,e,script);
        if(e.pushControl<=2) {
            if(!plan.moves.empty()) {const auto goal=plan.moves.front().goal;const double x=goal.x-e.position.x,z=goal.z-e.position.z;
                const auto delta=std::abs(x)>std::abs(z)?Point{x>0?1.0:-1.0,0,0}:Point{0,0,z>0?1.0:-1.0};e.zone.approach={-delta.x,0,-delta.z};}
            return;
        }
        if(e.pushControl==5 || e.pushControl==6) return;
        if(plan.complete || !plan.blocked.empty() || plan.moves.empty()) {e.inactive=true;return;}
        const auto p=e.position,goal=plan.moves.front().goal;const double x=goal.x-p.x,z=goal.z-p.z;
        const auto delta=std::abs(x)>std::abs(z)?Point{x>0?1.0:-1.0,0,0}:Point{0,0,z>0?1.0:-1.0};auto& zone=e.zone;
        if(e.pushControl==7 && plan.phase==0) {
            const auto edgeZone=edge?sceneInteractionZone(*edge,16,s.extraTalkRange):InteractionZone{};const auto r=radius(player);
            zone={};zone.shape=3;zone.width=zone.depth=.4;zone.height=1.5;
            zone.center={(edgeZone.center.x+edgeZone.end.x)*.5,edgeZone.center.y,(edgeZone.center.z+edgeZone.end.z)*.5-std::max(4.0,r+2)};
            zone.valid=edgeZone.valid && std::isfinite(r) && r>0;e.inactive=!zone.valid || !edge || entries(*edge,rows[edge->global].channels,EntryMask{1u<<6}).empty();return;
        }
        const double r=actor?radius(*actor)+radius(player):0,approach=std::max(5.0,r+.7);
        zone={};zone.shape=3;zone.height=9.5;zone.width=zone.depth=.4;zone.center={p.x-delta.x*approach,p.y,p.z-delta.z*approach};
        zone.obstacle=p;zone.obstacleRadius=r;zone.valid=std::isfinite(r) && r>0 && approach+.4<9.6;
        if(e.pushControl==7) {zone.height=2;zone.center={p.x,p.y,std::max(268.0,p.z+approach)};}e.inactive=!zone.valid;
    } catch(...) {if(e.pushControl>2 && e.pushControl!=5 && e.pushControl!=6) e.inactive=true;}
}
struct BuildContext {
    const Scene& scene;const std::array<OwnedRow,SceneWorkers>& rows;unsigned part;
    std::vector<Entity>& entities;std::vector<Entity>& butterflies;const SceneNameLookup& areas;const SceneNameLookup& people;
};
int partBinding(const PartFact& part,unsigned low,int fallback=-1) {
    if(!part.bindingsReady) return fallback;
    for(unsigned i=0;i<part.bindings.count;++i) if(part.bindings.rows[i].binding==low) return part.bindings.rows[i].value;return fallback;
}
InteractionZone touchZone(const Scene& s,const WorkerFact& w) {
    InteractionZone zone;if(w.actor<0) return zone;const auto& actor=s.actors[w.actor];const auto& player=s.actors[s.playerIndex];
    zone.shape=1;zone.height=-1;zone.worker=w.global;zone.center=w.position;
    const double ar=radius(actor),pr=radius(player);zone.range=ar+pr;contactBounds(zone,actor,player);
    zone.valid=std::isfinite(ar) && ar>0 && std::isfinite(pr) && pr>0 && std::isfinite(zone.range) && zone.range>0 && validNativePoint(zone.center);return zone;
}
bool visibleButterfly(const ActorBody& actor) {
    const auto& b=actor.bytes;const auto alpha=field<float>(b,0x354),targetAlpha=field<float>(b,0x358);
    return field<uint16_t>(b,0)==0x5008 && field<uint16_t>(b,2) && !(field<unsigned>(b,0x194)&0x100) &&
        !(field<uint8_t>(b,0x180) && !field<uint8_t>(b,0x181)) && std::isfinite(alpha) && std::isfinite(targetAlpha) && alpha>0 && targetAlpha>0;
}
const butterfly::Course* butterflyCourse(const AtelScript& script) {
    for(const auto& course:butterfly::courses) if(script.name==course.name && script.codeLength==course.length && script.checksum==course.hash && script.workers.size()==course.workers) return &course;return nullptr;
}
void butterflyStarter(Entity& e,const Scene& s,const WorkerFact& w,const OwnedRow& row,const butterfly::Course* course) {
    if(!course || w.local!=(course==&butterfly::courses[0]?19u:9u)) return;
    e.name=L"Butterfly hunt start";e.category=L"Object";e.butterfly=e.butterflyStart=true;e.hazardous=false;e.butterflyEpoch=s.epoch;e.usefulEntries=EntryMask(0x30u);
    e.inactive=true;e.zone.valid=false;
    if(w.actor<0 || !s.allowed || s.parts[w.part].butterflyExiting || partBinding(s.parts[w.part],0x0c000041)!=0 || !visibleButterfly(s.actors[w.actor])) return;
    unsigned channels=0;const auto selected=entries(w,row.channels,e.usefulEntries,&channels);
    if(!(channels&4) || std::find(selected.begin(),selected.end(),4u)==selected.end()) return;
    e.zone=touchZone(s,w);e.position=e.zone.center;e.distance=distanceXZ(s.player,e.position);e.inactive=!e.zone.valid;
}
void butterflyTarget(const Scene& s,const WorkerFact& w,const OwnedRow& row,const butterfly::Course* course,std::vector<Entity>& out) {
    if(!course || w.local<course->first || w.local>course->last || w.actor<0 || !s.allowed || s.parts[w.part].butterflyExiting ||
        partBinding(s.parts[w.part],0x0c000041)!=1 || !visibleButterfly(s.actors[w.actor]) || entries(w,row.channels,EntryMask(1u<<4)).empty()) return;
    const auto& b=s.actors[w.actor].bytes;const auto r=field<float>(b,0x334),g=field<float>(b,0x340),blueValue=field<float>(b,0x34c);
    const bool blue=std::abs(r-.1f)<.001f && std::abs(g-.3f)<.001f && std::abs(blueValue-1.f)<.001f;
    const bool red=std::abs(r-.9f)<.001f && std::abs(g-.1f)<.001f && std::abs(blueValue-.1f)<.001f;if(!blue && !red) return;
    Entity e;e.name=blue?L"Blue butterfly":L"Red butterfly";e.category=L"Object";e.butterfly=true;e.hazardous=red;e.worker=w.global;e.actor=w.actor;e.id=0xb000u+w.local;
    e.zone=touchZone(s,w);if(!e.zone.valid && blue) return;e.position=e.zone.center;e.distance=distanceXZ(s.player,e.position);e.usefulEntries=EntryMask(1u<<4);out.push_back(std::move(e));
}
void buildPart(const OwnedAtelGuardView& view,void* opaque) {
    auto& context=*static_cast<BuildContext*>(opaque);const auto& s=context.scene;const auto& script=view.script;const auto catalog=navigationCatalog(script);const auto course=butterflyCourse(script);
    const wchar_t* categories[]={L"",L"Exit",L"Event",L"Save Sphere",L"Treasure Chest",L"NPC",L"Object",L"Shop"};
    for(unsigned i=0;i<s.count;++i) {
        const auto& w=s.workers[i];const auto& row=context.rows[i];if(!row.valid || w.part!=context.part) continue;
        const auto index=w.local;butterflyTarget(s,w,row,course,context.butterflies);
        if(w.type==4) {
            if(catalog && script.name=="cdsp0000" && index==30 && s.primary==s.parts[w.part].span.chunk && s.room==71 &&
                (s.area==19 || s.area==20) && s.control && s.controlled && field<uint16_t>(s.actors[s.playerIndex].bytes,2)) {
                Entity goal;goal.worker=i;goal.swimDepth=true;goal.category=L"Exit";goal.name=L"Exit";goal.id=0xffff;goal.swimChunk=s.parts[w.part].span.chunk;
                goal.swimReady=s.swimming && s.dive;const auto clearance=field<float>(s.actors[s.playerIndex].bytes,0x4fc);
                goal.swimClearance=std::isfinite(clearance)?std::max(0.0,static_cast<double>(clearance)):0;goal.position=s.player;goal.position.y=251;goal.distance=std::max(0.0,251-s.player.y);
                const auto name=context.areas(areaIndex(288));if(!name.empty()) goal.name=L"Exit to "+name;context.entities.push_back(std::move(goal));
            }continue;
        }
        const auto target=navigationTarget(catalog,index);const bool bound=w.actor>=0;const auto model=bound?field<uint16_t>(s.actors[w.actor].bytes,0):0;
        const bool presence=navigationActorPresence(catalog,model,bound);
        const bool saveSphere=bound && navigationSaveSphereModel(model),nativeObject=saveSphere || (bound && model==0x5002);
        const auto useful=saveSphere || presence?EntryMask{}:target?target->entries:EntryMask{};unsigned channels=0;
        // Type6's native dispatcher schedules entry on channel5 and leave on6.
        // Default callback aliases on the other channels are not talk actions.
        const auto activeEntries=entries(w,w.type==6?row.channels&0x20u:presence?row.channels&1u:row.channels,useful,&channels);
        const bool trial=target && target->identity==2 && target->category==6 && (script.name=="bsyt0100" || script.name=="klyt1000" || script.name=="djyt0600" || script.name=="mcyt0500" || script.name=="bvyt1100" || script.name=="dome0700");
        // Authored entry regions remain identifiable while their native gate
        // is inactive, like trial controls. A missing prerequisite must not
        // silently remove one of the six seals from the player's list.
        const bool region=target && target->identity==3 && w.type==6;
        if(activeEntries.empty() && !trial && !region && !presence) continue;
        Entity e;e.worker=i;e.inactive=activeEntries.empty();e.swimApproach=catalog && script.name=="stbv0000" && (index==16 || index==17 || index==25 || index==26);
        e.usefulEntries=useful;e.authoredTrialControl=trial;
        if(bound && (!field<uint16_t>(s.actors[w.actor].bytes,2) || field<unsigned>(s.actors[w.actor].bytes,0x824)==0xcdcdcdcd)) continue;
        if(w.type==1 && !bound) continue;int room=-1;
        if(nativeObject) {
            if(model==0x5002 && !target) {const auto result=scanWorker(script,index,&activeEntries);if(!result.interactive && !result.request && !result.exit && !result.truncated && !result.unresolved) continue;}
            e.category=e.name=saveSphere?L"Save Sphere":L"Treasure Chest";
        } else if(target || presence) {e.category=categories[navigationEntityCategory(target,model,bound)];room=target?target->room:-1;e.name=navigationEntityName(target,model,bound,context.people);}
        else if(!catalog) {
            const auto result=scanWorker(script,index,&activeEntries);
            if(result.exit && w.type!=1) {e.category=e.name=L"Exit";room=result.room;}
            else if(result.interactive) {e.category=categories[navigationEntityCategory(nullptr,model,bound)];e.name=navigationEntityName(nullptr,model,bound,context.people);}else continue;
        } else continue;
        e.position=w.position;e.distance=distanceXZ(s.player,e.position);e.actor=w.actor;e.id=bound?model:e.category==L"Exit"?0xffff:0xfffe;
        configurePedestalTarget(e,catalog?catalog->name:nullptr,index);e.zone=sceneInteractionZone(w,channels?channels:trial || presence?1u:region?0x20u:0u,s.extraTalkRange);
        if(w.type==6 && !e.zone.valid) e.inactive=true;
        if(!e.pushControl || e.pushControl>2) competitors(e,s,context.rows);pedestalZone(e,s,w,context.rows,script);butterflyStarter(e,s,w,row,course);
        if(e.category==L"Exit" && room!=-2) {if(room<0) room=scanWorker(script,index,&activeEntries).room;e.name=navigationExitName(script,index,room,e.name,context.areas);}
        if(e.pushControl>=2 && e.pushControl<=4) {
            Entity recess=e;recess.name=L"Pedestal sphere recess";recess.pushControl=recess.pushLocal=0;recess.usefulEntries=EntryMask{1u<<2};
            recess.zone=sceneInteractionZone(w,1,s.extraTalkRange);competitors(recess,s,context.rows);recess.inactive=!(row.channels&1) || !recess.usefulEntries.contains(w.interactions.entries[0]);recess.unreachable=false;
            if(recess.zone.valid) context.entities.push_back(std::move(recess));
        }
        context.entities.push_back(std::move(e));
    }
}
}
bool scanNavigationScene(const Scene& s,std::vector<Entity>& entities,const SceneNameLookup& areas,const SceneNameLookup& people) {
    if(nativeGameThread() || s.epoch!=generation() || s.count>SceneWorkers || s.actorCount>SceneActors || s.partCount>SceneParts || s.playerIndex<0 || static_cast<unsigned>(s.playerIndex)>=s.actorCount) return false;
    std::array<OwnedRow,SceneWorkers> rows{};bool ready=true;std::vector<Entity> butterflies;
    std::array<bool,SceneParts> prepared{};
    for(unsigned part=0;part<s.partCount;++part) {
        checkComputation();if(s.epoch!=generation()) return false;
        ScanContext context{s,part,rows};prepared[part]=visitReadyAtelGuardedScript(s.parts[part].span,inspectPart,&context);
        // Original readScript rejected unsupported/malformed parts and omitted
        // those workers. Pending preparation must retain the entire request.
        if(!prepared[part] && atelPreparation(s.parts[part].span.chunk)!=AtelPreparation::Rejected) ready=false;
    }
    if(!ready) return false;
    // Each visit owns its immutable resource lease for the whole part. Only
    // primitive proof results cross visits; no borrowed script is retained.
    for(unsigned part=0;part<s.partCount;++part) {
        checkComputation();if(s.epoch!=generation()) {entities.clear();return false;}if(!prepared[part]) continue;
        BuildContext context{s,rows,part,entities,butterflies,areas,people};if(!visitReadyAtelGuardedScript(s.parts[part].span,buildPart,&context)) {entities.clear();return false;}
    }
    if(s.epoch!=generation()) {entities.clear();return false;}
    std::stable_sort(entities.begin(),entities.end(),[](const Entity& a,const Entity& b) {return a.worker<b.worker;});
    std::array<int,SceneActors> firstWorker;firstWorker.fill(-1);
    entities.erase(std::remove_if(entities.begin(),entities.end(),[&](const Entity& e) {
        if(e.actor<0 || static_cast<unsigned>(e.actor)>=SceneActors) return false;
        auto& first=firstWorker[e.actor];if(first<0) first=e.worker;return first!=e.worker;
    }),entities.end());
    if(!butterflies.empty()) {
        entities.erase(std::remove_if(entities.begin(),entities.end(),[&](const Entity& e) {
            return std::any_of(butterflies.begin(),butterflies.end(),[&](const Entity& b) {return e.actor==b.actor && e.worker==b.worker;});
        }),entities.end());entities.insert(entities.end(),butterflies.begin(),butterflies.end());
    }
    return true;
}
bool scenePedestalPlan(const Scene& s,const Entity& target,PedestalPlan& plan) {
    if(nativeGameThread() || target.worker<0 || static_cast<unsigned>(target.worker)>=s.count) return false;
    const auto& worker=s.workers[target.worker];if(!worker.valid || worker.part>=s.partCount) return false;
    struct Request {const Scene& scene;const WorkerFact& worker;const Entity& target;PedestalPlan& plan;bool ready=false;} request{s,worker,target,plan};
    const bool visited=visitReadyAtelScript(s.parts[worker.part].span,[](const AtelScript& script,void* opaque) {
        auto& q=*static_cast<Request*>(opaque);
        try {q.plan=copiedPedestalPlan(q.scene,q.worker,q.target,script);q.ready=true;}catch(...) {}
    },&request);
    return visited && request.ready;
}

}
