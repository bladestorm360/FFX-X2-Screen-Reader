void airshipSearchFixtures() {
    constexpr unsigned chunkOffset=0x2520000,fieldOffset=0x2540000,configOffset=0x2541000,cameraOffset=0x2542000;
    const auto oldField=read<uintptr_t>(address(0xf26ae8)),oldCamera=read<uintptr_t>(address(0x1f11430)),oldConfig=read<uintptr_t>(address(0x1efb504));
    const auto oldPlayer=read<uintptr_t>(address(0xf00740));const auto oldScene=scene.load(std::memory_order_acquire);
    std::ifstream file(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/hi/hiku2100/hiku2100.ebp",std::ios::binary);
    check(file.good(),"actual airship search archive exists");
    unsigned begin=0;file.seekg(4);file.read(reinterpret_cast<char*>(&begin),4);check(begin>=0x30,"airship event script chunk exists");
    file.seekg(0,std::ios::end);const auto length=static_cast<unsigned>(file.tellg())-begin;check(length<0x20000,"airship fixture chunk bounded");
    file.seekg(begin);file.read(reinterpret_cast<char*>(address(chunkOffset)),length);check(file.gcount()==length,"complete archived airship code and variable declarations");
    const auto chunk=address(chunkOffset),global=chunk+0x150f8;
    set<uintptr_t>(0xf26ae8,address(fieldOffset));set<uintptr_t>(fieldOffset+0x48,chunk);set<uint8_t>(fieldOffset,2);
    set<uintptr_t>(0xf00740,address(0x2240000));set<uintptr_t>(0x1f11430,address(cameraOffset));
    set<float>(cameraOffset+0x40,0);set<float>(cameraOffset+0x48,10);set<float>(cameraOffset+0x65c,0);set<float>(cameraOffset+0x664,0);
    set<uintptr_t>(0x1efb504,address(configOffset));set<uint8_t>(0x859554,0);
    constexpr uint8_t scans[]={0xc8,0xd0,0xcb,0xcd,0x1c,0x01};
    for(unsigned i=0;i<6;++i) {set<unsigned>(configOffset+0x60+i*4,28+i);set<unsigned>(configOffset+0x60+(49+i)*4,128);write<uint8_t>(address(0x830e90)+i,scans[i]);}
    set<unsigned>(configOffset+0x60+18*4,12); // native Confirm's A button
    scene=0;testForeground=true;fixtureInputDevice(false);setSetting(Setting::HiddenAirshipDestinations,0);
    installAirshipSearch();resetAirshipSearch();
    auto capture=[&](float x,float y,float z,bool search=true) {
        write<int>(global+0x150,search?1:0);write<float>(global+0x180,x);write<float>(global+0x184,y);write<float>(global+0x188,z);
        Event e;e.rva=0x46a280;e.parentRva=0x456cc0;e.epoch=generation();e.args[0]=2;
        e.args[1]=global+0x180;e.args[2]=global+0x184;e.args[3]=global+0x188;
        for(const auto& listener:listeners[e.rva]) if(listener.leave) listener.leave(e);
    };
    capture(50,0.125f,-43);drainAirshipSearchForTest();
    const auto savedRenderRoom=read<int>(address(0xf30804));
    const unsigned searchPickerChord='M'|0x10000u;
    check(chords.count(searchPickerChord)!=0,"Shift+M airship picker chord is registered");
    setSetting(Setting::HiddenAirshipDestinations,1);
    for(int renderRoom:{382,0}) {
        // Both native menu routes use the original hiku2100 search flag. Test
        // the normal renderer room and the formerly unrecognized context; the
        // flag, rather than a guessed second room number, supplies ownership.
        set<int>(0xf30804,renderRoom);
        check(chords[searchPickerChord].second(),"Shift+M picker guard accepts the active native search for renderer room 382 and controlled non-382 context");
        Event boundary;boundary.rva=0x225930;boundary.epoch=generation();
        for(const auto& listener:listeners[boundary.rva]) if(listener.leave) listener.leave(boundary);
        check(!fixtureFieldAudioAllowed,"coordinate-reader search state excludes field loops independently of renderer room");
        write<int>(global+0x150,0);set<int>(0xf30804,0);
        for(const auto& listener:listeners[boundary.rva]) if(listener.leave) listener.leave(boundary);
        check(fixtureFieldAudioAllowed,"leaving each search entry restores field audio permission");
        // Same original airship script also owns password entry. Keep its
        // subject separate from character naming on both renderer paths.
        constexpr unsigned passwordWindow=0x2544000;
        resetNaming();scene=14;set<int>(passwordWindow+0x28,4);set<uint8_t>(passwordWindow+0x41,0);
        set<int>(0x146a218,-1);set<int>(0x146a230,0);encoded(0x146a240,"B");set<unsigned>(0x146a244,0);
        Event naming;naming.rva=0x4c3560;naming.epoch=generation();naming.args[0]=address(passwordWindow);
        for(const auto& listener:listeners[naming.rva]) if(listener.enter) listener.enter(naming);
        for(const auto& listener:listeners[naming.rva]) if(listener.leave) listener.leave(naming);
        keys['N'](false);expect(L"Password: B","password readback uses copied airship subject even without renderer room382");
        resetNaming();scene=0;
        write<int>(global+0x150,1);
    }
    set<int>(0xf30804,savedRenderRoom);
    setSetting(Setting::HiddenAirshipDestinations,0);
    check(!hiddenAirshipMenuAvailable() && !beginHiddenAirshipSearch(0),"default-off setting gates native picker and activation");
    check(!chords[searchPickerChord].second(),"Shift+M picker guard rejects the active search when its setting is off");
    auto before=said.size();keys['M'](false);keys['M'](false);
    write<float>(global+0x180,999);write<float>(global+0x184,999);write<float>(global+0x188,999);
    drainAirshipSearchForTest();
    check(said.size()==before+2 && said.back().first==L"X 50.0000, Y minus 0.1250, Z 43.0000","M reads copied displayed coordinate digits; separate manual events remain separate");
    setSetting(Setting::HiddenAirshipDestinations,1);check(hiddenAirshipMenuAvailable(),"verified current search enables picker only with consent setting");
    check(beginHiddenAirshipSearch(0),"approved Baaj search starts");drainAirshipSearchForTest();
    uint8_t keysDown[256]{};check(searchKeyboard && searchController,"central native input assist is installed");searchKeyboard(keysDown);
    check(keysDown[0xc8] || keysDown[0xcb] || keysDown[0xd0] || keysDown[0xcd],"ordinary configured movement steers search cursor");
    std::memset(keysDown,0,256);keysDown[0xc8]=0x80;searchKeyboard(keysDown);std::memset(keysDown,0,256);searchKeyboard(keysDown);
    check(!keysDown[0xc8] && !keysDown[0xcb] && !keysDown[0xd0] && !keysDown[0xcd],"manual movement cancels injected search movement immediately");
    constexpr float destinations[6][2]={{14.3147459f,-59.843399f},{72.0362701f,-36.3792343f},{30.5800018f,-74.6152496f},{35.0868149f,-57.518692f},{41.4966354f,-58.689209f},{14.2042418f,-42.8997765f}};
    for(unsigned i=0;i<6;++i) {
        capture(destinations[i][0],0,destinations[i][1]);drainAirshipSearchForTest();check(beginHiddenAirshipSearch(i),"each approved destination starts in current native search");drainAirshipSearchForTest();
        std::memset(keysDown,0,256);searchKeyboard(keysDown);check(keysDown[0x1c]&0x80,"region arrival uses the normal configured Confirm input");
        std::memset(keysDown,0,256);searchKeyboard(keysDown);check(!keysDown[0x1c],"native confirmation is a single press, not a held key");
    }
    fixtureInputDevice(true);capture(destinations[1][0],0,destinations[1][1]);drainAirshipSearchForTest();check(beginHiddenAirshipSearch(1),"controller search starts");drainAirshipSearchForTest();
    pad::State pad;searchController(pad);check(pad.held&pad::bit(pad::South),"controller arrival uses configured A button");pad={};searchController(pad);check(!pad.held,"controller confirmation releases on the next poll");
    capture(50,0,-43);drainAirshipSearchForTest();check(beginHiddenAirshipSearch(0),"search can restart after arrival");
    setSetting(Setting::HiddenAirshipDestinations,0);drainAirshipSearchForTest();setSetting(Setting::HiddenAirshipDestinations,1);
    std::memset(keysDown,0,256);fixtureInputDevice(false);searchKeyboard(keysDown);check(!keysDown[0xc8]&&!keysDown[0xcb]&&!keysDown[0xd0]&&!keysDown[0xcd],"turning setting off cancels a pending route and reenabling does not resume it");
    capture(50,0,-43,false);drainAirshipSearchForTest();check(!hiddenAirshipMenuAvailable() && !readAirshipCoordinates(),"closed native search retires coordinate and picker context");
    check(!chords[searchPickerChord].second(),"Shift+M picker guard rejects a closed native search");
    resetAirshipSearch();write<uint8_t>(chunk+0xe6c,read<uint8_t>(chunk+0xe6c)^1);capture(50,0,-43);drainAirshipSearchForTest();
    check(!hiddenAirshipMenuAvailable(),"modified bytecode fails the exact approved contract");
    setSetting(Setting::HiddenAirshipDestinations,0);resetAirshipSearch();set<uintptr_t>(0xf26ae8,oldField);set<uintptr_t>(0x1f11430,oldCamera);set<uintptr_t>(0x1efb504,oldConfig);set<uintptr_t>(0xf00740,oldPlayer);scene=oldScene;
}
