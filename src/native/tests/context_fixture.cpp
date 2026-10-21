#include <atomic>
#include "../events.hpp"
#include "../navigation.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
namespace ffx {
std::atomic<int> scene{0};
unsigned mask=0,paused=0,resumed=0,details=0,status=0,audio=0;
struct Listener { Handler before,after; };
std::map<unsigned,Listener> listeners;
unsigned __cdecl active() { return mask; }
uintptr_t address(unsigned) { return reinterpret_cast<uintptr_t>(active); }
bool hook(unsigned id,Handler before,Handler after) { listeners[id]={before,after}; return true; }
void pauseNavAudio() { ++paused; }
void updateNavAudio(Point,bool) { ++resumed; }
Point playerPosition() { return {}; }
void refreshMinigameAudio() { ++audio; }
void resetSelectionDetail() { ++details; }
void resetStatusReview() { ++status; }
void helpSceneChanged() {} // Help owns its separate renderer/context fixture.
void log(const char*,...) {}
unsigned generation() { return 1; }
}
using namespace ffx;
void check(bool value,const char* reason) { if(!value) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
void event(unsigned id,unsigned value) { Event e; e.args[0]=1; if(listeners[id].before) listeners[id].before(e); mask=value; listeners[id].after(e); }
int main() {
    installGameContext();
    event(0x4aa0b0,2); check(scene==1,"native main menu owns context");
    event(0x4aa0b0,2|(1u<<19)); check(scene==19,"nested overlay has priority over its parent");
    event(0x4aad20,2); check(scene==1,"closing overlay restores still-open parent");
    event(0x4aaad0,0); check(!scene && resumed==1 && audio==1,"closing parent restores field navigation and console context");
    event(0x4aa0b0,1u<<22); check(scene==22,"native remapping is observed after the open call");
    event(0x4aad20,0); check(!scene && resumed==2,"closing remapped overlay restores field context");
    event(0x4aa0b0,1); check(!scene,"field window zero is compatible with navigation");
    std::puts("PASS: native window open, remap, nesting, close/delete and field restoration.");
}
