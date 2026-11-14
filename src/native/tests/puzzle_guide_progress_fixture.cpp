// Synthetic trial routes for the expected-step tracker. Needs no game data.
#include "../puzzle_guide_progress.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace ffx;
namespace {
int failures=0;
void check(bool valid,const char* reason) {if(!valid) {std::fprintf(stderr,"FAIL: %s\n",reason);++failures;}}
constexpr unsigned carry=0x100,door=0x101,hidden=0x102,touch=0x103,pedestal=0x104,treasure=0x105,recess=0x106;
PuzzleStep step(unsigned low,int value,bool checkpoint) {return PuzzleStep{L"",0,{},{low,1,value,0},true,false,checkpoint};}
// Same shape as the Besaid route: a sphere is carried, put in the door, taken
// back out, put in a hidden recess, then a second sphere goes in a pedestal.
// The pedestal step is not a checkpoint.
const PuzzleRoute route{"synthetic",{
    step(carry,2,false),     // 0 take the first sphere
    step(door,1,true),       // 1 insert it into the door
    step(carry,2,false),     // 2 take it back out of the door
    step(hidden,1,true),     // 3 insert it into the hidden recess
    step(touch,1,true),      // 4 touch the symbols
    step(carry,1,false),     // 5 take the second sphere
    step(pedestal,1,false),  // 6 insert it into the pedestal
    step(carry,3,false),     // 7 take the third sphere
    step(treasure,1,true),   // 8 open the chest
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
// The end of the Besaid route: the Besaid Sphere stays in the pedestal, and
// the Destruction Sphere goes into the recess the Besaid Sphere came from.
const PuzzleRoute tail{"synthetic tail",{
    step(touch,1,true),      // 0 touch the symbols
    step(carry,1,false),     // 1 take the Besaid Sphere
    step(pedestal,1,false),  // 2 insert it into the pedestal
    step(carry,3,false),     // 3 take the Destruction Sphere
    step(recess,1,true),     // 4 insert it into the empty recess
    step(treasure,1,true),   // 5 open the chest
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
std::map<unsigned,int> flags;
unsigned expected(PuzzleProgress& progress,const PuzzleRoute& r=route) {
    advancePuzzleProgress(r,0,static_cast<unsigned>(r.steps.size()),progress,
        [](const PuzzleStep& s) {return s.check && flags[s.done.low]==s.done.value;});
    return progress.expected;
}
}
int main() {
    PuzzleProgress p;
    check(expected(p)==0,"fresh trial expects the first step");
    flags[carry]=2; check(expected(p)==1,"carrying the first sphere expects the door");
    flags[carry]=0; flags[door]=1; check(expected(p)==2,"door placement expects the sphere to come back out");
    // The route's own removal. Whether the door flag stays set is game data,
    // so both readings must keep the player moving forward.
    flags[carry]=2; check(expected(p)==3,"taking the sphere out of the door keeps progress");
    {
        PuzzleProgress q; std::map<unsigned,int> saved=flags; flags={{carry,0},{door,1}};
        check(expected(q)==2,"door placement expects removal");
        flags[door]=0; flags[carry]=2; check(expected(q)==3,"a door flag that clears on the route's removal does not send the guide back");
        check(expected(q)==3,"the route's removal stays passed on later reads");
        flags=saved;
    }
    flags[carry]=0; flags[hidden]=1; check(expected(p)==4,"hidden recess expects the symbols");
    flags[touch]=1; check(expected(p)==5,"symbols expect the second sphere");
    flags[carry]=1; check(expected(p)==6,"carrying the second sphere expects the pedestal");
    flags[carry]=0; flags[pedestal]=1; check(expected(p)==7,"pedestal placement expects the third sphere");
    // Taking the sphere back out of the pedestal is not a route step.
    flags[pedestal]=0; flags[carry]=1; check(expected(p)==6,"removing the pedestal sphere moves the guide back to the pedestal");
    flags[carry]=0; check(expected(p)==6,"a carried value alone never moves the guide back");
    flags[carry]=1; flags[pedestal]=0; check(expected(p)==6,"picking it up again expects the pedestal");
    flags[carry]=0; flags[pedestal]=1; check(expected(p)==7,"reinserting it resumes after the pedestal");
    flags[carry]=3; check(expected(p)==8,"carried pickup still advances sequentially");
    flags[carry]=0; check(expected(p)==8,"a carried value that changes later does not move the guide back");
    flags[treasure]=1; check(expected(p)==9,"treasure expects the final unchecked step");
    // Undoing an earlier placement while a later one holds is not tracked.
    flags[hidden]=0; check(expected(p)==9,"only the latest placement can move the guide back");
    {
        // Taking the Destruction Sphere back out may clear its recess flag. The
        // guide must ask for that recess again, not for the Besaid Sphere, which
        // is still in the pedestal.
        PuzzleProgress t; flags={{touch,1},{carry,1}};
        check(expected(t,tail)==2,"tail: carrying the Besaid Sphere expects the pedestal");
        flags[carry]=0; flags[pedestal]=1; check(expected(t,tail)==3,"tail: pedestal expects the Destruction Sphere");
        flags[carry]=3; check(expected(t,tail)==4,"tail: carrying the Destruction Sphere expects the recess");
        flags[carry]=0; flags[recess]=1; check(expected(t,tail)==5,"tail: recess expects the treasure");
        flags[recess]=0; flags[carry]=3; check(expected(t,tail)==4,"removing the Destruction Sphere expects its recess, not an earlier pickup");
    }
    if(failures) return 1;
    std::puts("PASS: puzzle guide expected step follows placements taken back out, keeps route removals.");
    return 0;
}
