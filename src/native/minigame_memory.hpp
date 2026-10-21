#pragma once
#include "minigame_data.hpp"
#include "navigation.hpp"
#include "atel_resources.hpp"
namespace ffx {
enum class MinigameKind : unsigned { None,Blitz,Race };
struct MinigameScript {
    uintptr_t chunk=0,worker=0; unsigned workers=0; MinigameKind kind=MinigameKind::None;
    AtelSpan span;unsigned length=0,codeOffset=0;
};
struct MinigameActor { Point position; uintptr_t worker=0,actor=0; unsigned local=0,index=0,model=0; float radius=0,height=0; bool valid=false; };
MinigameScript minigameScript(uintptr_t worker=0);
// Capture uses only an exact native resource span and fixed ready facts.
// Classification/checksum qualification happens against owned preparation.
MinigameScript captureMinigameScript(uintptr_t worker=0);
AtelPreparation qualifyMinigameScript(const MinigameScript&);
MinigameActor minigameActor(const MinigameScript&,unsigned local);
bool captureMinigameValue(const MinigameScript&,const MinigameBinding&,unsigned element,float&);
const MinigameBinding* minigameBinding(const MinigameScript&,unsigned index);
}
