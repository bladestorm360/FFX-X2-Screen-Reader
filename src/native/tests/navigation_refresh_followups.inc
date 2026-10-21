namespace {
void navigationRefreshFixtures(unsigned worker,unsigned header,unsigned npc) {
    const auto oldType=read<uint8_t>(address(header)),oldFlags=read<uint8_t>(address(worker+0xab));
    const auto oldCount=read<uint16_t>(address(header+8));
    const auto oldTable=read<uintptr_t>(address(worker+0x44));
    const unsigned geometry=worker+0x558;
    const unsigned fields[]={0,4,8,0x1c,0x30,0x34,0x38};
    float original[7]{};
    for(unsigned i=0;i<7;++i) original[i]=read<float>(address(geometry+fields[i]));
    set<uint8_t>(header,2); set<uint16_t>(header+8,64); set<uint8_t>(worker+0xab,0x11);
    set<uintptr_t>(worker+0x44,0);
    set<float>(geometry,120); set<float>(geometry+4,0); set<float>(geometry+8,0); set<float>(geometry+0x1c,50);
    set<float>(geometry+0x30,120); set<float>(geometry+0x34,0); set<float>(geometry+0x38,100);
    Entity exit; exit.category=L"Exit"; exit.worker=0; exit.usefulEntries=EntryMask{1u<<4};
    refreshTargetZone(exit,address(worker),16);
    unsigned broadChannels=0; interactionEntries(address(worker),EntryMask{},&broadChannels);
    const auto former=interactionZone(address(worker),broadChannels);
    check(former.talk && !former.crossing,"former all-callback refresh reproduces replacement of a valid crossing with unrelated talk");
    Mesh plane; plane.openPlane=true;
    for(const Point player: {Point{80,0,20},Point{100,0,50},Point{160,0,80}}) {
        refreshNavigationTarget(exit);
        check(!exit.inactive && exit.zone.valid && exit.zone.crossing && !exit.zone.talk && exit.zone.end.z==100,
            "movement refresh preserves discovery callback mask and crossing geometry");
        check(!interactionRoute(plane,player,exit.zone).empty(),"preserved exit crossing remains routeable from both sides of the same map");
    }
    set<uint8_t>(worker+0xab,1); refreshNavigationTarget(exit);
    check(exit.inactive && !exit.zone.valid,"unrelated enabled talk cannot keep a genuinely disabled exit active");
    constexpr unsigned table=0x2253000;
    for(unsigned channel=0;channel<7;++channel) set<uint16_t>(table+channel*2,0xffff);
    set<uint16_t>(table,2); set<uint16_t>(table+8,40);
    set<uintptr_t>(worker+0x44,address(table)); set<uint8_t>(worker+0xab,0x11);
    exit.usefulEntries=EntryMask{0u}; exit.usefulEntries.words[1]=1u<<8;
    refreshNavigationTarget(exit);
    check(!exit.inactive && exit.zone.crossing,"refresh preserves useful callbacks above entry 31");
    set<uint16_t>(table+8,41); refreshNavigationTarget(exit);
    check(exit.inactive,"remapping to a no-op callback invalidates the target using the retained semantic mask");
    exit.authoredTrialControl=true; refreshNavigationTarget(exit);
    check(exit.inactive && exit.zone.valid && exit.zone.talk,"verified inactive trial controls retain their native geometry exception");
    set<uint8_t>(header,1); set<uintptr_t>(worker+0x44,0); set<uint8_t>(worker+0xab,5);
    Entity push; push.worker=0; push.pushControl=2; push.usefulEntries=EntryMask{1u<<4}; push.authoredTrialControl=true;
    Entity recess;
    check(pedestalRecessTarget(push,address(worker),recess) && recess.usefulEntries.contains(2) && !recess.usefulEntries.contains(4),
        "copied pedestal socket retains its own useful entry rather than the physical push callback");
    refreshNavigationTarget(recess); check(!recess.inactive && recess.zone.talk,"socket refresh follows enabled sphere entry");
    set<uint8_t>(worker+0xab,4); refreshNavigationTarget(recess);
    check(recess.inactive,"enabled physical contact cannot keep a disabled sphere socket active");
    const unsigned bounds[]={0x5c,0x64,0x4e4}; float savedBounds[3]{},savedPlayerBounds[3]{};
    const auto player=playerActor();
    for(unsigned i=0;i<3;++i) {
        savedBounds[i]=read<float>(address(npc+bounds[i])); savedPlayerBounds[i]=read<float>(player+bounds[i]);
        write<float>(player+bounds[i],i==2?1.f:4.f);
    }
    set<float>(npc+0x5c,20000); set<float>(npc+0x64,20000); set<float>(npc+0x4e4,1);
    refreshNavigationTarget(push);
    check(!push.inactive && push.zone.valid && push.zone.contactRadius>10000,
        "finite native contact bounds have no arbitrary size ceiling and retain enabled physical push guard");
    set<uint8_t>(worker+0xab,1); refreshNavigationTarget(push);
    check(push.inactive,"native contact-channel inhibition still disables physical pushing");
    for(unsigned i=0;i<3;++i) {
        set<float>(npc+bounds[i],savedBounds[i]); write<float>(player+bounds[i],savedPlayerBounds[i]);
    }
    for(unsigned i=0;i<7;++i) set<float>(geometry+fields[i],original[i]);
    set<uint8_t>(header,oldType); set<uint16_t>(header+8,oldCount); set<uint8_t>(worker+0xab,oldFlags);
    set<uintptr_t>(worker+0x44,oldTable);
}
}
