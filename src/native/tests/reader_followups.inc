// Follow-up regression cases using the production listeners and native getters.
namespace {
bool followupLabels=false;
unsigned equipSlot=0;
uint8_t __cdecl focusedCharacter() { return static_cast<uint8_t>(read<int>(address(0x147150c))); }
unsigned __cdecl focusedSlot() { return equipSlot; }
int __cdecl partyCount() { return 2; }
uint8_t __cdecl orderedParty(unsigned row) { return static_cast<uint8_t>(row); }
unsigned __cdecl hp(unsigned id) { return read<unsigned>(savedCharacter(id)+0x1c); }
unsigned __cdecl mp(unsigned id) { return read<unsigned>(savedCharacter(id)+0x20); }
unsigned __cdecl maxHp(unsigned id) { return read<unsigned>(savedCharacter(id)+0x24); }
unsigned __cdecl maxMp(unsigned id) { return read<unsigned>(savedCharacter(id)+0x28); }
unsigned __cdecl weapon(unsigned) { return 0; }
uint8_t rikkuArmor=197;
uint8_t __cdecl armor(unsigned character) { return character==6?rikkuArmor:1; }
uint8_t __cdecl slevel(unsigned id) { return read<uint8_t>(savedCharacter(id)+0x3b); }
unsigned __cdecl nextAp(unsigned) { return 500; }
unsigned __cdecl earnedAp(unsigned) { return 349; }
uintptr_t __cdecl descriptor() { return address(0x2105100); }
uintptr_t __cdecl controls() { return address(0x2105120); }
uintptr_t __cdecl nameText(unsigned) { return address(0x2105140); }
uintptr_t __cdecl slotName(unsigned id) { return address(id==197?0x2400800:id?0x2105160:0x21000c0); }
uintptr_t __cdecl menuLabels(int space,unsigned id,unsigned lang) {
    if(space==11 && id==0x17) return 0; // native None label can be empty
    if(space==11 && id==5) return address(0x2105180);
    if(space==11 && id==6) return address(0x21051a0);
    if(space==11 && id>=100 && id<108) return address(0x2105200+(id-100)*32);
    if(space==1 && id==0x1035) return address(0x21051c0);
    return menu(space,id,lang);
}
void followupStubs() {
    stub(0x4a9810,focusedCharacter); stub(0x4cfc00,focusedSlot);
    stub(0x4a9ae0,partyCount); stub(0x4a9ba0,partyCount);
    stub(0x4a9a80,orderedParty); stub(0x4a9b70,orderedParty);
    stub(0x4a9870,hp); stub(0x4a9920,mp); stub(0x4a9940,maxHp); stub(0x4a9960,maxMp);
    stub(0x4a9c20,weapon); stub(0x4a97d0,armor); stub(0x4a9b30,slevel);
    stub(0x384f50,nextAp); stub(0x385370,earnedAp);
    stub(0x4c4f60,descriptor); stub(0x4c4fc0,controls); stub(0x4beda0,nameText);
    stub(0x3909c0,record);
    set<uint16_t>(0x2105102,10); set<uint16_t>(0x2105120,1);
    encoded(0x2105140,"Finish"); encoded(0x2105160,"Yellow Shield");
    encoded(0x2105180,"Weapon"); encoded(0x21051a0,"Armor"); encoded(0x21051c0,"Stoic");
    const char* labels[]={"Strength","Defense","Magic","Magic Defense","Agility","Luck","Evasion","Accuracy"};
    for(unsigned i=0;i<8;++i) {
        encoded(0x2105200+i*32,labels[i]); set<int>(0x85a790+i*8,100+i); set<int>(0x85a794+i*8,0x2f+i);
    }
}
void followupFixtures() {
    setSetting(Setting::ShowMaximumHp,1);
    setSetting(Setting::AutoDetail,0); resetSystemMenus(); resetSphereGrid();
    std::memset(reinterpret_cast<void*>(address(win)),0,0x100);
    scene=3; set<int>(win+0x28,1); set<int16_t>(win+0x48,0); set<int>(win+0x58,0); set<int>(win+0x5c,0);
    set<uint8_t>(0x146a351,1);
    set<unsigned>(0xd3205c+0x1c,517); set<unsigned>(0xd3205c+0x24,517);
    set<unsigned>(0xd3205c+0x20,12); set<unsigned>(0xd3205c+0x28,12);
    set<unsigned>(0xd3205c+0x94+0x1c,360); set<unsigned>(0xd3205c+0x94+0x24,360);
    set<unsigned>(0xd3205c+0x94+0x20,20); set<unsigned>(0xd3205c+0x94+0x28,40);
    event(0x4c7400,address(win),[]{set<int>(win+0x28,4);}); expect(L"Tidus: HP 517/517","initial potion target reads only displayed HP");
    auto n=said.size(); event(0x4c7400,address(win)); check(said.size()==n,"idle target silent");
    event(0x4c7400,address(win),[]{set<int16_t>(win+0x48,1);}); expect(L"Yuna: HP 360/360","target follows native party ordering");
    event(0x4c7400,address(win),[]{set<int>(win+0x5c,1);}); expect(L"Yuna: MP 20/40","ether shows only MP");
    event(0x4c7400,address(win),[]{set<uint8_t>(0x146a351,0);}); expect(L"All party members. Tidus: MP 12/12. Yuna: MP 20/40","all targets includes actual party and displayed resource");
    event(0x4c7400,address(win),[]{set<int>(win+0x28,9);});
    event(0x4c7400,address(win),[]{set<int>(win+0x28,4);}); expect(L"All party members. Tidus: MP 12/12. Yuna: MP 20/40","target reopens at initial selection");
    setSetting(Setting::ShowMaximumHp,0);
    stub(0x38fd40,menuLabels); stub(0x3abe10,slotName);
    scene=4; set<int>(win+0x1c,9); set<int>(0x147150c,0); equipSlot=0;
    event(0x4ceff0,address(win),[]{set<int>(win+0x1c,10);}); expect(L"Weapon: Sonic Steel","initial equipment slot");
    event(0x4ceff0,address(win),[]{equipSlot=1;}); expect(L"Armor: Yellow Shield","armor slot label and equipped name");
    n=said.size(); event(0x4ceff0,address(win)); check(said.size()==n,"equipment slot idle silence");
    encoded(0x2400800,"Rikku Armor"); rikkuArmor=197;
    event(0x4ceff0,address(win),[]{set<int>(0x147150c,6);});
    expect(L"Armor: Rikku Armor","Rikku reads her actual byte equipment index, not Tidus's weapon or blank name");
    event(0x4ceff0,address(win),[]{set<int>(0x147150c,0);});
    rikkuArmor=255; event(0x4ceff0,address(win),[]{set<int>(0x147150c,6);});
    expect(L"Armor: Nothing equipped","native FF empty slot remains explicit when its rendered None label is empty");
    rikkuArmor=197; set<int>(0x147150c,0);
    scene=6; set<int>(win+0x1c,4); set<uint8_t>(0xd3205c+0x3b,30); set<uint8_t>(0xd3205c+0x38,2);
    event(0x4d1dd0,address(win)); expect(L"Tidus","status opens at character");
    check(contexts.at(VK_DOWN)(),"arrows captured only in active status");
    keys[VK_DOWN](false); expect(L"HP: 517/517","status arrow buffer HP");
    keys[VK_DOWN](false); expect(L"MP: 12/12","status arrow buffer MP");
    keys[VK_DOWN](false); expect(L"Sphere Level: 30","displayed sphere levels");
    keys[VK_RIGHT](false); expect(L"Next AP: 151","native next AP subtraction");
    keys[VK_RIGHT](false); expect(L"Overdrive mode: Stoic","mode without hidden bar numbers");
    keys[VK_RIGHT](false); expect(L"Weapon: Sonic Steel","status weapon"); keys[VK_RIGHT](false); expect(L"Armor: Yellow Shield","status armor");
    for(unsigned i=0;i<8;++i) keys[VK_DOWN](false);
    drainSpeechDomainsForTest();
    check(said.back().first.find(L"Accuracy:")==0,"native eight-stat ordering");
    event(0x4d1dd0,address(win),[]{set<int>(win+0x1c,6);});
    set<int>(win+0x60,0); set<int>(win+0x64,0);
    Event draw; draw.rva=0x4d2760; draw.args[0]=address(win); draw.epoch=generation();
    listeners[draw.rva][0].enter(draw);
    Event ability; ability.rva=0x4f4f10; ability.parentRva=draw.rva; ability.args[0]=1;
    listeners[ability.rva][0].enter(ability); ability.args[0]=2; listeners[ability.rva][0].enter(ability);
    listeners[draw.rva][0].leave(draw);
    for(int i=0;i<5;++i) keys[VK_DOWN](false); expect(L"Potion","Auto-Abilities buffer includes actual rendered ability");
    keys[VK_DOWN](false); expect(L"Ether","second rendered ability");
    keys['I'](false);expect(L"No native description is available for this selection.","regular Status cannot speak undisplayed ability descriptions");
    keys[VK_DOWN](false); expect(L"Tidus. Auto-Abilities","Auto-Abilities wraps without hidden stats, gear or Overdrive");
    event(0x4d1dd0,address(win),[]{set<int>(win+0x1c,8);});set<int>(win+0x60,1);
    draw={};draw.rva=0x4d2760;draw.epoch=generation();draw.args[0]=address(win);
    listeners[draw.rva][0].enter(draw);listeners[draw.rva][0].leave(draw);drainSpeechDomainsForTest();
    expect(L"Tidus. Skills. No learned abilities","empty regular Status page explicitly identifies character and active category");
    keys[VK_UP](false);expect(L"No learned abilities","empty character page has a readable buffer row");
    event(0x4d1dd0,address(win),[]{set<int>(win+0x1c,0x13);}); check(!contexts.at(VK_DOWN)(),"status closure restores game arrows");
    n=said.size(); keys[VK_DOWN](false); check(said.size()==n,"closed status cannot speak stale buffer");
    // The Grid has its own pool and visibility mutations. 4D1DD0 is NEVER
    // involved in this actual path; the former fixture incorrectly used it.
    scene=22; set<uint8_t>(0x1685f70,1);
    constexpr unsigned grid=0x2320000,status=pool+0x340;
    set<uintptr_t>(0x1f05834,address(grid)); set<uint8_t>(grid+0x115bc,0);
    encoded(0x21055e0,"Status"); set<uintptr_t>(pool+6*0x340+0x40,address(0x21055e0));
    set<int16_t>(status+0x1e,10); set<uint8_t>(status+0x23,0);
    const char* stats[]={"HP","MP","Strength","Magic","Defense","Magic Defense","Agility","Evasion","Luck","Accuracy"};
    for(unsigned i=0;i<10;++i) {
        encoded(0x2105600+i*32,stats[i]);
        set<uintptr_t>(status+0x40+i*12,address(0x2105600+i*32));
        set<uint8_t>(status+0x44+i*12,0); set<unsigned>(status+0x48+i*12,i);
    }
    const unsigned savedStats[]={0x2f,0x33,0x30,0x34,0x31,0x35,0x32,0x36};
    for(unsigned i=0;i<8;++i) set<uint8_t>(static_cast<unsigned>(savedCharacter(0)-testBase)+savedStats[i],static_cast<uint8_t>(i+1));
    const auto gridStart=said.size();
    event(0x65a270,address(status),[=]{set<uint8_t>(status+0x23,1);});
    check(said.size()==gridStart+1 && said.back().first==L"Tidus. Attributes",
        "Grid Status AutoDetail Off speaks only its character and native title on entry");
    check(readSelectionDetail() && said.back().first.find(L"Tidus. Attributes. HP: 517. MP: 12. Strength:")==0,
        "I reads the copied full displayed Grid Status buffer with AutoDetail Off");
    check(contexts.at(VK_DOWN)(),"Grid Status captures review arrows");
    keys[VK_DOWN](false); expect(L"HP: 517","Grid HP has no hidden current/max pair");
    set<int>(win+0x60,0); set<int>(win+0x64,0); event(0x4d2760,address(win));
    keys[VK_UP](false); expect(L"Tidus. Attributes","Grid Status header retains its own row");
    keys[VK_DOWN](false); expect(L"HP: 517","unrelated regular Status drawing cannot replace Grid maximum-only data");
    n=said.size(); event(0x65a270,address(status)); check(said.size()==n,"native fade updates do not repeat page speech");
    event(0x658ec0,0,[=]{set<uint8_t>(grid+0x115bc,1);});
    check(said.back().first==L"Yuna. Attributes","Grid Status follows its own character and native title instead of regular menu focus");
    n=said.size(); event(0x4a9810,0); check(said.size()==n,"ordinary Grid character/node producer yields to the actual Status page owner");
    event(0x659800,address(status),[=]{set<uint8_t>(status+0x23,0);});
    check(!contexts.at(VK_DOWN)(),"closing actual Grid Status restores grid controls");
    constexpr unsigned abilities=pool+3*0x340;
    set<uint8_t>(grid+0x115bc,0); set<int16_t>(abilities+0x1e,3); set<uint8_t>(abilities+0x23,0);
    const unsigned commandIds[]={0x3001,0x3002,0x3003};
    for(unsigned i=0;i<3;++i) {set<unsigned>(abilities+0x40+i*12,commandIds[i]);set<uint8_t>(abilities+0x44+i*12,i==2);}
    event(0x65a270,address(abilities),[=]{set<uint8_t>(abilities+0x23,1);});
    expect(L"Tidus. Skills","Grid ability page AutoDetail Off speaks only its character and native title");
    check(readSelectionDetail() && said.back().first==L"Tidus. Skills. Potion. Ether",
        "I reads only the copied visible ability rows, excluding the hidden row");
    keys[VK_DOWN](false); expect(L"Potion","Grid abilities use the shared arrow review buffer");
    keys['I'](false);expect(L"No native description is available for this selection.","Grid status buffer cannot substitute undisplayed skill help for the node's visible help");
    event(0x659800,address(abilities),[=]{set<uint8_t>(abilities+0x23,0);});
    // Exercise every native status slot. Pages with no visible rows still announce
    // the character/title. AutoDetail adds the copied displayed rows after them.
    setSetting(Setting::AutoDetail,1); set<uint8_t>(grid+0x115bc,0);
    set<int16_t>(status+0x1e,10); set<uint8_t>(status+0x23,0);
    n=said.size(); event(0x65a270,address(status),[=]{set<uint8_t>(status+0x23,1);});
    check(said.size()==n+2 && said[n].first==L"Tidus. Attributes" &&
        said[n+1].first==L"HP: 517. MP: 12. Strength: 1. Magic: 2. Defense: 3. Magic Defense: 4. Agility: 5. Evasion: 6. Luck: 7. Accuracy: 8",
        "Grid Status AutoDetail On reads details after its headline");
    event(0x659800,address(status),[=]{set<uint8_t>(status+0x23,0);});
    setSetting(Setting::AutoDetail,0);
    for(unsigned pageIndex:{2u,4u,5u}) {
        const unsigned pageWindow=pool+pageIndex*0x340;
        set<int16_t>(pageWindow+0x1e,2); set<uint8_t>(pageWindow+0x23,0);
        for(unsigned row=0;row<2;++row) {set<unsigned>(pageWindow+0x40+row*12,0x3001+row);set<uint8_t>(pageWindow+0x44+row*12,0);}
        event(0x65a270,address(pageWindow),[=]{set<uint8_t>(pageWindow+0x23,1);});
        const std::wstring title=pageIndex==2?L"Tidus. Special":pageIndex==4?L"Tidus. White magic":L"Tidus. Black magic";
        expect(title.c_str(),"each Grid Status command page identifies its displayed category");
        check(readSelectionDetail() && said.back().first==title+L". Potion. Ether","I copies all displayed rows on every native Status command page");
        keys[VK_DOWN](false); expect(L"Potion","arrows access every populated native page");
        event(0x659800,address(pageWindow),[=]{set<uint8_t>(pageWindow+0x23,0);});
    }
    set<int16_t>(abilities+0x1e,3); set<uint8_t>(abilities+0x23,0);
    for(unsigned i=0;i<3;++i) set<uint8_t>(abilities+0x44+i*12,1);
    event(0x65a270,address(abilities),[=]{set<uint8_t>(abilities+0x23,1);});
    expect(L"Tidus. Skills. No learned abilities in this category","empty learned-ability page explicitly identifies its blank content");
    keys[VK_DOWN](false); expect(L"Tidus. Skills. No learned abilities in this category","blank page review cannot expose unlearned command names");
    event(0x659800,address(abilities),[=]{set<uint8_t>(abilities+0x23,0);});
    set<uint8_t>(0x1685f70,0);
    // Grid exit uses slot 10 for its prompt, and slot 11 for its actual choices.
    scene=0; set<uint8_t>(0x1685f70,1); const unsigned choices=pool+11*0x340;
    set<int16_t>(pool+0x2700,11); set<uintptr_t>(pool+0x2704,address(0x656060));
    set<uint8_t>(choices+0x23,1); set<int16_t>(choices+0x1e,2); set<int16_t>(choices+0x18,0);
    set<uintptr_t>(pool+10*0x340+0x40,address(0x2100180));
    set<uintptr_t>(choices+0x40,address(0x21001a0)); set<uintptr_t>(choices+0x4c,address(0x21001c0));
    event(0x6583e0,0); expect(L"Yes","grid exit initially focused option"); check(!said.back().second && said[said.size()-2].first==L"Proceed","grid prompt survives queued initial option");
    n=said.size(); event(0x6583e0,0); check(said.size()==n,"grid prompt idle silence");
    event(0x6583e0,0,[=]{set<int16_t>(choices+0x18,1);}); expect(L"No","grid exit navigation"); check(said.back().second,"moved choice interrupts");
    event(0x6583e0,0,[=]{set<int16_t>(pool+0x2700,-1);});
    event(0x6583e0,0,[=]{set<int16_t>(pool+0x2700,11);set<int16_t>(choices+0x18,0);}); expect(L"Yes","grid exit reopens");
    // Naming cells are copied; F11 can reuse its repeat action contextually.
    resetNaming(); set<int>(win+0x28,3); set<uint8_t>(win+0x41,0);
    for(auto off:{0x146a21c,0x146a22c,0x146a230,0x146a218}) set<int>(off,0);
    set<int>(0x146a228,6);
    encoded(0x146a240,"A"); encoded(0x146a244," "); encoded(0x146a248,"B"); set<unsigned>(0x146a24c,0);
    event(0x4c3560,address(win),[]{set<int>(win+0x28,4);});
    draw={}; draw.rva=0x4c4530; draw.epoch=generation(); listeners[draw.rva][0].enter(draw);
    Event line; line.parentRva=draw.rva; line.epoch=generation();
    for(int group=0;group<2;++group) {
        line.rva=0x4beda0; listeners[line.rva][0].leave(line);
        for(int col=0;col<5;++col) {
            char glyph[2]={static_cast<char>('A'+group*5+col),0}; encoded(0x2105400,glyph);
            line.rva=0x505ab0; line.args[0]=address(0x2105400); listeners[line.rva][0].enter(line);
        }
    }
    listeners[draw.rva][0].leave(draw); expect(L"G","naming speaks highlighted glyph from native row order");
    check(readEnteredName(),"naming context available"); expect(L"Name: A B","entered name preserves spaces");
    const int previousPunctuationSetting=setting(Setting::SpeakPunctuationNames);
    setSetting(Setting::SpeakPunctuationNames,0);
    event(0x4c3560,address(win),[]{set<int>(0x146a228,0);}); encoded(0x2105400,"!");
    draw={}; draw.rva=0x4c4530; draw.epoch=generation(); draw.args[0]=address(win);
    listeners[draw.rva][0].enter(draw);
    line.parentRva=draw.rva; line.epoch=generation(); line.rva=0x4beda0;
    listeners[line.rva][0].leave(line);
    line.rva=0x505ab0; line.args[0]=address(0x2105400); listeners[line.rva][0].enter(line);
    listeners[draw.rva][0].leave(draw); expect(L"!","punctuation glyph remains raw with the default Off setting");
    keys['N'](false); expect(L"Name: A B","N reads the copied entered name in the naming context");
    setSetting(Setting::SpeakPunctuationNames,1);
    event(0x4c3560,address(win),[]{set<int>(0x146a228,1);});
    event(0x4c3560,address(win),[]{set<int>(0x146a228,0);});
    draw={}; draw.rva=0x4c4530; draw.epoch=generation(); draw.args[0]=address(win);
    listeners[draw.rva][0].enter(draw);
    line.parentRva=draw.rva; line.epoch=generation(); line.rva=0x4beda0;
    listeners[line.rva][0].leave(line);
    line.rva=0x505ab0; line.args[0]=address(0x2105400); listeners[line.rva][0].enter(line);
    listeners[draw.rva][0].leave(draw); expect(L"exclamation mark","punctuation names are spoken when enabled");
    encoded(0x146a240,"Z"); readEnteredName(); expect(L"Name: A B","shortcut reads copied name without game pointers");
    event(0x4c3560,address(win),[]{set<int>(0x146a230,1);}); expect(L"Finish","native highlighted naming control");
    readEnteredName(); expect(L"Name: Z B","native input refreshes copied edited name");
    const auto oldAirshipRoom=read<int>(address(0xf30804));
    set<int>(0xf30804,382);
    event(0x4c3560,address(win),[]{set<int>(0x146a218,-1);});
    keys['N'](false);expect(L"Password: Z B","N labels airship password cells separately from character naming");
    set<unsigned>(0x146a240,0);event(0x4c3560,address(win));
    keys['N'](false);expect(L"Password is empty","empty airship password has the correct readback subject");
    encoded(0x146a240,"Z");event(0x4c3560,address(win),[]{set<int>(0x146a218,0);});
    keys['N'](false);expect(L"Name: Z B","ordinary character naming remains Name even in an airship room");
    set<int>(0xf30804,oldAirshipRoom);
    event(0x4c3560,address(win),[]{set<int>(win+0x28,9);}); check(!readEnteredName(),"naming closes and restores normal repeat context");
    n=said.size(); keys['N'](false);drainSpeechDomainsForTest();
    check(said.size()>n && said.back().first.find(L"Name:")!=0,"N falls through to existing detail help after naming closes");
    setSetting(Setting::SpeakPunctuationNames,previousPunctuationSetting);
    puts("PASS: field targets, all-party HP/MP, slots, visible-page status arrows, grid exit and copied naming glyph/name events.");
}
}
