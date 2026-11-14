// Cloister route guide fixtures. Each walk replays the trial script variables
// in the order the original event scripts write them, then asks for the
// expected step (Shift+F11). A stage expecting 0 proves the guide reports an
// unidentified step instead of guessing past an unchecked action.
namespace {
struct GuideWrite {unsigned low;int value;};
struct GuideStage {const char* action;std::vector<GuideWrite> writes;int expected;};
const PuzzleRoute& guideRoute(const char* script) {
    for(const auto& route:puzzleRoutes) if(!std::strcmp(route.script,script)) return route;
    check(false,"guide route exists");std::abort();
}
void loadGuideRoute(const char* script) {
    const auto trial=std::find_if(std::begin(trialUtilities),std::end(trialUtilities),[&](const TrialUtility& t){return !std::strcmp(t.script,script);});
    check(trial!=std::end(trialUtilities),"guide trial exists");
    load(*trial,trial->collect);resetOwnedPuzzleGuide();
    for(const auto& step:guideRoute(script).steps) if(step.check) declaration(step.done.low,step.done.high);
    put(chunk,0x402,static_cast<uint16_t>(declarations.size()));
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
}
void expectGuide(const char* script,const char* action,int expected) {
    const auto text=fixturePuzzleGuideSync(OwnedGuideAction::Expected);
    const bool ok=expected?text.find(L"Step "+std::to_wstring(expected)+L" of")!=std::wstring::npos:
        text.find(L"does not identify an exact puzzle step")!=std::wstring::npos;
    if(!ok) std::fprintf(stderr,"%s after %s expected step %d: %ls\n",script,action,expected,text.c_str());
    check(ok,"guide expected step follows native trial state");
}
void walkGuideRoute(const char* script,const std::vector<GuideStage>& stages) {
    loadGuideRoute(script);
    for(const auto& stage:stages) {
        for(const auto& write:stage.writes) setValue(write.low,1,write.value);
        expectGuide(script,stage.action,stage.expected);
    }
    resetOwnedPuzzleGuide();
}
void djoseGuideRouteFixture() {
    // djyt0600: 0x94 carried sphere (1 Djose, 3 charged, 10 Destruction);
    // 0x95/0x97/0x99/0x9a/0x9b/0x9c recess sockets; 0x28e door opened;
    // 0xa1 charger (2 = pedestal sphere charged); 0xd9 pedestal in gap;
    // 0xda gap switch; 0xdb lift circuit; 0xe5 hidden glyph (2 touched);
    // 0xde-0xe1 upper pedestals; 0xe3 all five pushed (1) then sphere set (2).
    walkGuideRoute("djyt0600",{
        {"entry",{{0x0c000095,1},{0x0c00009b,1},{0x0c00009c,1}},1},
        {"take entry sphere 1",{{0x0c000094,1},{0x0c000095,0}},2},
        {"insert door recess 1",{{0x0c000097,1},{0x0c000094,0}},3},
        {"take entry sphere 2",{{0x0c000094,1}},4},
        {"insert door recess 2 opens door",{{0x0c000094,0},{0x0000028e,1}},5},
        {"take pedestal sphere",{{0x0c000094,1}},6},
        {"insert charging recess 1",{{0x0c000099,1},{0x0c000094,0}},7},
        {"take charged-door recess sphere",{{0x0c000094,1},{0x0c00009b,0}},8},
        {"insert charging recess 2 powers charger",{{0x0c00009a,1},{0x0c000094,0},{0x0c0000a1,1}},0},
        {"take left circuit sphere",{{0x0c000094,1},{0x0c00009c,0}},0},
        {"insert pedestal under charger",{{0x0c000094,0},{0x0c0000a1,2}},12},
        {"take charged sphere",{{0x0c000094,3}},13},
        {"insert charged door recess",{{0x0c00009b,3},{0x0c000094,0}},0},
        {"take charging recess 1",{{0x0c000094,1},{0x0c000099,0}},0},
        {"insert pedestal socket",{{0x0c000094,0}},0},
        {"take charging recess 2",{{0x0c000094,1},{0x0c00009a,0}},0},
        {"insert other pedestal socket",{{0x0c000094,0}},0},
        {"push pedestal into gap",{{0x0c0000d9,1}},20},
        {"press gap switch",{{0x0c0000da,1}},21},
        {"reset glyph returns pedestal",{{0x0c0000d9,0}},22},
        {"take charged door sphere",{{0x0c000094,3},{0x0c00009b,0}},23},
        {"insert left circuit",{{0x0c00009c,3},{0x0c000094,0}},24},
        {"take pedestal sphere",{{0x0c000094,1}},25},
        {"insert entry recess 1",{{0x0c000095,1},{0x0c000094,0}},26},
        {"take remaining pedestal sphere",{{0x0c000094,1}},27},
        {"insert entry recess 2 completes lift circuit",{{0x0c000094,0},{0x0c0000db,1},{0x0c0000e5,1}},28},
        {"touch hidden glyph",{{0x0c0000e5,2}},29},
        {"take Destruction Sphere",{{0x0c000094,10}},0},
        {"push upper pedestals 1 to 4",{{0x0c0000de,1},{0x0c0000df,1},{0x0c0000e0,1},{0x0c0000e1,1}},0},
        {"push upper pedestal 5",{{0x0c0000e3,1}},36},
        {"insert Destruction Sphere",{{0x0c000094,0},{0x0c0000e3,2}},37},
        {"open Destruction treasure",{{0x01000a70,4}},38},
    });
    // Durable checkpoints recover the route when the guide starts late.
    walkGuideRoute("djyt0600",{{"late start after hidden glyph",{{0x0000028e,1},{0x0c0000a1,2},{0x0c0000da,1},{0x0c0000db,1},{0x0c0000e5,2}},29}});
    walkGuideRoute("djyt0600",{{"late start after door",{{0x0000028e,1},{0x0c000094,1}},6}});
    // Removing a sphere from a checked socket never moves the guide backward.
    walkGuideRoute("djyt0600",{
        {"insert door recesses",{{0x0c000097,1},{0x0000028e,1}},5},
        {"take a door sphere back",{{0x0c000097,0},{0x0c000094,1}},6},
    });
}
}
void puzzleGuideRouteFixtures() {
    djoseGuideRouteFixture();
}
