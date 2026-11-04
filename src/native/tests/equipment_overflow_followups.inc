// Actual results return hooks, ready-text adapter and I/Autodetail. Native
// state transitions/getters are controlled at the retained 8BA620/8BCFE0
// contracts; this does not simulate renderer timing, input edges or a save.
namespace ffx {void resetEquipmentOverflow();}
namespace {
unsigned overflowLocale=1,overflowReads=0;
uintptr_t __cdecl overflowSubstitution(unsigned slot,uintptr_t token) {
    check(testStatusOwner && slot==7 && read<uint8_t>(token)==0x12,"capacity warning uses native window7 substitution on its owner");
    return address(0x24c2340);
}
unsigned __cdecl overflowLanguage() {check(testStatusOwner,"overflow locale stays on native owner");return overflowLocale;}
uintptr_t __cdecl overflowGear(unsigned handle,uintptr_t*) {
    check(testStatusOwner,"overflow equipment getter stays on native owner");++overflowReads;
    if(handle==255)return 0;
    return address(0x24c1000)+(handle>>12==5?(handle&0xfff)%3:3+(handle&0xfff))*32;
}
uintptr_t __cdecl overflowName(unsigned id,unsigned,int,uintptr_t) {
    check(testStatusOwner,"overflow name getter stays on native owner");return address(id==0x7a01?0x24c2000:0x24c2040);
}
void equipmentOverflowFixtures() {
    constexpr unsigned table=0x24c0000;
    const auto fields=address(0x146a148),prompts=address(0x21cc590),discard=address(0x1f10f8e);
    const auto previousWindow=read<uintptr_t>(address(0x14676b0)+7*4);write<uintptr_t>(address(0x14676b0)+7*4,0);
    const int oldAuto=setting(Setting::AutoDetail),oldIndex=setting(Setting::MenuIndex),oldScene=scene;
    uint8_t priorLocale[7]{};check(copy(address(0x241290),priorLocale,sizeof(priorLocale)),"retain existing locale stub");
    stub(0x241290,overflowLanguage);stub(0x3abbf0,overflowGear);stub(0x3a0c70,overflowName);
    encoded(0x24c2000,"Sonic Steel");encoded(0x24c2040,"Hunter Sword");
    encoded(0x24c2100,"Equipment capacity exceeded.");encoded(0x24c2200,"Choose equipment to keep.");
    encoded(0x24c2300,"Discard this equipment.");
    for(unsigned i=0;i<200;++i) write<uint16_t>(address(table)+i*2,static_cast<uint16_t>(0x5000+i));
    for(unsigned i=0;i<6;++i)write<uint16_t>(discard+i*2,static_cast<uint16_t>(0x7000+i));
    for(unsigned i=0;i<9;++i) {
        const auto record=address(0x24c1000)+i*32;std::memset(reinterpret_cast<void*>(record),0,22);
        write<uint16_t>(record,i%2?0x7a02:0x7a01);write<uint8_t>(record+2,1);write<uint8_t>(record+6,255);write<uint8_t>(record+11,1);
        for(unsigned j=0;j<4;++j)write<uint16_t>(record+14+j*2,j?255:1);
    }
    auto initialize=[&](bool standalone=false) {
        resetBattleResults();scene=0;overflowLocale=1;
        Event open;open.epoch=generation();open.args[1]=standalone?1:0;
        for(const auto& h:listeners.at(0x4bdb30))if(h.leave)h.leave(open);
        write<uintptr_t>(address(0x21cc580),address(table));write<int>(fields,200);write<int>(fields+8,0);
        write<int>(fields+0x14,0);write<int>(fields+0x18,0);write<int>(fields+0x1c,0);write<int>(fields+0x20,0);
        write<uintptr_t>(prompts-8,address(0x24c2100));write<uintptr_t>(prompts,address(0x24c2200));set<int>(0x1469ec8,0x20);
    };
    setSetting(Setting::MenuIndex,0);
    double native=0,usable=0,maximum=0;unsigned episodes=0;
    auto advance=[&] {event(0x4ba620,0);native+=fixtureNativeEventMs;usable+=fixtureUsableEventMs;maximum=std::max(maximum,fixtureNativeEventMs);++episodes;};
    for(bool standalone:{false,true})for(int automatic:{0,1}) {
        setSetting(Setting::AutoDetail,automatic);initialize(standalone);const auto n=said.size();advance();
        check(said.size()==n+1+automatic && said[n].first==L"Equipment capacity exceeded., Choose equipment to keep., Keep list, Sonic Steel","overflow first focus reads native prompt, list and gear for both entry paths");
        keys['I'](false);expect(L"Potion","overflow manual I uses selected modifiers with Autodetail On or Off");
        const auto getters=overflowReads;keys['I'](false);check(overflowReads==getters,"manual I never reenters the game getter");
        write<int>(fields+0x18,1);advance();
        check(said[said.size()-1-automatic].first==L"Keep list, Hunter Sword","keep focus uses current native row");
        write<int>(fields+0x14,1);advance();
        check(said[said.size()-1-automatic].first==L"Discard list, Hunter Sword","discard list switch announced even for an equal equipment name");
        write<int>(fields+0x20,1);set<int>(0x1469ec8,0x25);advance();
        check(said[said.size()-1-automatic].first==L"Keep list, Hunter Sword","swap target follows native inverted selection side");
        write<int>(fields+0x20,0);set<int>(0x1469ec8,0x23);write<uintptr_t>(prompts,address(0x24c2300));advance();
        check(said[said.size()-1-automatic].first.find(L"Discard this equipment.")!=std::wstring::npos,"native discard warning is spoken");
        const auto quiet=said.size();for(unsigned i=0;i<50;++i)event(0x4ba620,0);check(said.size()==quiet,"idle warning callbacks do not repeat speech");
        set<int>(0x1469ec8,0x20);write<uintptr_t>(prompts,address(0x24c2200));advance();
        set<int>(0x1469ec8,0x23);write<uintptr_t>(prompts,address(0x24c2300));const auto again=said.size();advance();
        check(said.size()==again+1+automatic,"repeated valid warning remains a separate event");
        set<int>(0x1469ec8,0x2a);event(0x4ba620,0);check(!readSelectionDetail(),"leaving overflow retires its description");
    }
    initialize();setSetting(Setting::AutoDetail,0);advance();
    // Native row/handle can stay fixed after the game swaps record contents.
    auto record=overflowGear(0x5000,nullptr);write<uint16_t>(record,0x7a02);advance();expect(L"Keep list, Hunter Sword","same-handle record mutation refreshes the focused name");
    write<uint8_t>(record+6,0);advance();expect(L"Keep list, Hunter Sword, Equipped","native equipped marker is announced");
    write<uint8_t>(record+6,255);write<uint8_t>(record+2,0);advance();expect(L"Keep list, Empty","unowned native record is an empty cell");
    write<uint16_t>(address(table),255);advance();expect(L"Keep list, Empty","native FF sentinel is an empty cell");
    write<uint16_t>(address(table),0x50ff);advance();check(!readSelectionDetail(),"out-of-range native alias never names record zero");
    write<uint16_t>(address(table),0x5000);write<uint8_t>(record+2,1);advance();
    write<int>(fields,201);const auto invalid=said.size();event(0x4ba620,0);check(said.size()==invalid && !readSelectionDetail(),"oversized native table retires instead of scanning");
    write<int>(fields,200);write<int>(fields+8,194);write<int>(fields+0x18,5);setSetting(Setting::MenuIndex,1);advance();
    check(said.back().first.find(L"200/200")!=std::wstring::npos,"last native equipment slot and optional menu index are bounded");
    setSetting(Setting::MenuIndex,0);
    for(unsigned locale:{0u,1u,2u,3u,4u,5u,9u,10u}) {
        overflowLocale=locale;resetSelectionDetail();advance();
        check(!said.back().first.empty(),"all eight native locales have list headings");
    }
    overflowLocale=1;stub(0x4b6610,overflowSubstitution);encoded(0x24c2340,"Native reward");
    encoded(0x24c2400,"Discard ");const auto warning=address(0x24c2400);
    write<uint8_t>(warning+8,0x12);write<uint8_t>(warning+9,0x30);write<uint8_t>(warning+10,0x48);write<uint8_t>(warning+11,0);
    write<uintptr_t>(prompts,warning);advance();
    check(said.back().first.find(L"Discard Native reward.")!=std::wstring::npos,"warning reads the native substituted equipment name even when it differs from current selection");
    const auto quiet=said.size();double idleNative=0,baselineNative=0;const auto started=std::chrono::steady_clock::now();
    for(unsigned i=0;i<1000;++i) {event(0x4ba620,0);idleNative+=fixtureNativeEventMs;}
    check(said.size()==quiet,"1000 idle callbacks leave no speech backlog");
    const auto fullIdleMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    resetEquipmentOverflow();
    for(unsigned i=0;i<1000;++i) {event(0x4ba620,0);baselineNative+=fixtureNativeEventMs;}
    printf("METRIC overflow %u changed episodes native aggregate=%.3fms max=%.3fms capture-to-owned-output aggregate=%.3fms;1000 idle native aggregate=%.3fms disabled baseline=%.3fms; full fixture/drain overhead=%.3fms. Controlled native/getter/queue boundaries, not actual game or audible latency.\n",episodes,native,maximum,usable,idleNative,baselineNative,fullIdleMs);
    resetBattleResults();scene=oldScene;setSetting(Setting::AutoDetail,oldAuto);setSetting(Setting::MenuIndex,oldIndex);
    stub(0x3abbf0,gearRecord);stub(0x3a0c70,equipmentLabel);
    stub(0x4b6610,messageSubstitution);write<uintptr_t>(address(0x14676b0)+7*4,previousWindow);
    std::memcpy(reinterpret_cast<void*>(address(0x241290)),priorLocale,sizeof(priorLocale));FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(0x241290)),sizeof(priorLocale));
    puts("PASS: native overflow entry/list/cursor/replacement/warning/empty/equipped/record mutation/index bounds, manual I and Autodetail, all eight labels, repeated events, exit and idle pressure.");
}
}
