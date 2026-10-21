// Verified-profile bootstrap and shared epoch/reset integration; see Docs/features/index.md, Docs/features/runtime/index.md and Docs/dylan_reports_20261018.md.
#include "events.hpp"
#include "build_version.hpp"
#include "utilities.hpp"
#include "profile.hpp"
#include "puzzle_solver.hpp"
#include "settings.hpp"
#include "nav_audio.hpp"
#include "mod_menu.hpp"
#include "movie_dialogue.hpp"
#include "text_encoding.hpp"
#include "minigames.hpp"
#include "minigame_completion.hpp"
#include "lightning.hpp"
#include "jecht.hpp"
#include "gagazet.hpp"
#include "sin_collection.hpp"
#include "butterfly.hpp"
#include "navigation.hpp"
#include "map_transition.hpp"
#include "copied_text.hpp"
#include "airship_search.hpp"
#include "owned_retirement.hpp"
#include "resource_reset.hpp"
#include "atel_resources.hpp"
#include <MinHook.h>
#include <atomic>

namespace ffx {
std::atomic<int> scene{0};
void resetStatusReview();
void installIntroCue();
void resetMessageViews();
namespace {
std::atomic<unsigned> features{0};
void teardown(Event& e) {
    e.before[30]=GetTickCount();
    invalidate(e.rva == 0x46fed0 ? "map-request" : e.rva == map_transition::Initialize ? "field-load-start" : "resource-load-start");
    // This listener intentionally measures its own return in the new resource
    // generation; other old-generation callbacks still remain invalidated.
    e.epoch=generation();
}
void resetOwnedReaders() {
    resetNavigation(); resetBattle(); resetMessageViews(); resetSystemMenus(); resetSphereGrid(); scene = 0;
    resetPuzzleSolver();resetAirshipSearch();
    resetLightning();
    resetJecht();
    resetGagazet();
    resetSinCollection();
    resetButterfly();
    resetMinigames();
    resetMinigameCompletion();
    resetModMenu(); resetNavAudio();
    resetSelectionDetail();

}
void loadFinished(Event& e) {
    recordLog("native load return rva=%x owner=%lu total-ms=%lu",e.rva,GetCurrentThreadId(),GetTickCount()-static_cast<DWORD>(e.before[30]));
}
void fieldFinished(Event& e) {
    loadFinished(e); publishFieldArrival(static_cast<unsigned>(e.args[0]),e.epoch);
    if(nativeGameThread()) {NativeReaderAccess access;}
}
void completedArrival(unsigned room,unsigned epoch) {
    Event e; e.args[0]=room; e.epoch=epoch; announceMapArrival(e);
}
}
unsigned initializeReader(bool& initializedHooks) {
    if (!FfxNative_SpeechReady()) { recordLog("native readers unavailable: speech initialization failed"); return 0; }
    wchar_t exe[MAX_PATH]{}; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    const auto leaf = wcsrchr(exe, L'\\');
    const bool movieOnly=_wcsicmp(leaf ? leaf+1:exe,L"FFX&X-2_Will.exe")==0;
    if (!movieOnly && _wcsicmp(leaf ? leaf + 1 : exe, L"FFX.exe") != 0) {
        recordLog("native game hooks skipped: no verified addresses for this executable"); return 0;
    }
    if (!configureNativeProfile()) return 0;
    if(movieOnly!=(nativeProfile==NativeProfile::MovieCaptions)) {
        recordLog("native readers refused: executable name/profile disagree"); return 0;
    }
    if (!resolveNativeExtensions()) { recordLog("native readers refused: extension address verification failed"); return 0; }
    if (!validateNativeDependencies()) { recordLog("native readers refused: feature dependency verification failed"); return 0; }
    if (MH_Initialize() != MH_OK) return 0;
    initializedHooks=true;
    if(movieOnly) {
        installMovieDialogue(true);
        if(!enableHooks()) return 0;
        features=2;
        recordLog("native production readers enabled: Will captions and movie audio; saved FFX dialogue policy applies");
        return features;
    }
    if(!startAtelResources()) {recordLog("native ATEL resource owner unavailable");return 0;}
    installAtelResources();
    installTextEncoding();
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&startReader),&module);
    if(!startCopiedText(module)) { recordLog("native copied text worker unavailable"); return 0; }
    // Initialize the device on the proxy bootstrap worker before hooks run.
    // Actor allocation/movement callbacks must never open an audio device.
    startNavAudio();
    installGameContext();
    hook(0x46fed0, teardown, loadFinished);
    hook(0x5093b0, teardown, loadFinished);
    hook(map_transition::Initialize, teardown, fieldFinished);
    installFieldMenus(); installMessages(); installSystemMenus();
    installSphereGrid(); installBattle(); installNavigation(); installMinigames(); installMinigameCompletion(); installButterfly(); installInput(); installUtilities(); installLightning(); installJecht(); installGagazet(); installSinCollection(); installAirshipSearch();installIntroCue();
    retirement::prepare<std::wstring>();
    if(!retirement::start(module)) {recordLog("native retirement worker unavailable");return 0;}
    configureOwnerReset(resetOwnedReaders,completedArrival);
    if (!enableHooks()) { recordLog("native reader batch failed; restore_before_native.bat provides rollback"); return 0; }
    features = 0x1ff;
    recordLog("native production readers enabled: field,message,system,sphere,battle,navigation,minigames,input,utilities; Gadget unused");
    return features;
}
unsigned startReader() {
    recordLog("FFX Screen Reader release %s",FFX_RELEASE_VERSION);
    bool initializedHooks=false;
    try { return initializeReader(initializedHooks); }
    catch(const std::exception& error) {
        features=0;
        if(initializedHooks) discardHooks();
        recordLog("native reader startup refused: %s",error.what());
    } catch(...) {
        features=0;
        if(initializedHooks) discardHooks();
        recordLog("native reader startup refused: unexpected exception");
    }
    return 0;
}
}
extern "C" unsigned __cdecl FfxNative_Features() { return ffx::features.load(); }
