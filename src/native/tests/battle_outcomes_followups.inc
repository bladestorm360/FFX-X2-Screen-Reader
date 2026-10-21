namespace {
void battleOutcomesFixtures() {
    constexpr unsigned monster=0x2120000+18*0xf90, formation=0x2320000;
    encoded(monster+0x540,"Larva"); set<uint8_t>(monster+0xdc8,1);
    const auto initial=said.size();
    Event miss; miss.args[0]=18; miss.args[1]=3; miss.args[2]=0;
    listeners[0x39fa20][0].leave(miss); drainSpeechDomainsForTest(); expect((L"Larva: "+battleGraphicText(BattleGraphic::Miss)).c_str(),"native MISS graphic reads named true miss");
    check(!said.back().second,"miss queues without interrupting damage");
    Event damage; damage.args[0]=18; damage.args[2]=123;
    listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest(); expect(L"Larva: 123 damage","damage is preserved alongside misses");
    listeners[0x39fa20][0].leave(miss); drainSpeechDomainsForTest();
    check(said.size()==initial+3 && said[initial+1].first==L"Larva: 123 damage" && !said[initial+1].second,"miss cannot replace numeric damage");
    auto n=said.size(); damage.args[2]=0; listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    expect(L"Larva: 0 damage","a genuine numeric zero reads its target and outcome");
    check(said.size()==n+1,"zero numeric HP damage has one outcome producer"); n=said.size();
    miss.args[1]=4; listeners[0x39fa20][0].leave(miss); drainSpeechDomainsForTest();
    check(said.size()==n+1 && said.back().first==L"Larva: "+battleGraphicText(BattleGraphic::Guard),"accepted guard uses its native graphic label");
    miss.args[1]=5; listeners[0x39fa20][0].leave(miss); drainSpeechDomainsForTest();
    check(said.size()==n+1,"overkill graphic does not imply miss or healing");
    damage.args[2]=static_cast<uintptr_t>(-50); listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    expect(L"Larva: healed for 50","an enemy absorbing an ability reads its actual healing");
    damage.args[1]=1; damage.args[2]=0; listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    expect(L"Larva: 0 MP damage","numeric zero MP also retains its target and result");
    damage.args[3]=static_cast<uintptr_t>(-50); listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    expect(L"Larva: restored 0 MP","zero restoration follows the renderer's native original-amount sign");
    damage.args[1]=0; listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    expect(L"Larva: healed for 0","zero healing is not misreported as damage at full vitals"); n=said.size();
    damage.result=0xffffffff; listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    damage.args[3]=0; listeners[0x39fa20][0].leave(damage); drainSpeechDomainsForTest();
    check(said.size()==n,"rejected zero healing and zero damage popups stay silent"); damage.result=0;
    const auto& deathHook=listeners[0x38c800][0];
    Event death; death.args[1]=18; deathHook.enter(death); deathHook.leave(death); drainSpeechDomainsForTest();
    check(said.size()==n,"death check without committed KO is silent");
    death=Event{}; death.args[1]=18; deathHook.enter(death);
    set<uint8_t>(monster+0xdcc,1); set<uint16_t>(monster+0x606,1); set<uint8_t>(monster+0xdc8,0);
    deathHook.leave(death); drainSpeechDomainsForTest(); expect(L"Larva: KO","KO keeps entry name after native actor deactivation");
    check(said.size()==n+1 && !said.back().second,"confirmed KO queues once");
    death=Event{}; death.args[1]=18; deathHook.enter(death); deathHook.leave(death); drainSpeechDomainsForTest();
    check(said.size()==n+1,"already KO target does not repeat");
    set<uint8_t>(monster+0xdcc,0); set<uint16_t>(monster+0x606,0); set<uint8_t>(monster+0xdc8,1);
    Event effect; effect.rva=0x38f0b0; effect.args[2]=18;
    auto& outer=listeners[0x38f0b0][0]; outer.enter(effect);
    death=Event{}; death.args[1]=18; deathHook.enter(death);
    set<uint8_t>(monster+0xdcc,1); set<uint16_t>(monster+0x606,1);
    deathHook.leave(death); outer.leave(effect); drainSpeechDomainsForTest();
    check(said.size()==n+2,"outer effect and confirmed death produce only one KO");
    effect=Event{}; effect.rva=0x39a1a0; effect.args[0]=18;
    auto& status=listeners[0x39a1a0][0]; status.enter(effect);
    set<uint8_t>(monster+0xdcc,0); set<uint16_t>(monster+0x606,0); status.leave(effect); drainSpeechDomainsForTest();
    expect(L"Larva: revived","revival still uses committed status change");
    resetBattle(); set<uintptr_t>(0xd34460,address(formation));
    std::memset(reinterpret_cast<void*>(address(formation)),0,12*0xf90);
    Event ready; ready.args[0]=5; ready.args[1]=1;
    listeners[0x3982b0][0].leave(ready); drainSpeechDomainsForTest(); n=said.size();
    for(unsigned i=0;i<5;++i) {
        encoded(formation+i*0xf90+0x540,i<2?"Larva":i==2?"Dinonix":"Hidden");
        set<uintptr_t>(formation+i*0xf90,address(win));
        set<uint8_t>(formation+i*0xf90+0xdc8,i!=4);
        set<uint8_t>(formation+i*0xf90+0x15,i!=3);
    }
    for(unsigned i=8;i<12;++i) {
        encoded(formation+i*0xf90+0x540,"w");
        set<uintptr_t>(formation+i*0xf90,address(win));
        set<uint8_t>(formation+i*0xf90+0xdc8,1); set<uint8_t>(formation+i*0xf90+0x15,1);
    }
    listeners[0x3982b0][0].leave(ready); drainSpeechDomainsForTest();
    expect(L"Enemies: Larva x2, Dinonix","visible active formation uses native names counts and order");
    check(said.size()==n+1 && !said.back().second,"formation appends to turn speech");
    listeners[0x3982b0][0].leave(ready); drainSpeechDomainsForTest(); check(said.size()==n+1,"same battle-ready event cannot repeat formation");
    Event encounter; encounter.rva=0x3810f0;
    listeners[0x3810f0][0].enter(encounter); ready.args[0]=4; listeners[0x3982b0][0].leave(ready); drainSpeechDomainsForTest();
    check(said.size()==n+1,"unrelated battle requests do not announce formation");
    ready.args[0]=5; listeners[0x3982b0][0].leave(ready); drainSpeechDomainsForTest();
    check(said.size()==n+2,"identical next battle formation remains a valid new announcement");
    listeners[0x3810f0][0].enter(encounter); drainSpeechDomainsForTest();
    std::memset(reinterpret_cast<void*>(address(formation)),0,12*0xf90);
    const char* enemyNames[]={"Melusine","Kusariqqu","Aerouge"};
    for(unsigned i=0;i<3;++i) {
        const auto chr=formation+i*0xf90; encoded(chr+0x540,enemyNames[i]);
        set<uintptr_t>(chr,address(win)); set<uint8_t>(chr+0xdc8,1);
    }
    encoded(formation+8*0xf90+0x540,"w");
    set<uintptr_t>(formation+8*0xf90,address(win));
    set<uint8_t>(formation+8*0xf90+0xdc8,1); set<uint8_t>(formation+8*0xf90+0x15,1);
    encoded(0x2332000,"Pre-emptive strike"); n=said.size();
    Event message; message.args[1]=address(0x2332000); listeners[0x49db10][0].enter(message); drainSpeechDomainsForTest();
    check(said.size()==n,"opening message waits for enemy appearance");
    Event appearance; appearance.args[0]=20;
    set<uint8_t>(formation+0x15,1); listeners[0x397090][0].leave(appearance); drainSpeechDomainsForTest();
    check(said.size()==n,"first visible enemy cannot truncate formation");
    for(unsigned i=1;i<3;++i) set<uint8_t>(formation+i*0xf90+0x15,1);
    appearance.args[0]=22; listeners[0x397090][0].leave(appearance); drainSpeechDomainsForTest();
    check(said.size()==n+2 && said[n].first==L"Enemies: Melusine, Kusariqqu, Aerouge" && said[n+1].first==L"Pre-emptive strike","appearance formation precedes queued opening message, excluding extra actors");
    speechq::Header protectedHeader{};protectedHeader.generation=generation();
    check(speechq::domainsProtectSpeech(protectedHeader) && !said[n].second && !said[n+1].second,"opening cannot be interrupted");
    output(L"Attack",true,"battle-command"); check(!said.back().second,"later command cannot cancel formation");
    listeners[0x3982b0][0].leave(ready); drainSpeechDomainsForTest(); check(said.size()==n+3,"ready fallback cannot repeat appearance formation");
    listeners[0x3810f0][0].enter(encounter); drainSpeechDomainsForTest(); n=said.size();
    encoded(0x2332000,"Ambushed!"); listeners[0x49db10][0].enter(message); drainSpeechDomainsForTest();
    check(said.size()==n+2 && said[n].first==L"Enemies: Melusine, Kusariqqu, Aerouge" && said[n+1].first==L"Ambushed!",
        "consecutive normal, pre-emptive and ambushed encounters each read their complete formation without field teardown");
    set<uintptr_t>(0xd34460,0); set<uint8_t>(monster+0xdc8,0); resetBattle();
    check(!protectedSpeech,"battle end restores normal interruption policy");
    constexpr unsigned tidus=0x2120000;
    const auto active=read<uint8_t>(address(tidus+0xdc8)); set<uint8_t>(tidus+0xdc8,1);
    auto& commandEffect=listeners[0x38e680][0];
    constexpr unsigned buffer=tidus+0x774;
    set<uint8_t>(tidus+0x65e,0); set<uint8_t>(buffer,0); set<uint8_t>(buffer+1,2);
    set<uint8_t>(buffer+2,0); set<uint8_t>(buffer+3,42);
    const auto playback=[&](unsigned flags=0,unsigned sequence=42,bool consume=true) {
        Event applied; applied.rva=0x38f0b0; applied.args[1]=sequence; applied.args[4]=flags;
        outer.enter(applied);
        if(consume) set<uint8_t>(buffer,read<uint8_t>(address(buffer))+1);
        outer.leave(applied); drainSpeechDomainsForTest();
    };
    for(unsigned stack=1;stack<=2;++stack) {
        Event cheer; cheer.rva=0x38e680; cheer.args[2]=0; cheer.args[3]=address(tidus);
        cheer.args[6]=address(buffer+0x18+(stack-1)*0x2c);
        commandEffect.enter(cheer);
        Event nested; nested.rva=0x39a1a0; nested.parentRva=0x38e680; nested.args[0]=0;
        status.enter(nested); set<uint8_t>(tidus+0x65e,static_cast<uint8_t>(stack)); status.leave(nested);
        n=said.size(); commandEffect.leave(cheer); drainSpeechDomainsForTest();
        check(said.size()==n,"precalculated buffs cannot speak before the ability banner");
    }
    n=said.size(); playback(8,42,false); playback(0,43,false); playback(0,42,false);
    check(said.size()==n,"preview, unrelated sequence and unconsumed packets cannot flush calculated statuses");
    output(L"Tidus: Cheer",false,"fixture-native-action");
    for(unsigned stack=1;stack<=2;++stack) {
        n=said.size(); playback();
        check(said.size()==n+1,"native playback consumes exactly this effect's copied buff delta");
        const auto expected=L"Tidus: Cheer "+std::to_wstring(stack);
        expect(expected.c_str(),"successive Cheer stacks are separate committed changes");
    }
    Event protect; protect.rva=0x38e680; protect.args[2]=0; protect.args[3]=address(tidus);
    protect.args[6]=address(buffer+0x18+2*0x2c); set<uint8_t>(buffer+1,3);
    commandEffect.enter(protect); set<uint8_t>(tidus+0x608+4,1); commandEffect.leave(protect); drainSpeechDomainsForTest();
    playback(); expect(L"Tidus: protect","ordinary command statuses are deferred to their consumed effect as well");
    commandEffect.enter(protect); n=said.size(); commandEffect.leave(protect); drainSpeechDomainsForTest();
    check(said.size()==n,"a capped or unchanged effect cannot claim a new status");
    protect.args[6]=address(buffer+0x18+3*0x2c); set<uint8_t>(buffer+1,4);
    commandEffect.enter(protect); set<uint8_t>(tidus+0x608+4,0); commandEffect.leave(protect); drainSpeechDomainsForTest();
    playback();
    expect(L"Tidus: protect ended","command removal reports the actual ended status");
    protect.args[3]=address(tidus+0xf90); commandEffect.enter(protect);
    set<uint8_t>(tidus+0x65e,0); n=said.size(); commandEffect.leave(protect); drainSpeechDomainsForTest();
    check(said.size()==n,"mismatched target pointer cannot read another character's effects");
    protect.args[3]=address(tidus); protect.args[6]=address(buffer+0x18+4*0x2c); set<uint8_t>(buffer+1,5);
    commandEffect.enter(protect); set<uint8_t>(tidus+0x65e,3); commandEffect.leave(protect); drainSpeechDomainsForTest();
    resetBattle(); n=said.size(); playback();
    check(said.size()==n,"encounter reset drops calculated effects that never played");
    constexpr unsigned otherBuffer=buffer+0x2d8;
    set<uint8_t>(buffer,0); set<uint8_t>(buffer+1,1);
    set<uint8_t>(otherBuffer,0); set<uint8_t>(otherBuffer+1,1);
    set<uint8_t>(otherBuffer+2,0); set<uint8_t>(otherBuffer+3,42);
    for(unsigned slot=0;slot<2;++slot) {
        protect.args[6]=address((slot?otherBuffer:buffer)+0x18);
        commandEffect.enter(protect); set<uint8_t>(tidus+0x65e,static_cast<uint8_t>(4+slot)); commandEffect.leave(protect); drainSpeechDomainsForTest();
    }
    Event two; two.rva=0x38f0b0; two.args[1]=42; outer.enter(two);
    set<uint8_t>(buffer,1); set<uint8_t>(otherBuffer,1); n=said.size(); outer.leave(two); drainSpeechDomainsForTest();
    check(said.size()==n+2 && said[n].first==L"Tidus: Cheer 4" && said[n+1].first==L"Tidus: Cheer 5",
        "two consumed native slots preserve both effect applications instead of merging their status text");
    set<uint8_t>(tidus+0x65e,0); resetBattle();
    set<uint8_t>(tidus+0xdc8,active);
}
}
