#pragma once
#include "atel.hpp"
#include <cstdint>
#include <array>
namespace ffx {
constexpr unsigned AtelResourceLimit=0x800000,AtelResourceSlots=4,AtelResourceParts=11;
// epoch is the current reader observation epoch; serial identifies the engine
// allocation lifetime. A graphics/speech reset alone does not free the file.
struct AtelSpan {uintptr_t chunk=0;unsigned epoch=0,serial=0,size=0;};
struct AtelIdentity {AtelSpan span;unsigned checksum=0,length=0,codeOffset=0,workers=0;char name[12]{};};
enum class AtelPreparation : unsigned {Missing,Pending,Ready,Rejected,Retired};
// Rejected is terminal for the exact receipt. Pending never means an
// unsupported file, and does not authorize a native parser/hash fallback.
AtelPreparation atelPreparation(uintptr_t chunk);
using AtelPreparedHandler=void(*)() noexcept;
// Bootstrap-only registration; handlers signal copied-data consumers and
// must not enter engine getters or native feature code on the resource worker.
bool onAtelPrepared(AtelPreparedHandler);
// Bootstrap starts the sole parsing/hash/cache owner before resource hooks run.
bool startAtelResources();
void installAtelResources();
// One bounded read. Engine identities are opaque; only the originating native
// owner captures ready bytes. An absent/pending result never permits guessing a bound.
bool nativeAtelSpan(uintptr_t chunk,AtelSpan&);
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity&);
bool readyAtelDeclaration(uintptr_t chunk,unsigned header,unsigned low,unsigned high,unsigned& index);
bool readyAtelDeclarationAt(uintptr_t chunk,unsigned header,unsigned index,unsigned& low,unsigned& high);
bool readyAtelWorkerHeader(uintptr_t chunk,unsigned local,unsigned& header);
// Native copies the default saved bank only when controller+2C has the actual
// SaveData+1EC owner. Original878170 establishes total SaveData extent68C0.
struct AtelSavedFacts {std::array<uint8_t,0x68c0-0x1ec> bytes{};unsigned valid=0;};
// Preparation owner only, with exact opaque allocation identity and copied
// saved facts. A false return is unavailable/pending, never an available guard.
bool readyAtelCallbackAvailable(const AtelSpan&,unsigned local,unsigned entry,const AtelSavedFacts&,bool& available);
// Borrowed immutable resource view. The opaque context is mod-owned prepared
// storage protected by the enclosing lease, never an engine pointer. A consumer
// may use it only during visitReadyAtelGuardedScript; it cannot retain the view.
struct OwnedAtelGuardView {
    const AtelScript& script;
    const void* context;
    bool callbackAvailable(unsigned local,unsigned entry,const AtelSavedFacts&) const;
};
using OwnedAtelGuardConsumer=void(*)(const OwnedAtelGuardView&,void*);
bool visitReadyAtelGuardedScript(const AtelSpan&,OwnedAtelGuardConsumer,void*);
// Preparation-owner-only synchronous view. The immutable script reference is
// borrowed for this call; consumers must not retain it beyond the callback.
using OwnedAtelConsumer=void(*)(const AtelScript&,void*);
bool visitReadyAtelScript(const AtelSpan&,OwnedAtelConsumer,void*);
#ifdef FFX_READER_TEST
void drainAtelResourcesForTest();
unsigned rejectedAtelResourcesForTest();
size_t preparedAtelBytesForTest();
#endif
}
