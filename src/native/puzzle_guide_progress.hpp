#pragma once
#include "puzzle_guide.hpp"
#include <cstdint>
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
// expected: the step the guide expects next. placed: one bit per placement
// the guide has passed. removed: one bit per placement the route itself took
// back out, or whose binding moved on to another nonzero value. Routes have
// at most 64 steps.
struct PuzzleProgress {unsigned expected=0;std::uint64_t placed=0,removed=0;};
// Done(step) reports whether a checked step currently reads as done.
// Cleared(step) reports whether its done binding reads empty (0), as a recess
// or pedestal does once its sphere is taken back out.
template<class Done,class Cleared> void advancePuzzleProgress(const PuzzleRoute& route,unsigned begin,unsigned end,PuzzleProgress& progress,Done&& done,Cleared&& cleared) {
    const auto& steps=route.steps;
    const auto bit=[](unsigned i) {return std::uint64_t{1}<<i;};
    const auto passed=[&](unsigned i) {return (progress.removed&bit(i))!=0 || done(steps[i]);};
    const auto placedNow=[&]() {std::uint64_t mask=0;for(unsigned i=begin;i<progress.expected && i<end && i<64;++i) if(puzzleStepPlaced(route,i) && passed(i)) mask|=bit(i);return mask;};
    const auto latestCheckpoint=[&]() {int latest=-1;for(unsigned i=begin;i+1<end;++i) if(steps[i].checkpoint && passed(i)) latest=static_cast<int>(i);return latest;};
    // A placement the guide passed no longer reads as done: the player took
    // that sphere back out. Move the expected step back to the earliest such
    // placement; the checkpoint jump and forward scan below then run from
    // there. Exceptions:
    // - Some routes say to take a sphere back out (the Besaid door, the Kilika
    //   entry door, the Djose charging recesses). When a later carried-sphere
    //   step takes the sphere back out of the placement, and the guide has
    //   passed it or it reads as done now, the removal is that route step, so
    //   the placement stays passed for this trial context. Only steps up to the
    //   expected one count, unless a later checkpoint has passed: before that,
    //   an emptied recess is an undo even if a later step empties it too. A
    //   step takes the sphere back out when it targets the same worker as the
    //   placement (Kilika, Djose), or when it directly follows a checkpoint
    //   placement and carries the same sphere as the step before the placement
    //   (Besaid). A pickup of another sphere with the same carried value (the
    //   second Djose door sphere) is not a removal.
    // - A binding that moved to another nonzero value was not emptied: the
    //   Bevelle rail pedestal reads 1 on the entrance track and 19 once ridden.
    //   That placement stays passed too.
    // - A placement emptied behind the next passed checkpoint, before the
    //   guide has passed the step after that checkpoint, was used up by the
    //   route: the Djose door recesses clear as the door opens (the script may
    //   clear them just after it sets the door flag), and the Kilika
    //   antechamber recess clears as its sphere is removed. So is one the guide
    //   moved back to, once that checkpoint passes with nothing in hand and no
    //   later placement made.
    // - Later than that, a placement emptied behind a checkpoint counts as
    //   taken back out only while its sphere is in hand: the carried-sphere
    //   step before the placement reads as done again. Without a sphere in
    //   hand the recess may have been emptied by play the guide cannot see (a
    //   floor pattern resetting), so the placement stays passed.
    const auto checkpoint=latestCheckpoint();
    if(progress.placed) {
        const auto inHand=[&](unsigned i) {return i>begin && steps[i-1].check && !steps[i-1].checkpoint && steps[i-1].done.value!=0 &&
            !puzzleStepPlaced(route,i-1) && done(steps[i-1]);};
        unsigned back=progress.expected;
        for(unsigned i=begin;i<end && i<64;++i) {
            if(!(progress.placed&bit(i)) || passed(i)) continue;
            const auto sameBinding=[](const UtilityBinding& a,const UtilityBinding& b) {
                return a.low==b.low && a.high==b.high && a.value==b.value && a.element==b.element;};
            bool routeRemoval=false;
            for(unsigned next=i+1;next+1<end && (next<=progress.expected || checkpoint>static_cast<int>(i)) && !routeRemoval;++next) {
                const bool takesBack=steps[next].worker==steps[i].worker ||
                    (next==i+1 && steps[i].checkpoint && i>begin && steps[i-1].check && sameBinding(steps[i-1].done,steps[next].done));
                routeRemoval=takesBack && steps[next].check && !puzzleStepPlaced(route,next) && steps[next].done.value!=0 &&
                    (next<progress.expected || done(steps[next]));
            }
            if(!cleared(steps[i]) || routeRemoval) {progress.removed|=bit(i);continue;}
            if(checkpoint>static_cast<int>(i)) {
                unsigned next=i+1;while(next<end && !(steps[next].checkpoint && passed(next))) ++next;
                bool later=false;for(unsigned j=next+1;j<end && j<64;++j) if(progress.placed&bit(j)) later=true;
                if(progress.expected>i?progress.expected<=next+1:!later && !inHand(i)) {progress.removed|=bit(i);continue;}
                if(!inHand(i)) continue;
            }
            if(i<back) back=i;
        }
        progress.expected=back;
    }
    // A pickup undone: the step before the expected one carried a sphere, the
    // carried value now reads empty, and the player put the sphere back where
    // it came from (the Djose entry recesses), so ask for the pickup again.
    // Where it came from is a placement step at the same worker that stores
    // the same value; when it holds that value again, the sphere is back. That
    // also covers an expected step that only checks for an empty hand (the
    // Djose pedestal sockets), which putting the sphere back would otherwise
    // pass. Without that source step an empty hand proves nothing: the sphere
    // may have gone anywhere, so the guide stays put. Not for an unchecked
    // expected step, whose own action may have used the sphere.
    if(progress.expected>begin && progress.expected<end) {
        const auto before=progress.expected-1;
        const auto& pickup=steps[before];
        if(steps[progress.expected].check && pickup.check && !pickup.checkpoint && pickup.done.value!=0 &&
           !puzzleStepPlaced(route,before) && cleared(pickup)) {
            int source=-1;
            for(unsigned i=begin;i<end;++i) if(i!=before && steps[i].worker==pickup.worker && steps[i].done.value==pickup.done.value && puzzleStepPlaced(route,i)) {source=static_cast<int>(i);break;}
            if(source>=0 && done(steps[static_cast<unsigned>(source)])) progress.expected=before;
        }
    }
    // Checkpoints are durable native flags, so the guide may jump forward to
    // one, but not past a placement it is waiting to see redone. Otherwise it
    // only moves forward one done step at a time, so a carried sphere that is
    // placed later does not move it backwards.
    bool waiting=false;
    for(unsigned i=progress.expected;static_cast<int>(i)<checkpoint && i<64;++i) if((progress.placed&bit(i)) && !passed(i)) waiting=true;
    if(checkpoint>=0 && !waiting && static_cast<unsigned>(checkpoint)+1>progress.expected) progress.expected=static_cast<unsigned>(checkpoint)+1;
    while(progress.expected+1<end && passed(progress.expected)) ++progress.expected;
    // A move back keeps the later placements that still hold. Once the player
    // redoes the earlier step, the guide resumes after them.
    std::uint64_t ahead=0;for(unsigned i=progress.expected;i<end && i<64;++i) if(progress.placed&bit(i)) ahead|=bit(i);
    for(unsigned i=progress.expected;i<end && i<64;++i) {
        if(!(ahead&bit(i))) continue;
        if(!passed(i)) break;
        progress.expected=i+1;
        while(progress.expected+1<end && passed(progress.expected)) ++progress.expected;
    }
    // An emptied placement left passed above (nothing in hand) keeps its bit,
    // so taking its sphere out later still moves the guide back to it.
    std::uint64_t kept=0;for(unsigned i=begin;i<end && i<64;++i) if(progress.placed&bit(i) && (i>=progress.expected?(ahead&bit(i))!=0:!passed(i))) kept|=bit(i);
    progress.placed=placedNow()|kept;
}
}
