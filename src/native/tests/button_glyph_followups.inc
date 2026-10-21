namespace {
std::string glyphBinding;
uintptr_t __cdecl verifiedKeyName(unsigned id) {
    constexpr const char* names[]={"Up","Down","Left","Right","Enter","Escape","E","Tab",
        "Q","R","Z","C","Space","Backspace"};
    if(id<28 || id>=128) return 0;
    glyphBinding=id<42?names[id-28]:"Key "+std::to_string(id);
    return reinterpret_cast<uintptr_t>(glyphBinding.c_str());
}
uintptr_t __cdecl verifiedPadName(unsigned id) {
    constexpr const char* names[]={"X button","A button","B button","Y button","LB","RB","LT","RT",
        "View","Menu","L3","R3","Up","Right","Down","Left"};
    return id>=11 && id<=26?reinterpret_cast<uintptr_t>(names[id-11]):0;
}
void archivedSwimmingPrompt(const char* area,unsigned index,unsigned destination,const char* group="cd") {
    const auto path=(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj").string()+"/"+group+"/"+
        area+"/"+area+".ebp";
    std::ifstream source(path,std::ios::binary);
    check(source.good(),"actual swimming text archive exists");
    unsigned begin=0,end=0;
    source.seekg(20); source.read(reinterpret_cast<char*>(&begin),4); source.read(reinterpret_cast<char*>(&end),4);
    check(begin && begin<end && end-begin<0x10000,"archived text chunk bounds");
    std::vector<uint8_t> bytes(end-begin);
    source.seekg(begin); source.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    check(source.gcount()==static_cast<std::streamsize>(bytes.size()) && index*8+4<=bytes.size(),"complete archived string table");
    unsigned offset=0; memcpy(&offset,bytes.data()+index*8,4); offset&=0xffff;
    check(offset<bytes.size(),"archived native string offset");
    const auto finish=std::find(bytes.begin()+offset,bytes.end(),0);
    check(finish!=bytes.end(),"archived native prompt terminator");
    const auto length=static_cast<size_t>(finish-(bytes.begin()+offset))+1;
    check(copy(reinterpret_cast<uintptr_t>(bytes.data()+offset),reinterpret_cast<void*>(address(destination)),length),
        "original prompt bytes including button tokens copied");
}
void buttonGlyphFixtures() {
    constexpr unsigned config=0x2380000,stream=0x2340000,window=0x2330000,cursor=0x2330200;
    const auto oldConfig=read<uintptr_t>(address(0x1efb504));
    uint8_t keyCode[7]{},padCode[7]{};
    check(copy(address(0x5af0),keyCode,7) && copy(address(0x5b10),padCode,7),"save native-name fixture stubs");
    stub(0x5af0,reinterpret_cast<void*>(verifiedKeyName)); stub(0x5b10,reinterpret_cast<void*>(verifiedPadName));
    set<uintptr_t>(0x1efb504,address(config));
    // Physical IDs are from native629E50, named through native405B10.
    constexpr unsigned physical[]={23,25,26,24,12,13,11,14,15,17,16,18,20,19};
    for(unsigned row=0;row<14;++row) {
        set<unsigned>(config+0x60+row*4,28+row); set<unsigned>(config+0x60+(49+row)*4,128);
        set<unsigned>(config+0x60+(14+row)*4,physical[row]);
    }
    testPad=true; fixtureInputDevice(false);
    set<unsigned>(config+0x60+36*4,42);set<unsigned>(config+0x60+37*4,43);set<unsigned>(config+0x60+38*4,44);
    check(buttonGlyph(0x21)==L"Key 42" && buttonGlyph(0x22)==L"Key 43" && buttonGlyph(0x20)==L"Key 44","extra keyboard glyphs use native38/36/37 actions rather than stick fallback");
    CopiedControls ctb;prepareCopiedControlSnapshot(ctb);prepareCopiedControlNames();
    const auto ctbContext=copiedControlContext(ctb);
    check(ctbContext.glyphs[0x21]==L"Key 42" && ctbContext.glyphs[0x22]==L"Key 43","owned Help decoding preserves the displayed CTB keyboard keys");
    check(buttonGlyph(0x33)==L"Tab","connected controller cannot turn keyboard glyph speech into Y");
    constexpr const wchar_t* keyboard[]={L"E",L"Enter",L"Escape",L"Tab",L"Q",L"Z",L"R",L"C",L"Space",L"Backspace"};
    constexpr const wchar_t* controller[]={L"X button",L"A button",L"B button",L"Y button",L"LB",L"RB",L"LT",L"RT",L"Menu",L"View"};
    set<uint8_t>(0x2240000+0x186,2);set<uint8_t>(swimming_rva::DiveEnabled,1);
    glyphSwap=false;
    for(bool pad:{false,true}) {
        testPad=pad; fixtureInputDevice(pad);
        for(unsigned glyph=0x30;glyph<0x3a;++glyph)
            check(buttonGlyph(static_cast<uint8_t>(glyph))==(pad?controller[glyph-0x30]:keyboard[glyph-0x30]),
                "all ten native face/shoulder/start/select tokens use the correct configured action");
        check(buttonGlyph(0x41)==L"Up" && buttonGlyph(0x42)==L"Right" && buttonGlyph(0x44)==L"Down" &&
            buttonGlyph(0x48)==L"Left","native directional tokens use movement rows");
        for(const auto chord:std::array<std::pair<uint8_t,const wchar_t*>,4>{{
            {uint8_t{0x43},L"Up plus Right"},{uint8_t{0x46},L"Right plus Down"},
            {uint8_t{0x49},L"Up plus Left"},{uint8_t{0x4c},L"Down plus Left"}}})
            check(buttonGlyph(chord.first)==chord.second,"all native diagonal tokens name simultaneous directions");
        check(buttonGlyph(0x45)==L"Up or Down" && buttonGlyph(0x4a)==L"Right or Left" &&
            buttonGlyph(0x4f)==L"Up or Right or Down or Left","opposing/full-pad tokens preserve alternatives");
        archivedSwimmingPrompt("cdsp0000",107,stream);
        check(messagePage(0,address(stream)).prompt==(pad?L"Y button: Return to ship B button: Dive":L"Tab: Return to ship Escape: Dive"),
            "real post-salvage HUD names actual face actions, never neighboring shoulder rows");
        check(swimmingGoalInstruction()==(pad?L"Dive first. Hold B button and follow the route until the map changes":L"Dive first. Hold Escape and follow the route until the map changes"),
            "depth-goal guidance and original Dive glyph agree on native mask40");
        archivedSwimmingPrompt("cdsp0700",0,stream);
        check(messagePage(0,address(stream)).prompt==(pad?L"A button: Examine":L"Enter: Examine"),"original Examine prompt uses Confirm");
        archivedSwimmingPrompt("cdsp0700",1,stream);
        check(messagePage(0,address(stream)).prompt==(pad?L"Y button: Return to ship":L"Tab: Return to ship"),"other swimming return prompt agrees");
    }
    // Exact bytes of the reported Jecht practice page, through the production
    // copied message decoder and the ordinary original message producer.
    testPad=false;fixtureInputDevice(false);set<unsigned>(config+0x60+4*4,39);
    set<uintptr_t>(0x14676b0,address(window));set<uintptr_t>(0x1468a50,address(cursor));
    set<uint16_t>(window+4,0);set<uint16_t>(window+0x2c,2);set<uintptr_t>(window+0x94,address(stream));set<uint8_t>(cursor+1,0);
    constexpr const wchar_t* practiceNames[]={L"Up Right ",L"Left ",L"Right Down "};
    for(bool pad:{false,true}) {
        testPad=pad;fixtureInputDevice(pad);
        for(const auto practice:std::array<std::pair<const char*,unsigned>,2>{{{"swin0000",72},{"swin0200",97}}}) {
            for(unsigned step=0;step<3;++step) {
                archivedSwimmingPrompt(practice.first,practice.second+step,stream,"sw");
                prepareCopiedControlNames();TextPlan p;prepareTextPlan(p,"jecht-practice-regression");
                const auto root=planMessage(p,0,address(stream));check(p.valid && root,"real practice page capture");
                const auto& n=p.nodes[root-1];
                const auto page=decodeOwnedMessage(p.bytes.data()+n.offset,n.length,copiedControlContext(p.controls));
                const auto expected=std::wstring(practiceNames[step])+(pad?L"A button":L"C");
                check(page.prompt==expected && page.lines.size()==1 && page.lines[0]==expected,
                    "all original practice pages contain only configured controls, including review lines");
                resetMessages();event(0x4b8980,0);expect(expected.c_str(),"ordinary producer speaks only the required controls");
            }
            archivedSwimmingPrompt(practice.first,practice.second+3,stream,"sw");
            check(messagePage(0,address(stream)).prompt.find(L"Did you get the hang of it?")==0,
                "original explanatory tutorial remains complete");
        }
    }
    testPad=false;fixtureInputDevice(false);
    encoded(stream,"Press Up plus Right C at the same time!");
    set<uint8_t>(stream+19,0x45); // Original charset '+'; fixture ASCII helper omits punctuation.
    check(messagePage(0,address(stream)).prompt==L"Press Up plus Right+C at the same time!",
        "matching literal prose without original control glyphs is not shortened");
    for(const auto chord:std::array<std::pair<uint8_t,const wchar_t*>,7>{{
        {uint8_t{0x43},L"Up plus Right"},{uint8_t{0x46},L"Right plus Down"},
        {uint8_t{0x49},L"Up plus Left"},{uint8_t{0x4c},L"Down plus Left"},
        {uint8_t{0x45},L"Up or Down"},{uint8_t{0x4a},L"Right or Left"},
        {uint8_t{0x4f},L"Up or Right or Down or Left"}}}) {
        const uint8_t token[]={11,chord.first,0};
        check(decodeOwnedBytes(token,sizeof(token),OwnedTextMode::Terminated,OwnedTextContext{})[0]==chord.second,
            "fallback decoder has the same diagonal/alternative distinction");
    }
    set<unsigned>(config+0x60+4*4,32);
    testPad=true; fixtureInputDevice(true); resetMessages();
    set<uintptr_t>(0x14676b0,address(window)); set<uintptr_t>(0x1468a50,address(cursor));
    set<uint16_t>(window+0x2c,2); set<uint16_t>(window+0x10,0); set<uintptr_t>(window+0x94,address(stream)); set<uint8_t>(cursor+1,0);
    archivedSwimmingPrompt("cdsp0000",107,stream);
    const auto before=said.size(); event(0x4b8980,0); event(0x4b8980,0);
    check(said.size()==before+2,"separate installs of the same swimming prompt both announce, without text dedup");
    expect(L"Y button: Return to ship B button: Dive","field-control producer speaks the real corrected prompt");
    dispatchFieldControlsKey(VK_DOWN,true); expect(L"B button: Dive","review buffer retains corrected native controls");
    set<unsigned>(config+0x60+21*4,15); set<unsigned>(config+0x60+19*4,17); set<unsigned>(config+0x60+18*4,16);
    check(messagePage(0,address(stream)).prompt==L"LB: Return to ship LT: Dive" && swimmingGoalInstruction()==L"Dive first. Hold LT and follow the route until the map changes",
        "deliberate physical remaps apply immediately to prompt and Dive guidance");
    archivedSwimmingPrompt("cdsp0700",0,stream);
    check(messagePage(0,address(stream)).prompt==L"RB: Examine","Examine follows its actual remapped Confirm control");
    testPad=false; fixtureInputDevice(false); set<unsigned>(config+0x60+5*4,128); set<unsigned>(config+0x60+54*4,89);
    archivedSwimmingPrompt("cdsp0000",107,stream);
    check(messagePage(0,address(stream)).prompt==L"Tab: Return to ship Key 89: Dive","native keyboard alternate names the real configured Dive key");
    glyphSwap=true;
    check(buttonGlyph(0x31)==L"Key 89" && buttonGlyph(0x32)==L"Enter","regional atlas swap exchanges only Confirm and Cancel");
    glyphSwap=false; set<unsigned>(config+0x60+54*4,128);
    check(buttonGlyph(0x32)==L"unassigned control","missing keyboard binding never invents a physical button");
    testPad=true; fixtureInputDevice(true); set<unsigned>(config+0x60+19*4,128);
    check(buttonGlyph(0x32)==L"unassigned control","missing controller binding never falls back to an unrelated PS label");
    set<uintptr_t>(0x1efb504,0); check(buttonGlyph(0x33)==L"unassigned control","unavailable native config never guesses a button");
    testPad=false; fixtureInputDevice(false); resetMessages(); set<uint16_t>(window+0x2c,0); set<uintptr_t>(0x1efb504,oldConfig);
    memcpy(reinterpret_cast<void*>(address(0x5af0)),keyCode,7); memcpy(reinterpret_cast<void*>(address(0x5b10)),padCode,7);
    std::puts("PASS: original swimming/Examine glyphs, concise Jecht practice controls in both scripts/devices, explanatory/literal guards, remaps, alternates and regional swap.");
}
}
