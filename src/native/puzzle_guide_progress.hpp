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
// guide has passed. removed: a placement the route itself took back out.
struct PuzzleProgress {unsigned expected=0;int placed=-1,removed=-1;};
// Done(step) reports whether a checked step currently reads as done.
template<class Done> void advancePuzzleProgress(const PuzzleRoute& route,unsigned begin,unsigned end,PuzzleProgress& progress,Done&& done) {
    const auto& steps=route.steps;
    const auto passed=[&](unsigned i) {return static_cast<int>(i)==progress.removed || done(steps[i]);};
    const auto latestPlaced=[&]() {int latest=-1;for(unsigned i=begin;i<progress.expected && i<end;++i) if(puzzleStepPlaced(route,i) && passed(i)) latest=static_cast<int>(i);return latest;};
    const auto latestCheckpoint=[&]() {int latest=-1;for(unsigned i=begin;i+1<end;++i) if(steps[i].checkpoint && passed(i)) latest=static_cast<int>(i);return latest;};
    // The latest placement the guide passed no longer reads as done: the
    // player took that sphere back out. Move the expected step back to that
    // placement; the checkpoint jump and forward scan below then run from
    // there. One exception: some routes say to take a sphere back out (the
    // Besaid door). When the next step is a carried-sphere step that now
    // reads as done, the removal is that route step, so the placement stays
    // passed for this trial context.
    if(latestPlaced()<progress.placed) {
        const auto next=static_cast<unsigned>(progress.placed)+1;
        if(next+1<end && steps[next].check && !puzzleStepPlaced(route,next) && done(steps[next])) progress.removed=progress.placed;
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
