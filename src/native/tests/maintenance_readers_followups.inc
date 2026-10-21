#include "../game_visual_text.hpp"
namespace {
unsigned fixtureLocale=1,fixtureEnglish=0;
unsigned __cdecl uiLocale() { return fixtureLocale; }
unsigned __cdecl englishConfig() { return fixtureEnglish; }
uintptr_t __cdecl maintenanceMenu(int space,unsigned id,unsigned lang) {
    if(space==3 && id>=0x2000 && id<=0x2002) return address(0x2400000+(id-0x2000)*64);
    if(space==3 && (id==0x2004 || id==0x201f)) return address(0x24000c0);
    return menuLabels(space,id,lang);
}
uintptr_t __cdecl modeLabel(unsigned id) { return encoded(0x2400140,id==0x1033?"Stoic":"Warrior"); }
uintptr_t __cdecl destinationRecord(int row) { return address(0x2401000+row*64); }
uintptr_t __cdecl destinationLabel(unsigned state,uintptr_t record) { return state==1?record+32:encoded(0x2400200,"Unknown"); }
int rawNames=0;
uintptr_t __cdecl fingerprintName(unsigned index,unsigned) { ++rawNames; return address(index==0?0x2402000:0x2402100); }
void maintenanceMenus() {
    stub(0x241290,uiLocale); stub(0x385290,englishConfig);
    stub(0x38fd40,maintenanceMenu); stub(0x4ddfa0,modeLabel);
    stub(0x4b4df0,destinationRecord); stub(0x4b5510,destinationLabel);
    encoded(0x2400000,"Save menu"); encoded(0x2400040,"Save"); encoded(0x2400080,"Load"); encoded(0x24000c0,"Saved games");
    encoded(0x2400400,"Charges when damaged"); encoded(0x2400440,"Charges when attacking");
    constexpr unsigned root=0x2403000,popup=0x2403100,list=0x2403300;
    scene=15; set<int>(root+0x1c,9); set<unsigned>(0x1466224,0); set<uint8_t>(0x1466228,1); check(write<uint8_t>(address(0x1466228)+1,1),"option availability");
    event(0x4b1580,address(root),[=]{set<int>(root+0x1c,10);set<uintptr_t>(0x146621c,address(popup));
        set<uintptr_t>(popup+0x58,address(root));set<uintptr_t>(popup+0x10,address(0x4b3810));});
    expect(L"Save menu. Save","Save/load first displayed selection and title");
    auto n=said.size(); event(0x4b1580,address(root)); check(said.size()==n,"Save/load idle producer is silent");
    event(0x4b1580,address(root),[]{set<unsigned>(0x1466224,1);}); expect(L"Load","Save/load selection change");
    event(0x4b1580,address(root),[]{check(write<uint8_t>(address(0x1466228)+1,0),"option availability");}); expect(L"Load. Unavailable","native disabled option remains described");
    event(0x4b1580,address(root),[=]{set<uint8_t>(popup+0x41,1);});
    n=said.size(); event(0x4b1580,address(root),[]{set<unsigned>(0x1466224,0);}); check(said.size()==n,"closed Save/load pointer cannot speak stale selections");
    set<uintptr_t>(0x1466214,address(list)); set<uintptr_t>(list+0x10,address(0x4b4a00)); set<uint16_t>(list+0x30,2);
    set<uint8_t>(0x1466250,1); check(write<uint8_t>(address(0x1466250)+1,2),"destination state"); set<uint8_t>(0x146624a,0);
    encoded(0x2401020,"Besaid"); set<uint16_t>(0x2401018,35); stub(0x46c3c0,mapName); stub(0x4b7f10,mapTable);
    event(0x4b4460,address(list),[=]{set<int>(list+0x28,2);}); expect(L"Saved games. Besaid. Besaid Village","Save/load rendered name and second line");
    n=said.size(); event(0x4b4460,address(list)); check(said.size()==n,"destination idle silence");
    event(0x4b4460,address(list),[=]{set<int16_t>(list+0x48,1);}); expect(L"Unknown. Unavailable","undiscovered destination uses displayed placeholder only");
    set<uintptr_t>(0x1466214,0); n=said.size(); event(0x4b4460,address(list),[=]{set<int16_t>(list+0x48,0);});
    check(said.size()==n,"retired destination window cannot speak old names");
    // Overdrive names and help occupy independent native record fields.
    scene=21; setSetting(Setting::AutoDetail,0); constexpr unsigned mode=0x2403500;
    set<uintptr_t>(mode+0x10,address(0x4d16c0)); set<uint16_t>(mode+0x30,2); set<uint8_t>(mode+0x42,1);
    for(unsigned row=0;row<2;++row) {set<uint16_t>(0x1197730+row*4,static_cast<uint16_t>(row));set<uint8_t>(0x1197732+row*4,3);set<uint8_t>(0x1197733+row*4,1);}
    event(0x4b4460,address(mode),[=]{set<int>(mode+0x28,2);}); expect(L"Stoic","existing mode option name remains spoken");
    keys['I'](false); expect(L"Charges when damaged","I reads the independent rendered mode help");
    event(0x4b4460,address(mode),[=]{set<int16_t>(mode+0x48,1);}); keys['I'](false); expect(L"Charges when attacking","mode change refreshes manual help");
    setSetting(Setting::AutoDetail,1); n=said.size(); event(0x4b4460,address(mode),[=]{set<int16_t>(mode+0x48,0);});
    check(said.size()==n+2 && said[n].first==L"Stoic" && said.back().first==L"Charges when damaged","mode automatic help follows current headline");
    // The View renderer 8D17F0 withholds flag0 cells. A distinct blank cursor
    // event remains useful, but its hidden record name/help must stay private.
    event(0x4d0e80,address(mode),[]{set<uint8_t>(0x1197733,0);});expect(L"Empty","Overdrive View announces an actual blank cell");
    keys['I'](false);expect(L"No native description is available for this selection.","blank Overdrive clears mode help");
    n=said.size();event(0x4d0e80,address(mode),[=]{set<int16_t>(mode+0x48,1);set<uint8_t>(0x1197737,0);});
    check(said.size()==n+1 && said.back().first==L"Empty","different empty Overdrive cells are never text-deduplicated");
    setSetting(Setting::AutoDetail,0);
    event(0x4d0e80,address(mode),[]{set<uint16_t>(0x1197734,0x2002);set<uint8_t>(0x1197736,1);set<uint8_t>(0x1197737,1);});
    expect(L"Ether","displayed Overdrive View entry retains its native ability name");
    keys['I'](false);expect(L"Restores MP","displayed Overdrive View help remains available");
    setSetting(Setting::AutoDetail,0); resetSelectionDetail(); scene=0; stub(0x38fd40,menuLabels);
}
void nativePolishDetection() {
    stub(0x4b7f10,mapTable); stub(0x46c3c0,fingerprintName); set<uint16_t>(0x2314000,0);
    setSetting(Setting::PolishText,0); installTextEncoding(); rawNames=0;
    const uint8_t glyph[]={static_cast<uint8_t>(0x30+115),0};
    auto word=[&]{return first(reinterpret_cast<uintptr_t>(glyph),sizeof(glyph));};
    check(word()==L"\u00c0" && !rawNames,"unloaded native table uses stock and retains unknown detection");
    const uint8_t pl0[]={93,120,114,0}, pl44[]={81,116,130,112,120,115,70,58,90,123,112,130,137,131,126,129,58,95,129,201,113,0};
    memcpy(reinterpret_cast<void*>(address(0x2402000)),pl0,sizeof(pl0)); memcpy(reinterpret_cast<void*>(address(0x2402100)),pl44,sizeof(pl44));
    set<uint16_t>(0x2314000,1024*4); check(word()==L"\u0104","actual native raw fingerprints enable Polish automatically after load");
    auto count=rawNames; word(); check(rawNames==count,"known native encoding is cached without repeated getter calls");
    installTextEncoding(); testStatusOwner=false; rawNames=0; check(word()==L"\u00c0" && !rawNames,"worker cannot call native detection getters");
    setSetting(Setting::PolishText,1); check(word()==L"\u0104" && !rawNames,"forced Polish works without game owner or native table");
    testStatusOwner=true; setSetting(Setting::PolishText,0); setTextEncodingDetector(nullptr); stub(0x46c3c0,mapName);
}
void renderedOverkill() {
    constexpr unsigned popupPool=0x2405000,link=0x2405a00;
    set<uint8_t>(0xd2a8e0,1); set<uintptr_t>(0xd334cc,address(0x2120000)); encoded(0x2120540,"Tidus");
    set<uintptr_t>(0xd360b0,address(popupPool)); set<uintptr_t>(0xd360b8,address(link)); set<uintptr_t>(0xd360b4,address(popupPool));
    set<int>(0x2120000+0xf44,100); set<int>(0x2120000+0xf48,100);
    auto allocate=[&](unsigned slot) {
        Event e; e.args[2]=61;e.args[8]=2;e.epoch=generation();
        auto& cb=listeners[0x39fc80][0]; cb.enter(e); set<uintptr_t>(link,address(popupPool+slot*20));
        set<uint8_t>(popupPool+slot*20,1);set<uint8_t>(popupPool+slot*20+1,2);set<uint8_t>(popupPool+slot*20+4,61);
        set<uint8_t>(popupPool+slot*20+6,1);set<uint8_t>(popupPool+slot*20+7,30);e.result=1; cb.leave(e);
    };
    auto draw=[&](uintptr_t result,unsigned parent=0x39f680) {
        Event e; e.args[0]=100;e.args[1]=100;e.args[2]=2;e.args[3]=61;e.args[7]=128;e.parentRva=parent;e.result=result;
        listeners[0x4999b0][0].leave(e);
    };
    allocate(0); auto n=said.size();
    check(said.size()==n,"native popup registration alone cannot expose undisplayed Overkill");
    event(0x39f680,0,[&]{draw(static_cast<uintptr_t>(-1));}); check(said.size()==n,"full native sprite popupPool delays speech");
    event(0x39f680,0,[&]{draw(1,0);}); check(said.size()==n,"unrelated sprite caller cannot expose a label");
    fixtureLocale=1; event(0x39f680,0,[&]{draw(1);}); expect(L"Tidus: OVERKILL","first accepted visible native Overkill word");
    event(0x39f680,0,[&]{draw(1);}); check(said.size()==n+1,"native popup animation frames retain one allocation announcement");
    allocate(0); fixtureLocale=4; event(0x39f680,0,[&]{draw(1);}); expect(L"Tidus: VERNICHTEND","reused popup slot is a new event and uses active atlas wording");
    set<int>(0x2120000+0xf44,600); allocate(0); n=said.size(); event(0x39f680,0); check(said.size()==n,"offscreen native popup is silent");
    set<int>(0x2120000+0xf44,100); set<uint8_t>(popupPool+5,2); event(0x39f680,0,[&]{draw(1);}); check(said.size()==n,"native countdown delays label reveal");
    fixtureLocale=0; fixtureEnglish=0; check(battleGraphicText(BattleGraphic::Overkill)==L"OVER KILL","Japanese graphic keeps its rendered spacing");
    fixtureEnglish=1; check(battleGraphicText(BattleGraphic::Overkill)==L"OVERKILL","Asian alternate English atlas is respected");
    fixtureLocale=3; check(battleGraphicText(BattleGraphic::Overkill)==L"Exterminio","Spanish exact displayed label");
    fixtureLocale=5; check(battleGraphicText(BattleGraphic::Overkill)==L"Ultracidio","Italian exact displayed label");
    resetBattle(); fixtureLocale=1; fixtureEnglish=0; set<uint8_t>(0xd2a8e0,0);
}
void maintenanceReaderFixtures() {
    nativePolishDetection();
    maintenanceMenus();
    renderedOverkill();
    std::puts("PASS: native Polish detection, Save/load choices/destinations, current Overdrive help and rendered Overkill lifetime/locale guards.");
}
}
