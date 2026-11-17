// Exercise the actual native-owner resume path without bootstrapping a game.
// Unrelated bootstrap endpoints fail immediately if the fixture reaches them.
#include "../reader.cpp"
#include "../navigation_observations.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
unsigned epoch=1,checks=0,maps=0,messages=0,battles=0,grids=0,navigationCaptures=0;
uint16_t room=23;
bool field=false,message=false,battle=false,grid=false,resetReady=true;
int visibleScene=0;
std::vector<std::wstring> said;
void check(bool ok,const char* why) {++checks;if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
}
namespace ffx {
#define UNUSED_READER_ENDPOINT(name) void name() {std::abort();}
UNUSED_READER_ENDPOINT(discardHooks)
UNUSED_READER_ENDPOINT(installFieldMenus) UNUSED_READER_ENDPOINT(installUnavailable)
UNUSED_READER_ENDPOINT(installMessages) UNUSED_READER_ENDPOINT(installSystemMenus)
UNUSED_READER_ENDPOINT(installSphereGrid) UNUSED_READER_ENDPOINT(installBattle)
UNUSED_READER_ENDPOINT(installNavigation) UNUSED_READER_ENDPOINT(installInput)
UNUSED_READER_ENDPOINT(resetNavigation) UNUSED_READER_ENDPOINT(resetBattle)
UNUSED_READER_ENDPOINT(installGameContext) UNUSED_READER_ENDPOINT(resetSystemMenus)
UNUSED_READER_ENDPOINT(resetSphereGrid) UNUSED_READER_ENDPOINT(resetSelectionDetail)
UNUSED_READER_ENDPOINT(installUtilities) UNUSED_READER_ENDPOINT(installAtelResources)
UNUSED_READER_ENDPOINT(resetPuzzleSolver) UNUSED_READER_ENDPOINT(startNavAudio)
UNUSED_READER_ENDPOINT(resetNavAudio) UNUSED_READER_ENDPOINT(resetModMenu)
UNUSED_READER_ENDPOINT(installTextEncoding) UNUSED_READER_ENDPOINT(installMinigames)
UNUSED_READER_ENDPOINT(resetMinigames) UNUSED_READER_ENDPOINT(installMinigameCompletion)
UNUSED_READER_ENDPOINT(resetMinigameCompletion) UNUSED_READER_ENDPOINT(installLightning)
UNUSED_READER_ENDPOINT(resetLightning) UNUSED_READER_ENDPOINT(installJecht)
UNUSED_READER_ENDPOINT(resetJecht) UNUSED_READER_ENDPOINT(installGagazet)
UNUSED_READER_ENDPOINT(resetGagazet) UNUSED_READER_ENDPOINT(installSinCollection)
UNUSED_READER_ENDPOINT(resetSinCollection) UNUSED_READER_ENDPOINT(installButterfly)
UNUSED_READER_ENDPOINT(resetButterfly) UNUSED_READER_ENDPOINT(installAirshipSearch)
UNUSED_READER_ENDPOINT(resetAirshipSearch) UNUSED_READER_ENDPOINT(installIntroCue)
UNUSED_READER_ENDPOINT(resetMessageViews)
#undef UNUSED_READER_ENDPOINT
bool hook(unsigned,Handler,Handler) {std::abort();}
bool enableHooks() {std::abort();}
bool tryNativeReaderAccess() noexcept {std::abort();}
void releaseNativeReaderAccess() noexcept {std::abort();}
bool validateNativeDependencies() {std::abort();}
bool configureNativeProfile() {std::abort();}
bool resolveNativeExtensions() {std::abort();}
bool startAtelResources() {std::abort();}
bool startCopiedText(HMODULE) {std::abort();}
void installMovieDialogue(bool) {std::abort();}
void announceMapArrival(Event&) {std::abort();}
void configureOwnerReset(void(*)(),void(*)(unsigned,unsigned)) {std::abort();}
void publishFieldArrival(unsigned,unsigned) noexcept {std::abort();}
namespace retirement {
bool registerPool(PoolBase*) noexcept {std::abort();}
bool start(HMODULE) noexcept {std::abort();}
}
unsigned generation() {return epoch;}
void invalidate(const char*) {++epoch;}
void interruptSpeechForInput() {}
bool output(const wchar_t* text,bool,const char*) {said.emplace_back(text);return true;}
void protectSpeech(bool) {}
void cancelAutoWalk() {}
void stopNavPreview() {}
void resetModInput() {}
void resetDirectInput() {}
void resetControllerInput() {}
void invalidateUtilityContext() {}
bool serviceOwnerReset() noexcept {return resetReady;}
void refreshGameContext() {scene=visibleScene;}
bool refreshMessageScreen() {++messages;return message;}
bool inBattle() {return battle;}
bool sphereGridOpen() {return grid;}
bool activeField() {return field;}
void refreshBattleScreen() {++battles;}
void refreshSphereScreen() {++grids;}
void announceCurrentMap() {++maps;}
bool nativeGameThread() {return true;}
bool nativeReaderResourcesReady() noexcept {return true;}
uintptr_t address(unsigned rva) {return rva==0xd2ca90?reinterpret_cast<uintptr_t>(&room):0;}
bool copy(uintptr_t source,void* target,size_t bytes) {if(!source)return false;std::memcpy(target,reinterpret_cast<void*>(source),bytes);return true;}
bool onAtelPrepared(AtelPreparedHandler) {return true;}
unsigned autoWalkInputRevision() {return 1;}
namespace nav {
unsigned navigationIntentRevision() {return 1;}
void prepareNavigationSceneStorage() {}
bool captureNavigationScene(Scene&,unsigned,bool) {++navigationCaptures;return false;}
void captureNavigationAreaNames(Scene&,const AreaNameDemand&) {}
}
namespace speechq {
void registerDomain(unsigned,DomainHandler,DomainReset) {}
void registerDomainMaintenance(unsigned,DomainMaintenance) {}
void notifyPreparation() noexcept {}
bool captureDomain(unsigned,const void*,unsigned,const wchar_t*,unsigned,unsigned) {return true;}
}
}
extern "C" int __cdecl FfxNative_SpeechReady() {std::abort();}
extern "C" MH_STATUS WINAPI MH_Initialize() {std::abort();}
int main() {
    using namespace ffx;
    toggleOnOwner(1);check(!modEnabled,"owner disables");
    toggleOnOwner(1);check(modEnabled && modResumeEpoch==generation(),"owner publishes fresh resume epoch");
    check(maps==0 && messages==1,"title/load dialog resume never reads a nonexistent field map");
    visibleScene=3;field=true;toggleOnOwner(2);
    check(maps==0,"field menu resume leaves focus to its native menu observation");
    visibleScene=0;message=true;toggleOnOwner(2);check(maps==0,"visible message takes precedence over map");
    message=false;battle=true;toggleOnOwner(2);check(battles==1 && maps==0,"battle refresh stays on native owner");
    battle=false;grid=true;toggleOnOwner(2);check(grids==1 && maps==0,"Grid refresh stays on native owner");
    grid=false;toggleOnOwner(2);check(maps==1,"valid field resume still reads current map");
    resetReady=false;toggleOnOwner(2);const auto before=messages;
    toggleOnOwner(0);check(messages==before,"unready reset never captures current screen");
    resetReady=true;toggleOnOwner(0);check(messages==before+1,"reset recovery refreshes once without another key");
    toggleOnOwner(0);check(messages==before+1,"idle resume does not repeat");
    nav::startNavigationObservations(nullptr,nullptr,nullptr);
    said.clear();room=23;
    check(nav::submitNavigationQuery(nav::Query::Refresh),"query is copied without native reads");
    nav::captureRequestedNavigationScene(true);
    check(navigationCaptures==0 && said.empty(),"title/load UI never produces a field navigation failure");
    nav::resetNavigationObservations();room=71;
    check(nav::submitNavigationQuery(nav::Query::Refresh),"field query accepted");
    nav::captureRequestedNavigationScene(true);
    check(navigationCaptures==1 && said.size()==1,"genuine missing field data still reports an explicit query failure");
    std::printf("PASS: mod resume owner/context and title navigation gates, %u checks.\n",checks);
}
