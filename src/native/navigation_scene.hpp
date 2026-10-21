#pragma once
#include "navigation_capture.hpp"
#include "copied_controls.hpp"
#include "atel_resources.hpp"
#include "pedestal.hpp"
#include <functional>

namespace ffx::nav {
constexpr unsigned SceneWorkers=500,SceneActors=128,SceneParts=64;
// These exact original object extents are copied on their native owner. Workers
// interpret only the copied scalar fields below, never any pointer in a body.
struct SceneName {std::array<uint8_t,40> bytes{};unsigned length=0;};
constexpr unsigned AreaNameLimit=256;
struct AreaNameDemand {unsigned epoch=0,revision=0,count=0;std::array<int,AreaNameLimit> indices{};};
struct SceneAreaName {int index=-1;unsigned length=0;std::array<uint8_t,256> bytes;};
struct ActorBody {std::array<uint8_t,0x880> bytes;};
struct WorkerFact {
    unsigned part=SceneParts,local=0,header=0,type=0,global=0;
    int actor=-1;
    NativeInteractionFacts interactions;
    std::array<uint8_t,0x4c> geometry;
    Point position;
    bool valid=false;
};
struct SceneValue {unsigned binding=0;int value=0;unsigned high=1,element=0;};
struct SceneBindings {std::array<SceneValue,32> rows{};unsigned count=0;};
struct PartFact {AtelSpan span;SceneBindings bindings;bool bindingsReady=false,butterflyExiting=false,guideReady=false,bindingsExpected=false,identityReady=false;};
struct Scene {
    ULONGLONG capturedAt=0;
    unsigned epoch=0,revision=0,count=0,actorCount=0,partCount=0,room=0,area=0;
    int playerIndex=-1;
    uintptr_t controller=0,primary=0;
    Point player,forward,right;
    float extraTalkRange=0,surface=0;
    unsigned confirmMask=0,audioContext=0,collisionSerial=0;
    bool surfaceReady=false;
    bool allowed=false,cameraValid=false,control=false,controlled=false,swimming=false,dive=false,geometryValid=false;
    // Exact Baaj request-time diagnostic facts, captured with its existing
    // native scene event. No polling or game pointer crosses into preparation.
    bool baajFactsReady=false;int baajTreasureFlags=-1,baajActivatedCount=-1;
    CopiedControls controls;
    std::array<SceneName,20> people;
    unsigned areaNameCount=0;
    std::array<SceneAreaName,AreaNameLimit> areaNames;
    AtelSavedFacts saved;
    PedestalNavigationState pedestal;
    std::array<PartFact,SceneParts> parts;
    std::array<WorkerFact,SceneWorkers> workers;
    std::array<ActorBody,SceneActors> actors;
    NativeGeometryContext geometry;
    std::array<RawNavigationTriangle,32767> triangles;
    std::array<RawNavigationVertex,32767> vertices;
};
static_assert(std::is_trivially_copyable_v<Scene> && std::is_trivially_destructible_v<Scene>);
bool captureNavigationScene(Scene&,unsigned revision,bool geometry);
void prepareNavigationSceneStorage(); // Bootstrap, fixed Native title-cache storage.
void captureNavigationAreaNames(Scene&,const AreaNameDemand&);
// Native title/name getters are supplied separately as copied encoded facts.
// Both lookups run on the preparation owner and may record missing IDs for the
// next bounded native observation. They must never enter engine getters.
using SceneNameLookup=std::function<std::wstring(int)>;
bool scanNavigationScene(const Scene&,std::vector<Entity>&,const SceneNameLookup& areas,const SceneNameLookup& people);
void stageAirshipPassages(const Scene&,const Mesh&,std::vector<Entity>&);
Mesh decodeNavigationSceneMesh(const Scene&);
std::wstring sceneSwimmingInstruction(const Scene&);
std::wstring scenePersonName(const Scene&,int,const OwnedTextContext&);
bool scenePedestalPlan(const Scene&,const Entity&,PedestalPlan&);
InteractionZone sceneInteractionZone(const WorkerFact&,unsigned channels,float extraTalkRange);
}
