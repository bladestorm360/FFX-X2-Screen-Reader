#include "navigation_door_followups.inc"
namespace {
unsigned lastArea=999;
const char* mapFixtureName="Besaid Village";
uintptr_t __cdecl mapTable(unsigned) { return address(0x2314000); }
uintptr_t __cdecl mapName(unsigned index,unsigned) { lastArea=index; return encoded(0x2314010,mapFixtureName); }
void __cdecl mapDecode(uintptr_t,char* buffer) { strcpy_s(buffer,256,mapFixtureName); }
int __cdecl arrivalIndex(uintptr_t chunk,unsigned entrance) {
    check(chunk==address(0x2317000),"arrival uses the primary completed field chunk");
    return entrance==1?35:34;
}
void mapControlsFixtures() {
    doorFixtures();
    stub(0x4b7f10,reinterpret_cast<void*>(mapTable));
    stub(0x46c3c0,reinterpret_cast<void*>(mapName));
    stub(0x2460e0,reinterpret_cast<void*>(mapDecode));
    set<uint16_t>(0x2314000,36*4); set<uint16_t>(0xd2ca90,17);
    keys['M'](false); expect(L"Besaid Village","M reads the current room's localized native area name");
    check(lastArea==35,"room IDs are translated into verified area indexes");
    set<uint16_t>(0x2314000,35*4); keys['M'](false);
    expect(L"No native map name is available.","map getter respects native catalog bounds");
    set<uint16_t>(0xd2ca90,0); lastArea=999; keys['M'](false);
    expect(L"No native map name is available.","ambiguous room has no invented map label");
    check(lastArea==999,"unknown room never calls a guessed map index");
    const auto previousController=read<uintptr_t>(address(0xf26ae8));
    const auto previousActors=read<uintptr_t>(address(0x1fc44e4)),previousControlled=read<uintptr_t>(address(0xf00740));
    const auto previousCapacity=read<unsigned>(address(0x1fc44e0));
    set<uintptr_t>(0xf26ae8,address(0x2316000));
    set<uintptr_t>(0x2316000+0x1f8,0); set<uintptr_t>(0x2316000+0x48,address(0x2317000));
    set<unsigned>(0x2317000+4,0x80); set<unsigned>(0x2317000+8,0xc0);
    set<uint16_t>(0x2317000+0x1e,35); set<uint16_t>(0xd2ca90+0xe,35);
    fixturePhysicalPart(0x2317000,0x1000);
    stub(0x46bd20,reinterpret_cast<void*>(arrivalIndex));
    set<uint16_t>(0x2314000,36*4); set<uint16_t>(0xd2ca90,17);
    set<uint8_t>(0xd2ca90+0xc,1); set<uint8_t>(0xd2a8e0,0);
    mapFixtureName="Guadosalam - Shop";
    Event arrival; arrival.args[0]=17;
    set<uint8_t>(0x2316000,2); set<uint16_t>(0x2316000+0xc,1);
    set<uintptr_t>(0x1fc44e4,address(0x2318000)); set<unsigned>(0x1fc44e0,1);
    set<uintptr_t>(0xf00740,0);
    auto count=said.size(); announceMapArrival(arrival);
    check(said.size()==count && !activeField(),"title with saved room/map and control bit but no player remains silent");
    set<uintptr_t>(0xf00740,address(0x2318000)); set<uint16_t>(0x2318000+2,1);
    set<float>(0x2318000+0xc,0); set<float>(0x2318000+0x10,0); set<float>(0x2318000+0x14,0);
    set<unsigned>(0x2318000+0x824,0); event(0x471a50,0);
    check(said.size()==count+1 && !said.back().second,"arrival queues after existing speech without interrupting it");
    expect(L"Entering Guadosalam - Shop","completed field arrival reads the native subarea name");
    check(lastArea==35,"arrival uses the current entrance rather than a requested room label");
    event(0x471a50,0); finishMapArrival();
    check(said.size()==count+1,"activation consumes one completed transition without suppressing later arrivals");
    announceMapArrival(arrival);
    drainSpeechDomainsForTest();
    check(said.size()==count+2,"another completed arrival with identical text is still announced");
    count=said.size();announceMapArrival(arrival);announceMapArrival(arrival);
    encoded(0x2314010,"Changed source");drainSpeechDomainsForTest();
    check(said.size()==count+2 && said[count].first==L"Entering Guadosalam - Shop" && said[count+1].first==said[count].first,
        "two accepted equal arrivals retain copied names before worker preparation");
    count=said.size();announceMapArrival(arrival);resetNavigationScripts();drainSpeechDomainsForTest();
    check(said.size()==count,"same-epoch teardown cancels an already queued arrival");
    set<uint16_t>(0x2317000+0x1e,0x8001);set<unsigned>(0x2317000+0x28,0x2000);
    set<uint16_t>(0x2317000+0x2000,35);lastArea=999;
    announceMapArrival(arrival);drainSpeechDomainsForTest();
    check(said.size()==count && lastArea==999,"readable memory outside the exact physical part cannot supply a guessed area table");
    resetNavigationScripts();set<uint16_t>(0x2317000+0x1e,35);set<unsigned>(0x2317000+8,0x1020);
    announceMapArrival(arrival);drainSpeechDomainsForTest();
    check(said.size()==count && !activeField(),"entry table outside the exact physical part cannot activate a field or arrival");
    resetNavigationScripts();set<unsigned>(0x2317000+8,0xc0);
    count=said.size(); arrival.args[0]=18; announceMapArrival(arrival);
    check(said.size()==count,"uncompleted or superseded room request is silent");
    arrival.args[0]=17; set<uint8_t>(0x2316000,0); announceMapArrival(arrival); finishMapArrival();
    check(said.size()==count,"unactivated field has no arrival announcement");
    resetNavigationScripts(); set<uint8_t>(0x2316000,2); event(0x471a50,0);
    check(said.size()==count,"map teardown cancels pending activation");
    set<uintptr_t>(0xf00740,address(0x2318000)+1); announceMapArrival(arrival);
    check(said.size()==count && !activeField(),"misaligned controlled actor is not ActiveField");
    resetNavigationScripts(); set<uintptr_t>(0xf00740,address(0x2318000));
    set<unsigned>(0x2318000+0x824,0xcdcdcdcd); announceMapArrival(arrival);
    check(said.size()==count,"uninitialized player is not a loaded field");
    resetNavigationScripts(); set<unsigned>(0x2318000+0x824,0);
    set<float>(0x2318000+0x10,std::numeric_limits<float>::quiet_NaN()); announceMapArrival(arrival);
    check(said.size()==count,"nonfinite player coordinate cannot announce arrival");
    resetNavigationScripts(); set<float>(0x2318000+0x10,0);
    arrival.args[0]=17; set<uint8_t>(0xd2a8e0,1); announceMapArrival(arrival);
    check(said.size()==count,"battle cannot be announced as a map arrival");
    set<uint8_t>(0xd2a8e0,0); set<uint16_t>(0xd2ca90,23); arrival.args[0]=23; announceMapArrival(arrival);
    check(said.size()==count,"title/save-load scene cannot announce a stale field name");
    set<uintptr_t>(0xf26ae8,0); set<uint16_t>(0xd2ca90,0); arrival.args[0]=0; announceMapArrival(arrival);
    check(said.size()==count,"unavailable native destination label produces no invented announcement");
    set<uintptr_t>(0xf26ae8,previousController);
    set<uintptr_t>(0x1fc44e4,previousActors); set<uintptr_t>(0xf00740,previousControlled); set<unsigned>(0x1fc44e0,previousCapacity);
    mapFixtureName="Besaid Village";
    std::puts("PASS: map arrivals use native entrance names, retain repeated arrivals and exclude stale/title/battle events.");
}
}
