// Synthetic trial routes for the expected-step tracker. Needs no game data.
#include "../puzzle_guide_progress.hpp"
#include <cstdio>
#include <cstdlib>
#include <map>
using namespace ffx;
namespace {
int failures=0;
void check(bool valid,const char* reason) {if(!valid) {std::fprintf(stderr,"FAIL: %s\n",reason);++failures;}}
constexpr unsigned carry=0x100,door=0x101,hidden=0x102,touch=0x103,pedestal=0x104,treasure=0x105,recess=0x106,door2=0x107,rail=0x108;
PuzzleStep step(unsigned low,int value,bool checkpoint,unsigned worker) {return PuzzleStep{L"",worker,{},{low,1,value,0},true,false,checkpoint};}
// Same shape as the Besaid route: a sphere is carried, put in the door, taken
// back out, put in a hidden recess, then a second sphere goes in a pedestal.
// The door step is a checkpoint, the pedestal step is not. As at Besaid, the
// door and the recess the sphere is taken back from are different workers.
const PuzzleRoute route{"synthetic",{
    step(carry,2,false,1),      // 0 take the first sphere
    step(door,1,true,2),        // 1 insert it into the door
    step(carry,2,false,3),      // 2 take it back out of the door
    step(hidden,1,true,4),      // 3 insert it into the hidden recess
    step(touch,1,true,5),       // 4 touch the symbols
    step(carry,1,false,6),      // 5 take the second sphere
    step(pedestal,1,false,7),   // 6 insert it into the pedestal
    step(carry,3,false,8),      // 7 take the third sphere
    step(treasure,1,true,9),    // 8 open the chest
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
// The end of the Besaid route: the Besaid Sphere stays in the pedestal, and
// the Destruction Sphere goes into the recess the Besaid Sphere came from.
const PuzzleRoute tail{"synthetic tail",{
    step(touch,1,true,10),      // 0 touch the symbols
    step(carry,1,false,11),     // 1 take the Besaid Sphere
    step(pedestal,1,false,12),  // 2 insert it into the pedestal
    step(carry,3,false,13),     // 3 take the Destruction Sphere
    step(recess,1,true,14),     // 4 insert it into the empty recess
    step(treasure,1,true,15),   // 5 open the chest
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
// The Kilika opening: the route puts a sphere in the entry door recess, which
// is not a checkpoint, then takes it back out of that same recess.
const PuzzleRoute kilika{"synthetic kilika",{
    step(carry,1,false,28),     // 0 take the entry sphere
    step(door,1,false,29),      // 1 insert it into the entry door recess
    step(carry,1,false,29),     // 2 remove it again from the same recess
    step(hidden,1,false,31),    // 3 insert it into the wall recess
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
// The Djose opening: two spheres go from the entry recesses into the two door
// recesses. The first door recess is not a checkpoint, and the next step picks
// up the second sphere with the same carried value. That pickup is not the
// route taking the first sphere back out.
const PuzzleRoute djose{"synthetic djose",{
    step(carry,1,false,48),     // 0 take the sphere from entry recess 1
    step(door,1,false,50),      // 1 insert it into door recess 1
    step(carry,1,false,49),     // 2 take the sphere from entry recess 2
    step(door2,1,true,51),      // 3 insert it into door recess 2, door opens
    step(carry,1,false,1),      // 4 take the pedestal sphere
    step(recess,1,false,52),    // 5 insert it into a charging recess
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
// The Bevelle opening: the rail pedestal node reads 1 once pushed onto the
// entrance track and 19 once ridden. Riding moves it on; nothing is taken back.
const PuzzleRoute bevelle{"synthetic bevelle",{
    step(rail,1,false,13),      // 0 push the rail pedestal onto the track
    step(carry,2,false,22),     // 1 take the upper sphere
    step(recess,1,false,23),    // 2 insert it into the power recess
    step(carry,3,false,24),     // 3 take the lower sphere
    PuzzleStep{L"",0,{},{0,1,0,0},false,true,false},
}};
std::map<unsigned,int> flags;
unsigned expected(PuzzleProgress& progress,const PuzzleRoute& r=route) {
    advancePuzzleProgress(r,0,static_cast<unsigned>(r.steps.size()),progress,
        [](const PuzzleStep& s) {return s.check && flags[s.done.low]==s.done.value;},
        [](const PuzzleStep& s) {return flags[s.done.low]==0;});
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
    // The sphere was put down somewhere other than the pedestal, so the pickup
    // is undone.
    flags[carry]=0; check(expected(p)==5,"putting the carried sphere down elsewhere expects its pickup again");
    flags[carry]=1; flags[pedestal]=0; check(expected(p)==6,"picking it up again expects the pedestal");
    flags[carry]=0; flags[pedestal]=1; check(expected(p)==7,"reinserting it resumes after the pedestal");
    flags[carry]=3; check(expected(p)==8,"carried pickup still advances sequentially");
    flags[carry]=0; check(expected(p)==7,"putting the third sphere down before the chest expects its pickup again");
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
    {
        // Kilika: the route's own removal from the same recess keeps progress.
        PuzzleProgress k; flags={{carry,1}};
        check(expected(k,kilika)==1,"kilika: carrying the entry sphere expects the door recess");
        flags[carry]=0; flags[door]=1; check(expected(k,kilika)==2,"kilika: door recess expects the sphere to come back out");
        flags[door]=0; flags[carry]=1; check(expected(k,kilika)==3,"kilika: removing it from the same recess is the route step");
        check(expected(k,kilika)==3,"kilika: the route's removal stays passed on later reads");
    }
    {
        // Djose: taking the sphere back out of door recess 1 is not a route
        // step, even though the next pickup now reads as done.
        PuzzleProgress d; flags={{carry,1}};
        check(expected(d,djose)==1,"djose: carrying the first sphere expects door recess 1");
        // Putting it back into entry recess 1 empties the carried value and
        // leaves door recess 1 empty: the pickup was undone.
        flags[carry]=0;
        const auto putBack=expected(d,djose);
        if(putBack!=0) std::fprintf(stderr,"djose: expected step index %u after putting the sphere back into entry recess 1, want 0\n",putBack);
        check(putBack==0,"djose: putting the first sphere back expects that pickup again");
        flags[carry]=1; check(expected(d,djose)==1,"djose: taking it again expects door recess 1");
        flags[carry]=0; flags[door]=1; check(expected(d,djose)==2,"djose: door recess 1 expects the second entry sphere");
        flags[door]=0; flags[carry]=1;
        const auto back=expected(d,djose);
        if(back!=1) std::fprintf(stderr,"djose: expected step index %u after taking the sphere back out of door recess 1, want 1\n",back);
        check(back==1,"djose: taking the sphere back out of door recess 1 expects door recess 1");
        flags[carry]=0; flags[door]=1; check(expected(d,djose)==2,"djose: reinserting it expects the second entry sphere");
        flags[carry]=1; check(expected(d,djose)==3,"djose: carrying the second sphere expects door recess 2");
        flags[carry]=0; flags[door2]=1; check(expected(d,djose)==4,"djose: door recess 2 expects the pedestal sphere");
    }
    {
        // Bevelle: a placement binding that moves to another nonzero value
        // keeps progress. Only an emptied binding moves the guide back.
        PuzzleProgress b; flags={};
        check(expected(b,bevelle)==0,"bevelle: entry expects the rail pedestal");
        flags[rail]=1; check(expected(b,bevelle)==1,"bevelle: pedestal on the track expects the upper sphere");
        flags[rail]=19;
        const auto ridden=expected(b,bevelle);
        if(ridden!=1) std::fprintf(stderr,"bevelle: expected step index %u after riding the rail, want 1\n",ridden);
        check(ridden==1,"bevelle: riding the rail does not move the guide back to the pedestal");
        flags[carry]=2; check(expected(b,bevelle)==2,"bevelle: carrying the upper sphere expects the power recess");
        flags[carry]=0; flags[recess]=1; check(expected(b,bevelle)==3,"bevelle: power recess expects the lower sphere");
        flags[recess]=0; flags[carry]=2; check(expected(b,bevelle)==2,"bevelle: emptying the power recess still moves the guide back");
    }
    if(failures) return 1;
    std::puts("PASS: puzzle guide expected step follows placements taken back out, keeps route removals.");
    return 0;
}
