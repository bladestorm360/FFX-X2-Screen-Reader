// Reuse only the synthetic native/input and semantic speech adapters. Both
// route retirement and Auto walk below link their production implementations.
#define FFX_NAV_CUES_INTEGRATION_TEST
#define main autoWalkComponentFixtureEntry
#include "auto_walk_fixture.cpp"
#undef main
#include "../navigation_scene.hpp"
#include "../navigation_observations.hpp"
#include "test_speech_domains.hpp"

namespace ffx {
void prepareNavRetirement();
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
bool cameraBasis(Point& f,Point& right) {f=forward;right={f.z,0,-f.x};return camera;}
std::vector<Entity> butterflyTargets() {return {};}
void refreshButterflyTarget(Entity&) {}
void refreshButterflyStarter(Entity&) {}
void installWallCues() {}
void captureWallCues() {}
void cancelWallCues() {}
void publishNavAudio(const NavAudioState&) {}
void pauseNavAudio() {}
void pauseNavRouteAudio() {}
void stopNavPreview() {}
void navAudioArrival() {}
void publishPositionalCue(float,float,bool,unsigned) {}
}
namespace ffx::speechq {void notifyPreparation() noexcept {}}
namespace ffx::nav {void requestNavigationScene() {}}

int main() {
    using namespace ffx;
    memory=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(memory!=0,"isolated native field configuration");prepareNavRetirement();
    static nav::Scene observation;Mesh mesh;mesh.openPlane=mesh.fixedHeight=true;
    Entity target;target.name=L"Cancel fixture goal";target.position={0,0,100};target.worker=1;
    observation.control=observation.controlled=true;
    for(bool nativeClear:{false,true}) {
        nativeOwner=false;observation.epoch=epoch;observation.allowed=true;observation.cameraValid=true;
        observation.forward={0,0,1};observation.right={1,0,0};observation.audioContext=navAudioContextSerial();observation.collisionSerial=navCollisionSerial();
        speechq::Header header{};header.generation=epoch;
        updateOwnedNavAudio(observation,mesh,{target},header,L"");routeNavAudio({{},target.position},target);
        nativeOwner=true;begin();check(autoWalkActive(),"real route starts real Auto walk");speech.clear();
        nativeOwner=nativeClear;clearNavRoute();
        nativeOwner=true;updateAutoWalk();
        check(speech.empty() && !autoWalkActive(),"clearNavRoute then updateAutoWalk before explicit stop stays silent");
        stopAutoWalk();
    }
    nativeOwner=false;speechq::Header header{};header.generation=epoch;
    updateOwnedNavAudio(observation,mesh,{target},header,L"");
    routeNavAudio({{},target.position},target);
    nativeOwner=true;begin();check(keyboard()[0xc8],"fresh route supplies a movement mask");
    nativeOwner=false;updateOwnedNavAudio(observation,mesh,{},header,L"");
    nativeOwner=true;check(!keyboard()[0xc8] && autoWalkActive(),"first missing observation invalidates the old mask immediately");
    pad::State smallInput;smallInput.lx=2000;autoWalkController(smallInput);
    check(smallInput.lx==2000 && !smallInput.ly,"missing observation preserves the player's original stick input");
    nativeOwner=false;updateOwnedNavAudio(observation,mesh,{target},header,L"");
    nativeOwner=true;check(!keyboard()[0xc8],"a recovered worker route does not re-enable the pre-gap mask");
    updateAutoWalk();check(keyboard()[0xc8],"fresh native steering resumes after the target returns");
    VirtualFree(reinterpret_cast<void*>(memory),0,MEM_RELEASE);
    std::puts("PASS: production route-clear/Auto walk interleaving stays silent for native and worker cancellation.");
}
