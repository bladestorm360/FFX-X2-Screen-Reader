#pragma once
#include "navigation_scene.hpp"
#include "speech_domains.hpp"
#include <atomic>
namespace ffx::nav {
enum class Query : unsigned {Refresh,Scan,Next,Previous,Distance,Path,Filter,Beacon,Collision,Motion,AssistRoute,AssistInstruction,GuideStep,GuideNext,GuidePrevious,GuideExpected,GuidePath,GuideCompleted,GuideTileCompleted,GuideTarget};
struct QueryObservation {unsigned epoch=0,revision=0,serial=0,requested=0;Query query=Query::Refresh;bool shift=false;unsigned manual=0,intentRevision=0,policyRevision=0,local=0;uintptr_t chunk=0;unsigned resourceSerial=0;};
struct SceneObservation {const Scene& scene;unsigned requested=0;ULONGLONG capturedAt=0;};
// Bootstrap registration. Only the central preparation owner invokes consumers.
// A scene reference is borrowed for this callback; retained facts must be copied.
using QueryConsumer=void(*)(const speechq::Header&,const QueryObservation&);
using SceneConsumer=void(*)(const speechq::Header&,const SceneObservation&);
using ObservationReset=void(*)(unsigned);
void startNavigationObservations(QueryConsumer,SceneConsumer,ObservationReset);
bool submitNavigationQuery(Query,bool shift=false,unsigned policyRevision=0,unsigned local=0,uintptr_t chunk=0,unsigned resourceSerial=0); // Scalar-only input observation; no engine access.
bool captureRequestedNavigationScene(bool geometry,bool changed=false); // Native owner only.
void navigationWaitingForData(bool); // Preparation owner declares an outstanding dependency.
bool navigationDataPending(); // Atomic event interest, including a first pending command.
void requestNavigationScene(); // Copied-data dependency completed; no native work on caller.
void requestNavigationAreaNames(const AreaNameDemand&); // Single preparation publisher; Native getter handback.
void resetNavigationObservations(); // POD/atomic invalidation only, safe on foreign teardown.
unsigned navigationObservationRevision();
const std::atomic<unsigned>& navigationRevisionCounter(); // Copied-computation cancellation only.
#ifdef FFX_READER_TEST
unsigned navigationSceneSlotsForTest();
#endif
}
