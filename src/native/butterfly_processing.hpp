#pragma once
#include "butterfly.hpp"
#include "atel_resources.hpp"
namespace ffx::butterfly {
struct Course {const char* name;unsigned hash,length,workers,first,last,timer,active,count,time;};
inline constexpr Course courses[]={
    {"mcfr0100",0x908ca3c3,78486,55,20,26,37,26,27,50},
    {"mcfr0200",0xc248925f,14255,40,10,16,26,20,21,35}
};
struct TargetFact {
    Point position;InteractionShape zone;
    double contactMinY=-1e30,contactMaxY=1e30;
    NativeInteractionFacts interactions;
    unsigned local=0,id=0,flags=0;int actor=-1,worker=-1;
    float red=0,green=0,blue=0,alpha=0,targetAlpha=0;
    bool bound=false,enabled=false,visible=false;
};
enum Command : unsigned {Observe,Status,Blue,Red};
struct Frame {
    AtelSpan span;AtelSavedFacts saved;
    std::array<TargetFact,7> targets;
    Point player,forward,right;
    unsigned epoch=0,serial=0,course=0,command=Observe,ticket=0,ticketSerial=0;
    int caught=-1,tenths=-1,seconds=-1,minutes=-1;
    bool active=false,announce=false,collection=false,exiting=false,allowed=false,cues=false,cameraValid=false;
};
struct Targets {
    std::array<TargetFact,7> facts;
    unsigned epoch=0,serial=0,count=0;bool active=false;
};
void installProcessing();
bool submit(Frame&); // Native copied-fact producer only.
void resetProcessing(); // Native POD/atomic invalidation only.
bool processingActive();
bool copiedTargets(Targets&); // One bounded immutable read, no engine getter.
}
