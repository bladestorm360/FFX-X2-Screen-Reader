#pragma once
#include "puzzle_guide.hpp"
namespace ffx {
// A checked step whose done binding no other checked step shares records one
// placement (a glyph touched, a sphere put in a recess or pedestal). Steps
// that share a binding read the carried-sphere variable, which changes as
// play moves on, so it cannot show that an earlier step was undone.
inline bool puzzleStepPlaced(const PuzzleRoute& route,unsigned index) {
    const auto& step=route.steps[index];if(!step.check) return false;
    for(unsigned i=0;i<route.steps.size();++i) {const auto& other=route.steps[i];
        if(i!=index && other.check && other.done.low==step.done.low && other.done.high==step.done.high && other.done.element==step.done.element) return false;}
    return true;
}
// expected: the step the guide expects next. placed: the latest placement the
// guide has passed. removed: a placement the route itself took back out, or
// one whose binding moved on to another nonzero value.
struct PuzzleProgress {unsigned expected=0;int placed=-1,removed=-1;};
// Done(step) reports whether a checked step currently reads as done.
// Cleared(step) reports whether its done binding reads empty (0), as a recess
// or pedestal does once its sphere is taken back out.
template<class Done,class Cleared> void advancePuzzleProgress(const PuzzleRoute& route,unsigned begin,unsigned end,PuzzleProgress& progress,Done&& done,Cleared&& cleared) {
    const auto& steps=route.steps;
    const auto passed=[&](unsigned i) {return static_cast<int>(i)==progress.removed || done(steps[i]);};
    const auto latestPlaced=[&]() {int latest=-1;for(unsigned i=begin;i<progress.expected && i<end;++i) if(puzzleStepPlaced(route,i) && passed(i)) latest=static_cast<int>(i);return latest;};
    const auto latestCheckpoint=[&]() {int latest=-1;for(unsigned i=begin;i+1<end;++i) if(steps[i].checkpoint && passed(i)) latest=static_cast<int>(i);return latest;};
    // The latest placement the guide passed no longer reads as done: the
    // player took that sphere back out. Move the expected step back to that
    // placement; the checkpoint jump and forward scan below then run from
    // there. One exception: some routes say to take a sphere back out (the
    // Besaid door, the Kilika entry door). When the next step is a
    // carried-sphere step that now reads as done, and it takes the sphere back
    // out of the placement, the removal is that route step, so the placement
    // stays passed for this trial context. The next step takes the sphere back
    // out when it targets the same worker as the placement (Kilika), or when
    // the placement is a checkpoint and the next step carries the same sphere
    // as the step before the placement (Besaid). A pickup of another sphere
    // with the same carried value (the second Djose door sphere) is not a
    // removal, so the guide moves back to the placement. A binding that moved
    // to another nonzero value was not emptied: the Bevelle rail pedestal
    // reads 1 on the entrance track and 19 once ridden. That placement stays
    // passed too.
    if(latestPlaced()<progress.placed) {
        const auto placed=static_cast<unsigned>(progress.placed),next=placed+1;
        const auto sameBinding=[](const UtilityBinding& a,const UtilityBinding& b) {
            return a.low==b.low && a.high==b.high && a.value==b.value && a.element==b.element;};
        const bool takesBack=steps[next].worker==steps[placed].worker ||
            (steps[placed].checkpoint && placed>begin && steps[placed-1].check && sameBinding(steps[placed-1].done,steps[next].done));
        if(!cleared(steps[placed])) progress.removed=progress.placed;
        else if(next+1<end && steps[next].check && !puzzleStepPlaced(route,next) && done(steps[next]) && takesBack) progress.removed=progress.placed;
        else progress.expected=static_cast<unsigned>(progress.placed);
    }
    // Checkpoints are durable native flags, so the guide may jump forward to
    // one. Otherwise it only moves forward one done step at a time, so a
    // carried sphere that is placed later does not move it backwards.
    const auto checkpoint=latestCheckpoint();
    if(checkpoint>=0 && static_cast<unsigned>(checkpoint)+1>progress.expected) progress.expected=static_cast<unsigned>(checkpoint)+1;
    while(progress.expected+1<end && passed(progress.expected)) ++progress.expected;
    progress.placed=latestPlaced();
}
}
