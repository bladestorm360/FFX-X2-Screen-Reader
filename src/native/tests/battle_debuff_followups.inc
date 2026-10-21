// Recovered 78F0B0 commits packet+14 status flags, advances its index, then
// 78C800 may clear statuses/deactivate a shattered actor. External animation,
// RNG/resistance calculation and actual renderer/backend remain simulated.
namespace ffx {void resetBattleStatus();}
namespace {
void battleDebuffFixtures() {
    constexpr unsigned actor=0x2120000,buffer=actor+0x774,packet=buffer+0x18;
    std::array<uint8_t,31> actionStates{};
    for(unsigned i=0;i<31;++i) {
        actionStates[i]=read<uint8_t>(address(actor+i*0xf90+0xdf0));
        set<uint8_t>(actor+i*0xf90+0xdf0,0);
    }
    encoded(0x2100060,"Stone Gaze");
    Event title;title.rva=0x498bd0;title.args[0]=1;title.epoch=generation();title.result=1;
    listeners[0x498bd0][0].leave(title);
    encoded(0x2100060,"Changed");
    expect(L"Stone Gaze","debuff title survives absent optional actor flag and source reuse");
    encoded(0x2100060,"Stone Gaze");
    auto n=said.size();listeners[0x498bd0][0].leave(title);expect(L"Stone Gaze","equal distinct ability title is retained");
    check(said.size()==n+1,"same named command on a new action is not deduplicated");
    encoded(0x2310600,"Stone Gaze");
    Event direct;direct.args[0]=1;direct.args[1]=address(0x2310600);
    listeners[0x49db10][0].enter(direct);expect(L"Stone Gaze","direct native ability message slot is spoken");
    n=said.size();direct.parentRva=0x498bd0;listeners[0x49db10][0].enter(direct);drainSpeechDomainsForTest();
    check(said.size()==n,"nested banner message remains owned by its outer action producer");
    Event miss;miss.args[0]=0;miss.args[1]=3;
    listeners[0x39fa20][0].leave(miss);
    const auto missed=L"Tidus: "+battleGraphicText(BattleGraphic::Miss);expect(missed.c_str(),"accepted debuff MISS reads its native locale label");
    miss.args[1]=4;listeners[0x39fa20][0].leave(miss);
    const auto guarded=L"Tidus: "+battleGraphicText(BattleGraphic::Guard);expect(guarded.c_str(),"accepted resistant/immune popup keeps the distinct native label");
    n=said.size();miss.result=static_cast<uintptr_t>(-1);listeners[0x39fa20][0].leave(miss);drainSpeechDomainsForTest();
    check(said.size()==n,"rejected native popup cannot invent a debuff outcome");
    const auto& application=listeners[0x38f0b0][0];
    resetBattleStatus();
    for(unsigned attempt=0;attempt<2;++attempt) {
        set<uint8_t>(actor+0xdc8,1);set<uint8_t>(actor+0xdcc,0);set<uint16_t>(actor+0x606,0);
        set<uint8_t>(buffer,0);set<uint8_t>(buffer+1,1);set<uint8_t>(buffer+2,0);set<uint8_t>(buffer+3,static_cast<uint8_t>(17+attempt));
        set<uint16_t>(packet+0x14,attempt?12:4);
        set<uint16_t>(packet+0x16,0);
        for(unsigned i=0;i<13;++i) set<uint8_t>(packet+7+i,0);
        if(attempt) {set<uint16_t>(actor+0x606,8);set<uint8_t>(actor+0x60c,1);set<uint8_t>(packet+11,1);}
        Event apply;apply.rva=0x38f0b0;apply.args[1]=17+attempt;
        application.enter(apply);
        // Actual status commit; the second case reproduces the subsequent
        // native KO/shatter cleanup without pretending to run an animation.
        set<uint16_t>(actor+0x606,attempt?1:4);
        if(attempt) {set<uint8_t>(actor+0xdcc,1);set<uint8_t>(actor+0xdc8,0);set<uint8_t>(actor+0x60c,0);}
        set<uint8_t>(buffer,1);application.leave(apply);
        expect(L"Tidus: petrify",attempt?"committed petrify survives cleanup without false poison cure/protect-ended text":"committed ordinary petrify is spoken");
    }
    set<uint8_t>(actor+0xdc8,1);set<uint8_t>(actor+0xdcc,0);set<uint16_t>(actor+0x606,0);
    set<uint8_t>(actor+0x60c,0);set<uint16_t>(actor+0x616,0);set<uint8_t>(actor+0x5c8,7);
    set<uint8_t>(buffer,0);set<uint8_t>(buffer+1,1);set<uint8_t>(buffer+3,29);
    set<uint16_t>(packet+0x14,0);set<uint16_t>(packet+0x16,0x4000);
    for(unsigned i=0;i<13;++i) set<uint8_t>(packet+7+i,0);
    Event doom;doom.rva=0x38f0b0;doom.args[1]=29;application.enter(doom);
    set<uint16_t>(actor+0x606,1);set<uint16_t>(actor+0x616,0);set<uint8_t>(actor+0x5c8,0);
    set<uint8_t>(actor+0xdcc,1);set<uint8_t>(buffer,1);application.leave(doom);
    expect(L"Tidus: doom, 7 turns","consumed Doom retains its real prepared counter through KO cleanup");
    set<uint8_t>(actor+0x5c8,0);set<uint16_t>(packet+0x16,0);
    set<uint8_t>(actor+0xdc8,1);set<uint8_t>(actor+0xdcc,0);set<uint16_t>(actor+0x606,0);
    set<uint8_t>(buffer,0);set<uint8_t>(buffer+1,1);set<uint8_t>(buffer+3,30);set<uint16_t>(packet+0x14,4);
    Event unchanged;unchanged.rva=0x38f0b0;unchanged.args[1]=30;unchanged.args[4]=8;
    n=said.size();application.enter(unchanged);application.leave(unchanged);drainSpeechDomainsForTest();
    check(said.size()==n,"skipped packet or unchanged state never implies applied petrify or Miss");
    set<uint8_t>(buffer+1,0);resetBattleStatus();
    for(unsigned i=0;i<31;++i) set<uint8_t>(actor+i*0xf90+0xdf0,actionStates[i]);
    encoded(0x2100060,"Potion");
    std::puts("PASS: Stone Gaze/similar native titles, direct/nested messages, copied source, equal events, accepted Miss/Guard, consumed petrify/shatter and unchanged guards.");
}
}
