#pragma once
#include "controller_model.hpp"
#include <cstdint>
namespace ffx::physical {
struct Frame { pad::State state; uint64_t stamp=0,inputStamp=0,counter=0,frequency=0; unsigned device=0,focus=0,overflow=0,connected=0,focused=0,pressure=0; };
struct Edge { pad::State state; uint64_t stamp=0; unsigned device=0,focus=0,overflow=0; };
void startOwner(); // bootstrap only; SDK owner remains parked until native readiness
void requestReady(); // native: nonblocking publication only
bool snapshot(Frame&);
bool fresh(uint64_t counter,uint64_t frequency); // QPC age <=10ms; one read, never waits.
bool nextEdge(Edge&);
bool requestRumble(unsigned motors,const Frame&);
bool audioAvailable();
void waitForAudio(); // audio device worker only; never a native callback
#if defined(FFX_READER_TEST) && !defined(FFX_SDL_OWNER_TEST)
void manualOwnerStep(); // isolated virtual-pad fixture already owns SDL on this thread
#endif
#ifdef FFX_SDL_OWNER_TEST
void ownerStepForTest();
#endif
}
