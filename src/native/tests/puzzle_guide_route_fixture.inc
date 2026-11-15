// Cloister route guide fixtures. Each walk replays the trial script variables
// in the order the original event scripts write them, then asks for the
// expected step (Shift+F11). A stage expecting 0 proves the guide reports an
// unidentified step instead of guessing past an unchecked action.
namespace {
struct GuideWrite {unsigned low;int value;unsigned element=0;unsigned high=1;};
struct GuideStage {const char* action;std::vector<GuideWrite> writes;int expected;};
const PuzzleRoute& guideRoute(const char* script) {
    for(const auto& route:puzzleRoutes) if(!std::strcmp(route.script,script)) return route;
    check(false,"guide route exists");std::abort();
}
void loadGuideRoute(const char* script,const std::vector<GuideStage>& stages) {
    const auto trial=std::find_if(std::begin(trialUtilities),std::end(trialUtilities),[&](const TrialUtility& t){return !std::strcmp(t.script,script);});
    check(trial!=std::end(trialUtilities),"guide trial exists");
    load(*trial,trial->collect);resetOwnedPuzzleGuide();
    for(const auto& step:guideRoute(script).steps) if(step.check) declaration(step.done.low,step.done.high);
    for(const auto& stage:stages) for(const auto& write:stage.writes) declaration(write.low,write.high);
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
    loadGuideRoute(script,stages);
    for(const auto& stage:stages) {
        for(const auto& write:stage.writes) setValue(write.low,write.high,write.value,write.element);
        expectGuide(script,stage.action,stage.expected);
    }
    resetOwnedPuzzleGuide();
}
void djoseGuideRouteFixture() {
    // djyt0600: 0x94 carried sphere (1 Djose, 3 charged, 10 Destruction);
    // 0x95/0x96 entry, 0x97/0x98 door, 0x99/0x9a charging, 0x9b charged
    // door (worker 54) and 0x9c left circuit (worker 55) recess sockets;
    // 0x9d/0x9e pedestal sockets (east face toward the charging wall, west
    // face toward the reset tile); 0x9f Destruction Sphere recess (worker 62);
    // 0x28e door opened; 0xa1 charger (1 powered, 2 sticky once the pedestal
    // sphere is charged); 0xd9 pedestal in gap, cleared by reset tile 67 only
    // while 0x9b is not 3; 0xda gap switch; 0xdb lift circuit; 0xe5 hidden
    // glyph (1 revealed, 2 touched); 0xde-0xe1 upper pedestals; 0xe3 all five
    // pushed (1) then sphere set (2). The entry state is the script's own
    // initial values, and every stage writes only what the original action
    // writes: no stage fakes a flag to reach the next step.
    walkGuideRoute("djyt0600",{
        {"entry",{{0x0c000095,1},{0x0c000096,1},{0x0c00009b,1},{0x0c00009c,1},{0x0c00009d,1},{0x0c00009f,10}},1},
        {"take entry sphere 1",{{0x0c000094,1},{0x0c000095,0}},2},
        {"insert door recess 1",{{0x0c000097,1},{0x0c000094,0}},3},
        {"take entry sphere 2",{{0x0c000094,1},{0x0c000096,0}},4},
        {"insert door recess 2 opens door",{{0x0c000098,1},{0x0c000094,0},{0x0000028e,1}},5},
        {"take charged-door recess sphere",{{0x0c000094,1},{0x0c00009b,0}},6},
        {"insert charging recess 1",{{0x0c000099,1},{0x0c000094,0}},7},
        {"take left circuit sphere",{{0x0c000094,1},{0x0c00009c,0}},8},
        {"insert charging recess 2 powers charger",{{0x0c00009a,1},{0x0c000094,0},{0x0c0000a1,1}},9},
        {"push loaded pedestal under charger",{{0x0c00009d,3},{0x0c0000a1,2}},0},
        {"reset tile returns pedestal",{},0},
        {"take charged sphere from east face",{{0x0c000094,3},{0x0c00009d,0}},0},
        {"insert charged door recess",{{0x0c00009b,3},{0x0c000094,0}},0},
        {"take charging recess 1",{{0x0c000094,1},{0x0c000099,0}},0},
        {"insert west pedestal socket",{{0x0c00009e,1},{0x0c000094,0}},0},
        {"take charging recess 2",{{0x0c000094,1},{0x0c00009a,0}},0},
        {"insert east pedestal socket",{{0x0c00009d,1},{0x0c000094,0}},0},
        {"push pedestal into gap",{{0x0c0000d9,1}},18},
        {"press gap switch",{{0x0c0000da,1}},19},
        {"take charged door sphere",{{0x0c000094,3},{0x0c00009b,0}},20},
        {"insert left circuit",{{0x0c00009c,3},{0x0c000094,0}},21},
        {"reset tile clears gap once door recess is empty",{{0x0c0000d9,0}},22},
        {"take east pedestal sphere",{{0x0c000094,1},{0x0c00009d,0}},23},
        {"insert entry recess 1",{{0x0c000095,1},{0x0c000094,0}},24},
        {"take west pedestal sphere",{{0x0c000094,1},{0x0c00009e,0}},25},
        {"insert entry recess 2 completes lift circuit",{{0x0c000096,1},{0x0c000094,0},{0x0c0000db,1},{0x0c0000e5,1}},26},
        {"touch hidden glyph",{{0x0c0000e5,2}},27},
        {"take Destruction Sphere",{{0x0c000094,10},{0x0c00009f,0}},0},
        {"push upper pedestals 1 to 4",{{0x0c0000de,1},{0x0c0000df,1},{0x0c0000e0,1},{0x0c0000e1,1}},0},
        {"push upper pedestal 5",{{0x0c0000e3,1}},34},
        {"insert Destruction Sphere",{{0x0c000094,0},{0x0c0000e3,2}},35},
        {"open Destruction treasure",{{0x01000a70,4}},36},
    });
    // Durable checkpoints recover the route when the guide starts late.
    walkGuideRoute("djyt0600",{{"late start after hidden glyph",{{0x0000028e,1},{0x0c000095,1},{0x0c000096,1},{0x0c00009c,3},{0x0c00009f,10},
        {0x0c0000a1,2},{0x0c0000da,1},{0x0c0000db,1},{0x0c0000e5,2}},27}});
    walkGuideRoute("djyt0600",{{"late start after door",{{0x0000028e,1},{0x0c00009c,1},{0x0c00009d,1},{0x0c00009f,10},{0x0c000094,1}},6}});
    // Removing a sphere from a checked socket never moves the guide backward.
    walkGuideRoute("djyt0600",{
        {"insert door recesses",{{0x0c000097,1},{0x0c000098,1},{0x0000028e,1}},5},
        {"take a door sphere back",{{0x0c000097,0},{0x0c000094,1}},6},
    });
}
void kilikaGuideRouteFixture() {
    // klyt1000: 0x5b carried sphere (1-3 Kilika, 4 Glyph, 6 Destruction);
    // 0x59/0x5a/0x5c/0x5d/0x5e/0x5f/0x61/0x62 recess sockets; 0x5d becomes 5
    // when the wall glyph is exposed and 0 when it is touched; 0x64 pedestal
    // on the floor depression; 0x63 reaches 3 when the antechamber door
    // sphere is removed.
    walkGuideRoute("klyt1000",{
        {"entry",{{0x0c000058,1},{0x0c00005a,4},{0x0c00005f,2},{0x0c000060,6},{0x0c000061,3}},1},
        {"take entry sphere",{{0x0c00005b,1},{0x0c000058,0}},2},
        {"insert entry door recess",{{0x0c000059,1},{0x0c00005b,0}},3},
        {"remove entry door sphere",{{0x0c00005b,1},{0x0c000059,0},{0x0c000063,1}},4},
        {"insert wall glyph recess",{{0x0c00005d,1},{0x0c00005b,0}},5},
        {"remove wall glyph sphere",{{0x0c00005b,1},{0x0c00005d,5}},6},
        {"store in storage recess 2",{{0x0c00005e,1},{0x0c00005b,0}},7},
        {"touch exposed glyph",{{0x0c00005d,0}},8},
        {"take Glyph Sphere",{{0x0c00005b,4},{0x0c00005a,0}},9},
        {"store Glyph in storage recess 1",{{0x0c00005c,4},{0x0c00005b,0}},10},
        {"take fire chamber sphere",{{0x0c00005b,2},{0x0c00005f,0}},11},
        {"insert pedestal",{{0x0c00005a,2},{0x0c00005b,0}},12},
        {"retrieve Glyph Sphere",{{0x0c00005b,4},{0x0c00005c,0}},13},
        {"insert Glyph in fire chamber recess",{{0x0c00005f,4},{0x0c00005b,0}},0},
        {"push pedestal onto depression",{{0x0c000064,1}},16},
        {"take lower recess sphere",{{0x0c00005b,3},{0x0c000061,0}},17},
        {"insert antechamber door recess",{{0x0c000062,3},{0x0c00005b,0}},18},
        {"take Destruction Sphere",{{0x0c00005b,6},{0x0c000060,0}},19},
        {"insert Destruction in lower recess",{{0x0c000061,6},{0x0c00005b,0}},20},
        {"open Destruction treasure",{{0x01000a70,2}},21},
        {"remove antechamber door sphere",{{0x0c000062,0},{0x0c00005b,3},{0x0c000063,3}},22},
    });
    walkGuideRoute("klyt1000",{{"late start on depression",{{0x0c000064,1}},16}});
    walkGuideRoute("klyt1000",{{"late start after treasure",{{0x0c000064,1},{0x01000a70,2},{0x0c000063,3}},22}});
    // Taking a sphere back out of its socket never moves the guide backward.
    walkGuideRoute("klyt1000",{
        {"take and insert entry sphere",{{0x0c00005b,1}},2},
        {"insert entry door recess",{{0x0c000059,1},{0x0c00005b,0}},3},
        {"remove entry door sphere",{{0x0c000059,0},{0x0c00005b,1}},4},
    });
}
void macalaniaGuideRouteFixture() {
    // mcyt0500: 0x94 carried sphere (4 Macalania, 10 Destruction, 11 Glyph);
    // sockets 0x95 upper, 0x96 ramp, 0x97 pedestal, 0x98 central, 0x99 lower
    // treasure, 0x9a lower reveal, 0x9b revealed sphere, 0x9c lower bridge
    // support, 0x9d hidden storage swapped with 0x97 by the walkway ramp;
    // 0xa3 pedestal place (0 main floor, 1 ice mound, 3 lower floor, 4 bridge
    // support, 5 walkway, 6 down walkway ramp); one-shot 0xc8 reveal, 0xca
    // bridge restored, 0xcd Destruction in treasure recess.
    walkGuideRoute("mcyt0500",{
        {"entry",{{0x0c000095,4},{0x0c000096,4},{0x0c000098,11},{0x0c00009b,4},{0x0c00009d,10}},1},
        {"push into first ice mound",{{0x0c0000a3,1}},2},
        {"push down opened ramp",{{0x0c0000a3,3}},3},
        {"take central Glyph Sphere",{{0x0c000094,11},{0x0c000098,0}},4},
        {"insert lower reveal recess",{{0x0c00009a,11},{0x0c000094,0},{0x0c0000c8,1}},5},
        {"take revealed sphere",{{0x0c000094,4},{0x0c00009b,0}},6},
        {"insert pedestal",{{0x0c000097,4},{0x0c000094,0}},7},
        {"push onto lower bridge support",{{0x0c0000a3,4}},8},
        {"take upper sphere",{{0x0c000094,4},{0x0c000095,0}},9},
        {"insert lower bridge support",{{0x0c00009c,4},{0x0c000094,0}},10},
        {"take ramp sphere",{{0x0c000094,4},{0x0c000096,0}},11},
        {"insert central restores bridge",{{0x0c000098,4},{0x0c000094,0},{0x0c0000ca,1}},12},
        {"upper walkway reset tile",{{0x0c0000a3,5}},13},
        {"push down walkway ramp",{{0x0c0000a3,6},{0x0c000097,10},{0x0c00009d,4}},14},
        {"main floor reset tile",{{0x0c0000a3,0}},15},
        {"take central sphere",{{0x0c000094,4},{0x0c000098,0}},16},
        {"insert upper recess",{{0x0c000095,4},{0x0c000094,0}},17},
        {"push into restored ice mound",{{0x0c0000a3,1}},18},
        {"take upper sphere back",{{0x0c000094,4},{0x0c000095,0}},19},
        {"insert ramp recess",{{0x0c000096,4},{0x0c000094,0}},20},
        {"push down restored ramp",{{0x0c0000a3,3}},21},
        {"take Destruction Sphere",{{0x0c000094,10},{0x0c000097,0}},22},
        {"insert lower treasure recess",{{0x0c000099,10},{0x0c000094,0},{0x0c0000cd,1}},23},
        {"open Destruction treasure",{{0x01000a70,8}},24},
        {"take Destruction back",{{0x0c000094,10},{0x0c000099,0}},25},
        {"put Destruction in pedestal",{{0x0c000097,10},{0x0c000094,0}},26},
        {"upper walkway reset tile again",{{0x0c0000a3,5}},27},
        {"push down walkway ramp again",{{0x0c0000a3,6},{0x0c000097,4},{0x0c00009d,10}},28},
        {"main floor reset tile again",{{0x0c0000a3,0}},29},
        {"take converted sphere",{{0x0c000094,4},{0x0c000097,0}},30},
        {"insert upper recess again",{{0x0c000095,4},{0x0c000094,0}},31},
        {"push empty pedestal into mound",{{0x0c0000a3,1}},32},
        {"push empty pedestal down ramp",{{0x0c0000a3,3}},33},
        {"take upper sphere back again",{{0x0c000094,4},{0x0c000095,0}},34},
        {"insert pedestal again",{{0x0c000097,4},{0x0c000094,0}},35},
        {"push onto bridge support again",{{0x0c0000a3,4}},36},
        {"take ramp sphere again",{{0x0c000094,4},{0x0c000096,0}},37},
        {"insert central final bridge",{{0x0c000098,4},{0x0c000094,0}},38},
    });
    walkGuideRoute("mcyt0500",{{"late start after bridge",{{0x0c0000c8,1},{0x0c0000ca,1}},12}});
    walkGuideRoute("mcyt0500",{{"late start after treasure",{{0x0c0000c8,1},{0x0c0000ca,1},{0x0c0000cd,1},{0x01000a70,8}},24}});
    // Taking the bridge sphere back keeps the durable bridge checkpoint.
    walkGuideRoute("mcyt0500",{
        {"bridge restored",{{0x0c0000c8,1},{0x0c0000ca,1},{0x0c000098,4}},12},
        {"take central sphere back",{{0x0c000098,0},{0x0c000094,4}},12},
    });
}
// bvyt1100 keeps sphere locations in the 72-element array 0x0d000032: element
// 12*sphere+place is 1 while that sphere is at that place. Spheres 1-3 are
// Bevelle Spheres, 4 Glyph, 5 Destruction. Place 1 is carried; 4 w30, 5 w31,
// 6 w32, 7 w33, 8 w34, 9 w36, 10 w37, 11 w35, 2 the rail pedestal.
GuideWrite bevelleSphere(unsigned sphere,unsigned place,int value) {return {0x0d000032,value,12*sphere+place,0xc0048};}
// The solver contract binds each recess worker to its Destruction Sphere
// element 12*5+place, so it names the worker that owns each place.
unsigned bevellePlaceWorker(unsigned place) {
    for(const auto& contract:puzzleContracts) if(!std::strcmp(contract.script,"bvyt1100"))
        for(const auto& recess:contract.interactions) if(recess.sphere.element==60+place) return recess.worker;
    check(false,"Bevelle recess place has a worker");return 0;
}
void bevelleGuideRouteFixture() {
    // The Glyph Sphere starts in place 7, and w33 writes 0x26 when the
    // Destruction Sphere goes in. The lower Bevelle Sphere goes into the empty
    // place 8 (element 44).
    const auto& bevelle=guideRoute("bvyt1100");
    check(bevelle.steps[4].worker==bevellePlaceWorker(8),"Bevelle lower power recess step targets place 8");
    check(bevelle.steps[5].worker==bevellePlaceWorker(7),"Bevelle Glyph Sphere source step targets place 7");
    check(bevelle.steps[8].worker==bevellePlaceWorker(7),"Bevelle Destruction Sphere destination step targets place 7");
    check(bevelle.steps[9].worker==bevellePlaceWorker(8),"Bevelle lower power recess retrieval step targets place 8");
    // 0x08 rail pedestal node (1 = pushed onto the entrance track); 0x25
    // Bevelle Sphere in w31; 0x26 Destruction Sphere in w33; 0x29 sphere in
    // w37; 0x2a second chest opened.
    walkGuideRoute("bvyt1100",{
        {"entry",{bevelleSphere(1,2,1),bevelleSphere(2,4,1),bevelleSphere(3,6,1),bevelleSphere(4,7,1),bevelleSphere(5,9,1)},1},
        {"push rail pedestal onto track",{{0x0c000008,1}},2},
        {"ride the rail",{{0x0c000008,19}},2},
        {"take upper Bevelle Sphere",{bevelleSphere(2,4,0),bevelleSphere(2,1,1)},3},
        {"insert upper rail power recess",{bevelleSphere(2,1,0),bevelleSphere(2,5,1),{0x0c000025,1}},4},
        {"take lower Bevelle Sphere",{bevelleSphere(3,6,0),bevelleSphere(3,1,1)},5},
        {"insert lower rail power recess",{bevelleSphere(3,1,0),bevelleSphere(3,8,1)},6},
        {"take Glyph Sphere",{bevelleSphere(4,7,0),bevelleSphere(4,1,1)},7},
        {"insert Glyph barrier recess",{bevelleSphere(4,1,0),bevelleSphere(4,11,1)},8},
        {"take Destruction Sphere",{bevelleSphere(5,9,0),bevelleSphere(5,1,1)},9},
        {"insert Destruction Sphere in former Glyph recess",{bevelleSphere(5,1,0),bevelleSphere(5,7,1),{0x0c000026,1}},10},
        {"retrieve lower power recess sphere",{bevelleSphere(3,8,0),bevelleSphere(3,1,1)},11},
        {"open Destruction treasure",{{0x01000a70,16}},12},
        {"insert upper platform recess",{bevelleSphere(3,1,0),bevelleSphere(3,10,1),{0x0c000029,1}},13},
        // w53 is the push zone for the emptied pedestal; it alone writes
        // 0x09=15. w15 then rides to 0x08=66, gives the second chest (w17
        // entry 7) and sets 0x2a, and only then rests the pedestal at 0x09=17.
        {"push pedestal to upper platform",{{0x0c000009,15}},14},
        {"ride reaches the second chest",{{0x0c000008,66}},14},
        {"ride opens second chest",{{0x0c00002a,1},{0x0c000009,17}},15},
    });
    walkGuideRoute("bvyt1100",{{"late start after Glyph placed",{{0x0c000025,1},bevelleSphere(4,11,1)},8}});
    walkGuideRoute("bvyt1100",{{"late start after Destruction placed",{{0x0c000025,1},bevelleSphere(4,11,1),{0x0c000026,1}},10}});
    // Taking a Bevelle Sphere back out of w31 clears 0x25 (w31 writes
    // 0x25=0 at 1685a, 169b0 and 16b06); putting it back sets it again.
    // After the put-back the guide must expect step 4 again.
    walkGuideRoute("bvyt1100",{
        {"insert upper rail power recess",{bevelleSphere(2,1,0),bevelleSphere(2,5,1),{0x0c000025,1}},4},
        {"take it back and put it in again",{bevelleSphere(2,5,0),bevelleSphere(2,1,1),{0x0c000025,0},
            bevelleSphere(2,1,0),bevelleSphere(2,5,1),{0x0c000025,1}},4},
    });
}
void zanarkandGuideRouteFixture() {
    // dome0700: 0x5c000004 pattern stage (0 floor, 1-4 pedestals, 5 return);
    // 0x22 floor pattern counter (6 = complete); 0x20 completed patterns and
    // correct sphere placements; 0x29 return floor counter (8 = complete);
    // 0x5c000008 carried sphere (1 Kilika, 2 Besaid, 3 Destruction);
    // sockets 0x5c000010 Besaid source, 0x5c00000c Kilika source,
    // 0x5c000018 Besaid recess, 0x5c00001c Kilika recess.
    const std::vector<GuideWrite> firstVisit={{0x20000a00,2768},{0x5c000004,0},{0x0c000020,0},{0x0c000021,0},{0x0c000022,0},{0x0c000023,0},
        {0x0c000024,0},{0x0c000025,0},{0x0c000027,0},{0x0c000029,0},{0x5c000008,0},{0x5c000010,2},{0x5c00000c,1},{0x5c000018,0},{0x5c00001c,0}};
    walkGuideRoute("dome0700",{
        {"entry",firstVisit,0},
        {"step on activation tile",{{0x0c000022,0},{0x0c000023,0}},0},
        {"partial floor pattern",{{0x0c000022,3}},0},
        {"complete floor pattern",{{0x0c000022,6},{0x0c000023,0}},3},
        {"activate pedestal 1",{{0x5c000004,1},{0x0c000021,1}},3},
        {"complete pedestal 1 pattern",{{0x0c000024,9},{0x0c000020,1}},4},
        {"reset floor counter by stepping a pattern-0 panel twice",{{0x0c000022,0}},4},
        {"complete pedestal 2 pattern",{{0x5c000004,2},{0x0c000021,2},{0x0c000024,6},{0x0c000020,2}},5},
        {"complete pedestal 3 pattern",{{0x5c000004,3},{0x0c000021,3},{0x0c000024,7},{0x0c000020,3}},6},
        {"complete pedestal 4 pattern",{{0x5c000004,4},{0x0c000021,4},{0x0c000024,7},{0x0c000020,4}},7},
        {"take Kilika Sphere out of order",{{0x5c00000c,0},{0x5c000008,1}},7},
        {"return Kilika Sphere",{{0x5c00000c,1},{0x5c000008,0}},7},
        {"take Besaid Sphere",{{0x5c000010,0},{0x5c000008,2}},8},
        {"insert Besaid recess",{{0x5c000008,0},{0x5c000018,2},{0x0c000020,5}},9},
        {"take Kilika Sphere",{{0x5c00000c,0},{0x5c000008,1}},10},
        {"insert Kilika recess",{{0x5c000008,0},{0x5c00001c,1},{0x0c000020,6}},11},
        {"gate opens",{{0x0c000020,7}},11},
    });
    auto lateStart=firstVisit;lateStart.push_back({0x0c000020,3});
    walkGuideRoute("dome0700",{{"late start after three pedestals",lateStart,6}});
    lateStart=firstVisit;lateStart.insert(lateStart.end(),{{0x0c000020,5},{0x5c000010,0},{0x5c000018,2}});
    walkGuideRoute("dome0700",{{"late start after Besaid recess",lateStart,9}});
    const std::vector<GuideWrite> returnVisit={{0x20000a00,2875},{0x5c000004,5},{0x0c000020,7},{0x0c000022,0},{0x0c000023,0},{0x0c000024,0},
        {0x0c000025,0},{0x0c000027,0},{0x0c000029,0},{0x5c000008,0},{0x5c000014,3},{0x5c00000c,0},{0x5c000018,0},{0x5c00001c,0}};
    walkGuideRoute("dome0700",{
        {"return entry",returnVisit,1},
        {"partial return floor pattern",{{0x0c000029,4}},1},
        {"complete return floor pattern",{{0x0c000029,8}},2},
        {"take Destruction Sphere",{{0x5c000014,0},{0x5c000008,3}},3},
        {"insert Destruction Sphere in Kilika source recess",{{0x5c000008,0},{0x5c00000c,3}},4},
        {"open Destruction treasure",{{0x01000a70,32}},4},
    });
    auto returnLate=returnVisit;returnLate.insert(returnLate.end(),{{0x5c000014,0},{0x5c00000c,3}});
    walkGuideRoute("dome0700",{{"late return start after Destruction placed",returnLate,4}});
}
}
void puzzleGuideRouteFixtures() {
    djoseGuideRouteFixture();
    kilikaGuideRouteFixture();
    macalaniaGuideRouteFixture();
    bevelleGuideRouteFixture();
    zanarkandGuideRouteFixture();
}
