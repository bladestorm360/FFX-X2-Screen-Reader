namespace ffx {void resetBattleResults();}
namespace {
void escFocusFixtures() {
    constexpr unsigned entry=0x24a0000,config=0x24a1000;
    const auto oldConfig=read<uintptr_t>(address(0x1efb504));
    const auto oldMenu=read<uintptr_t>(address(0x8cc868));
    uint8_t previousKeyName[7]{};check(copy(address(0x5af0),previousKeyName,sizeof(previousKeyName)),"save previous keyboard-name fixture");
    stub(0x5af0,bindingLabel);
    const int automatic=setting(Setting::AutoDetail);exactEscLabels=true;setSetting(Setting::AutoDetail,0);
    set<uintptr_t>(0x1efb504,address(config));
    set<uintptr_t>(0x8cc868,address(config)+0x1000);set<uint8_t>(config+0x1070,1);
    set<unsigned>(entry,0x218);set<int16_t>(entry+4,1);set<int16_t>(entry+6,0);
    event(0x28e6e0,address(entry));expect(L"Resume","Escape highlights speak the actual option name");
    keys['I'](false);expect(L"Resume game","Escape manual detail is the displayed help");
    set<uint8_t>(0x1685f70,1); // Underlying Sphere Grid description must not win.
    set<unsigned>(entry,0x222);set<int16_t>(entry+4,11);set<int16_t>(entry+6,0);
    event(0x28e6e0,address(entry));expect(L"Audio","System submenu speaks its label");
    keys['I'](false);expect(L"Change audio settings","submenu I wins over the underlying Grid node");
    setSetting(Setting::AutoDetail,1);const auto n=said.size();
    // Context can hide preceding home rows; semantic help identity still names it.
    set<int16_t>(entry+4,1);set<int16_t>(entry+6,4);
    event(0x28e6e0,address(entry));
    check(said.size()==n+2 && said[n].first==L"Audio" && said.back().first==L"Change audio settings","Escape AutoDetail orders label then actual help independently of row");
    set<unsigned>(entry,0x2b7);set<int16_t>(entry+4,13);set<int16_t>(entry+6,0);
    event(0x28e6e0,address(entry));check(said[said.size()-2].first==L"Encounters","Change parameters uses the actual native builder caption");
    // Actual Special Features callback points to its native caption/help table.
    // Original702740 data: action28/caption291/activation290/help293;
    // encounter override is action34/caption297/activation290/help299.
    write<unsigned>(address(0x83a2bc)-4,28);write<unsigned>(address(0x83a2bc),0x291);
    write<unsigned>(address(0x83a2bc)+4,0x290);write<unsigned>(address(0x83a2bc)+16,0x293);
    const auto encounter=address(0x83a2bc)+2*24;
    write<unsigned>(encounter-4,34);write<unsigned>(encounter,0x297);
    write<unsigned>(encounter+4,0x290);write<unsigned>(encounter+16,0x299);
    set<unsigned>(config+0x60+28*4,0x3b);set<unsigned>(config+0x60+34*4,0x3d);
    strcpy_s(reinterpret_cast<char*>(address(0x2310540)),32,"F1");
    strcpy_s(reinterpret_cast<char*>(address(0x2310560)),32,"F3");
    Event focus;listeners[0x28e730][0].enter(focus);
    Event value;value.args[0]=0;value.result=12;listeners[0x628b20][0].leave(value);
    value.args[0]=1;value.result=0;listeners[0x628b20][0].leave(value);
    const auto beforeSpecial=said.size();listeners[0x28e730][0].leave(focus);
    check(said.size()==beforeSpecial,"actual Special Features page12 never emits controller key mappings");
    event(0x28e6e0,address(0x83a2bc)+16);
    if(said.size()>=2 && said[said.size()-2].first!=L"Speed. Press F1 to activate")
        std::fwprintf(stderr,L"Special Features focus received [%s]\n",said[said.size()-2].first.c_str());
    check(said[said.size()-2].first==L"Speed. Press F1 to activate" && said.back().first==L"Change game speed",
        "Special Features focus includes its displayed activation, then separate native AutoDetail");
    setSetting(Setting::AutoDetail,0);
    listeners[0x28e730][0].enter(focus);
    value.args[0]=0;value.result=12;listeners[0x628b20][0].leave(value);
    value.args[0]=1;value.result=2;listeners[0x628b20][0].leave(value);
    listeners[0x28e730][0].leave(focus);
    event(0x28e6e0,encounter+16);expect(L"Enemy Encounter Override. Press F3 to activate",
        "encounter activation comes from the actual action34 binding with AutoDetail Off");
    keys['I'](true);drainSpeechDomainsForTest();expect(L"Press F3 to activate","Shift I repeats the selected Special Features control");
    keys['I'](false);expect(L"Set enemy encounter frequency. It is not affected by character equipment.",
        "Special Features I keeps its native help separate from control help");
    set<unsigned>(config+0x60+34*4,3);
    event(0x28e6e0,encounter+16);expect(L"Enemy Encounter Override. Press Key three to activate",
        "a remapped native activation key is not hard-coded to F3");
    // A different Escape page must release the selected Special Features controls.
    listeners[0x28e730][0].enter(focus);
    value.args[0]=0;value.result=11;listeners[0x628b20][0].leave(value);
    value.args[0]=1;value.result=0;listeners[0x628b20][0].leave(value);
    listeners[0x28e730][0].leave(focus);
    keys['I'](true);drainSpeechDomainsForTest();expect(L"No controls displayed.","Special Features control help cannot leak into another Escape page");
    setSetting(Setting::AutoDetail,0);
    set<unsigned>(0x839624,0x221);set<unsigned>(0x83962c,4);set<unsigned>(config+16,73);
    set<unsigned>(entry,0x230);set<int16_t>(entry+4,110);set<int16_t>(entry+6,0);
    event(0x28e6e0,address(entry));expect(L"Audio: 73","Audio focus retains its configured value");
    keys['I'](false);expect(L"Set master volume","Audio I speaks its own displayed help over the Grid");
    auto at=said.size();event(0x28fca0,0);
    check(said.size()==at+2 && said[at].first==L"Quit game?" && said.back().first==L"No" && !said.back().second,
        "quit prompt and actual initial No are separate, with prompt speech preserved");
    keys['I'](false);expect(L"Quit game?","Quit detail contains the prompt without enumerating choices");
    constexpr unsigned soundValue=0x24a3000,soundText=0x24a3100;
    set<uintptr_t>(soundValue+8,address(soundText));
    auto sound=[&](const char* text,uintptr_t owner) {
        strcpy_s(reinterpret_cast<char*>(address(soundText)),32,text);
        Event e;e.args[0]=owner;e.args[1]=address(soundValue);
        for(auto h:listeners[0x28fb60])if(h.enter)h.enter(e);
        drainSpeechDomainsForTest();
    };
    sound("change",address(config)+0x1000);expect(L"Yes","accepted popup focus event reads Yes");
    sound("change",address(config)+0x1000);expect(L"No","next native focus event reads No");
    at=said.size();sound("change",address(config));check(said.size()==at,"another movie cannot alter Quit focus");
    sound("error",address(config)+0x1000);check(said.size()==at,"unrelated sound is not focus");
    sound("cancel",address(config)+0x1000);sound("change",address(config)+0x1000);
    check(said.size()==at,"cancelled popup releases focus speech");
    event(0x28fca0,0);expect(L"No","reopening always reads the actual default No");
    Event visibility;visibility.ecx=address(config)+0x1000;visibility.args[0]=0;
    for(auto h:listeners[0x628ef0])if(h.enter)h.enter(visibility);
    at=said.size();sound("change",address(config)+0x1000);check(said.size()==at,"Escape closure retires Quit focus");
    constexpr uintptr_t method=0x24a4100, prompt=0x24a4200, yes=0x24a4300, no=0x24a4400, stack=0x24a4500;
    strcpy_s(reinterpret_cast<char*>(address(method)),32,"showPopUpWithBtn");
    auto parameter=[&](const char* p,const char* y,const char* n,bool accepted=true,unsigned epoch=0,bool foreignOwner=false,unsigned type=5,const char* m="showPopUpWithBtn") {
        strcpy_s(reinterpret_cast<char*>(address(prompt)),512,p);strcpy_s(reinterpret_cast<char*>(address(yes)),512,y);strcpy_s(reinterpret_cast<char*>(address(no)),512,n);
        strcpy_s(reinterpret_cast<char*>(address(method)),32,m);
        Event e;e.rva=0x28df70;e.epoch=epoch?epoch:generation();e.nativeStack=address(stack);e.args[0]=address(config)+0x1000;e.args[1]=address(method);
        if(foreignOwner)e.args[0]=address(config)+0x2000;
        e.args[2]=type;e.args[4]=address(prompt);e.args[5]=static_cast<uintptr_t>(strlen(p));
        e.args[6]=5;e.args[8]=address(yes);e.args[9]=static_cast<uintptr_t>(strlen(y));e.args[10]=5;
        write<uintptr_t>(address(stack)+4+12*4,address(no));write<unsigned>(address(stack)+4+13*4,static_cast<unsigned>(strlen(n)));
        const auto opened=said.size();
        for(auto h:listeners[e.rva])if(h.enter)h.enter(e);
        for(auto h:listeners[e.rva])if(h.leave)h.leave(e);
        drainSpeechDomainsForTest();
        if(accepted) {
            ReadyBytes<512> wantedPrompt,wantedNo;
            copyReadyAscii(address(prompt),wantedPrompt,512);copyReadyAscii(address(no),wantedNo,512);
            check(said.size()==opened+2 && said[opened].first==decodeReadyAscii(wantedPrompt) &&
                said.back().first==decodeReadyAscii(wantedNo) && !said.back().second,
                "actual native by-value strings produce prompt then localized No without interruption");
        }
    };
    parameter("Localized option alpha?","Oui","Non");
    sound("change",address(config)+0x1000);expect(L"Oui","parameter popup accepted focus change speaks its localized Yes label");
    sound("change",address(config)+0x1000);expect(L"Non","parameter popup focus returns to localized No");
    sound("cancel",address(config)+0x1000);
    parameter("Localized option beta?","Ja","Nein");
    sound("cancel",address(config)+0x1000);
    parameter("Localized option alpha?","Oui","Non");
    sound("cancel",address(config)+0x1000);
    at=said.size();parameter("Wrong type?","Yes","No",false,0,false,4);
    parameter("Wrong owner?","Yes","No",false,0,true);
    parameter("Wrong method?","Yes","No",false,0,false,5,"otherMethod");
    parameter("Stale epoch?","Yes","No",false,generation()+1);
    check(said.size()==at,"parameter popup rejects wrong method, owner, type and stale epoch");
    parameter("AutoDetail independent?","Sí","No");sound("cancel",address(config)+0x1000);
    testStatusOwner=false;at=said.size();parameter("Wrong thread?","Yes","No",false);testStatusOwner=true;
    check(said.size()==at,"parameter popup requires native game thread");
    set<uint8_t>(0x1685f70,0);exactEscLabels=false;setSetting(Setting::AutoDetail,automatic);
    set<uintptr_t>(0x1efb504,oldConfig);resetSelectionDetail();
    set<uintptr_t>(0x8cc868,oldMenu);
    memcpy(reinterpret_cast<void*>(address(0x5af0)),previousKeyName,sizeof(previousKeyName));
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(0x5af0)),sizeof(previousKeyName));
    puts("PASS: contextual Escape labels, Special Features/parameters, AutoDetail and Audio I over underlying Grid.");
}
void specialFeatureNotificationFixtures() {
    constexpr unsigned owner=0x24a5000;
    const int automatic=setting(Setting::AutoDetail);setSetting(Setting::AutoDetail,0);exactEscLabels=true;
    auto notify=[&](unsigned id,bool ready=true,bool foreign=false,unsigned epoch=0) {
        set<unsigned>(owner,ready?1:0);Event e;e.rva=0x2e88b0;e.ecx=address(owner);e.args[0]=id;
        e.epoch=epoch?epoch:generation();e.result=ready?0xffffff01u:0xffffff00u;testStatusOwner=!foreign;
        for(auto h:listeners.at(e.rva))if(h.enter)h.enter(e);
        for(auto h:listeners.at(e.rva))if(h.leave)h.leave(e);
        testStatusOwner=true;drainSpeechDomainsForTest();
    };
    for(unsigned id:{0x2f0u,0x2f1u,0x2f6u,0x2f7u,0x2f9u,0x2fau,0x2fbu,0x2fdu,0x2feu,0x2ffu,0x37fu,0x380u}) {
        const auto before=said.size();notify(id);
        check(said.size()==before+1 && said.back().first==L"Native feature result "+std::to_wstring(id),
            "each accepted native feature result speaks its exact localized state with AutoDetail Off");
    }
    auto before=said.size();notify(0x2fa);notify(0x2fa);
    check(said.size()==before+2,"equal accepted feature activations remain separate events");
    before=said.size();notify(0x2fa,false);notify(0x30a);notify(0x2fa,true,true);notify(0x2fa,true,false,generation()+1);
    check(said.size()==before,"missing movie, unrelated toast, foreign owner and stale epoch stay silent");
    exactEscLabels=false;setSetting(Setting::AutoDetail,automatic);
    puts("PASS: native Special Features result notifications, all twelve modes, AutoDetail independence, equal events and original movie/owner/epoch guards.");
}
void battleResultsBufferFixtures() {
    constexpr unsigned reward=0x1f10ea0;
    const int oldScene=scene,automatic=setting(Setting::AutoDetail);
    resetBattleResults();scene=0;setSetting(Setting::AutoDetail,0);
    set<uint8_t>(0x1469ed8,2);write<int16_t>(address(0x1469ee6),1);write<int16_t>(address(0x1469ee6)+14,0);
    set<uint8_t>(0xd3205c+0x94+0x3b,3);set<uint8_t>(0xd3205c+0x3b,4);
    set<unsigned>(reward+0x80,120);set<unsigned>(reward+0x84,60);
    set<unsigned>(0x1469ed4,90);
    set<uint8_t>(reward+0xd0,1);set<uint8_t>(reward+0xd1,0);set<uint8_t>(reward+0xd2,0);
    set<uint16_t>(reward+0xd4,2);set<uint8_t>(reward+0xe4,3);
    event(0x4bdb30,0);set<int>(0x1469ec8,0xc);auto opened=said.size();event(0x4ba620,0);
    check(said.size()==opened+2 && said[opened].first==L"AP" && said.back().first==L"Yuna. AP earned: 60. Tidus. AP earned: 120",
        "earned AP speaks in native participant order with Autodetail Off, without per-character statistics");
    check(contexts.at(VK_DOWN)(),"AP results own review arrows");
    keys[VK_DOWN](false);expect(L"Yuna. AP earned: 60","earned total remains in review independently of rolling AP");
    keys[VK_DOWN](false);expect(L"Yuna. AP: 60. Sphere Level: 3. Next AP: 151","results preserve native participant display order");
    // Recovered native counter values mutate during the controller, before our outer return.
    auto n=said.size();event(0x4ba620,0,[=]{set<unsigned>(reward+0x84,0);set<unsigned>(reward+0x80,0);set<uint8_t>(0xd3205c+0x94+0x3b,5);set<int>(0x1469ec8,0xd);});
    check(said.size()==n+1 && said.back().first==L"Yuna. Sphere levels gained: 2","Confirm completion announces the actual cumulative level gain with Autodetail Off");
    n=said.size();event(0x4ba620,0);check(said.size()==n,"completed level gain does not repeat on idle controller calls");
    keys['I'](false);check(said.back().first.find(L"Yuna. AP: 0. Sphere Level: 5")!=std::wstring::npos,"AP Confirm completion updates current buffer values");
    keys[VK_DOWN](false);expect(L"Yuna. Sphere levels gained: 2","completed gain is also reviewable");
    keys[VK_DOWN](false);expect(L"Tidus. AP earned: 120","original earned AP survives the award animation");
    keys[VK_DOWN](false);expect(L"Tidus. AP: 0. Sphere Level: 4. Next AP: 151","counter refresh preserves the current review row");
    // On-demand review refresh while the native counter is still rolling.
    set<int>(0x1469ec8,0xc);event(0x4ba620,0,[=]{set<unsigned>(reward+0x80,7);});
    keys['I'](false);check(said.back().first.find(L"Tidus. AP: 7")!=std::wstring::npos,"review sees the current intermediate native counter too");
    set<unsigned>(0x1469ed4,90);set<int>(0x1469ec8,0x18);n=said.size();event(0x4ba620,0);
    check(said.size()==n+2 && said[n].first==L"Gil and items" && said.back().first==L"Gil earned: 90","earned gil speaks with Autodetail Off while drops stay silent");
    keys['I'](false);expect(L"Gil and items. Gil earned: 90. Gil: 90. Total Gil: 16563. Ether x3","second phase contains earned/current gil, wallet and native item quantities only");
    n=said.size();event(0x4ba620,0,[]{set<unsigned>(0x1469ed4,0);fixtureGil=16653;set<int>(0x1469ec8,0x19);});
    check(said.size()==n,"gil Confirm completion does not create speech spam");
    keys['I'](false);expect(L"Gil and items. Gil earned: 90. Gil: 0. Total Gil: 16653. Ether x3","gil skip preserves earned total while refreshing completed displayed values");
    keys[VK_DOWN](false);expect(L"Gil earned: 90","new phase resets buffer position");
    keys[VK_DOWN](false);expect(L"Gil: 0","rolling counter remains reviewable");
    set<int>(0x1469ec8,0x20);event(0x4ba620,0);check(!contexts.at(VK_DOWN)(),"capacity management releases reward review arrows");
    resetBattleResults();check(!readSelectionDetail(),"closing results cannot expose its old detail buffer");
    // Maximum level reproduces the native dashes, not a fabricated next-level cost.
    set<uint8_t>(0xd3205c+0x94+0x3b,99);event(0x4bdb30,0);set<int>(0x1469ec8,0xe);event(0x4ba620,0);
    keys[VK_DOWN](false);keys[VK_DOWN](false);expect(L"Yuna. AP: 0. Sphere Level: 99. Next AP: -----","capped Sphere Level follows native results rendering");
    resetBattleResults();setSetting(Setting::AutoDetail,1);event(0x4bdb30,0);set<int>(0x1469ec8,0x18);event(0x4ba620,0);
    check(said.back().first.find(L"Ether x3")!=std::wstring::npos,"Autodetail On retains automatic native item details");
    resetBattleResults();fixtureGil=16563;set<uint8_t>(0xd3205c+0x94+0x3b,3);setSetting(Setting::AutoDetail,automatic);scene=oldScene;
    puts("PASS: two-phase reward buffers, native participant order, live review, AP/gil Confirm skip, cursor preservation, cap and retirement.");
}
}
