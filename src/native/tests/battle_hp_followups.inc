// Exact native HP/MP playback, clamp and four display getters. Encounter,
// eligibility, actor lookup and accepted popup rendering are controlled seams.
namespace {
bool hpPlaybackAllowed=true;
unsigned hpNativeGlobals[2]{};
int hpSwitchCurrent=0,hpSwitchMaximum=0;
std::array<int,2> hpSwitchDigits{};
unsigned hpSwitchDigitCount=0;
float __cdecl hpDisplayScale() {return 0.0f;}
int __cdecl hpDisplayLanguage() {return 0;}
void __cdecl hpDisplayGraphic() {}
void __cdecl hpDisplayNumber(int value) {
    if(hpSwitchDigitCount<hpSwitchDigits.size())hpSwitchDigits[hpSwitchDigitCount]=value;
    ++hpSwitchDigitCount;
}
int __cdecl hpDisplayColour(int current,int maximum) {
    hpSwitchCurrent=current;hpSwitchMaximum=maximum;return 0;
}
uintptr_t __cdecl hpEncounter() {return 0;}
int __cdecl hpAllowed(unsigned) {return hpPlaybackAllowed?1:0;}
void __cdecl hpPopup(unsigned id,unsigned kind,int damage,unsigned,unsigned,unsigned,unsigned) {
    Event e;e.args[0]=id;e.args[1]=kind;e.args[2]=static_cast<uintptr_t>(damage);e.args[3]=e.args[2];e.result=0;
    for(auto h:listeners.at(0x39fa20))if(h.leave)h.leave(e);
}
void battleHpFixtures() {
    std::map<unsigned,uintptr_t> functions;
    const auto constants=ffx_fixture::load<1024>("offline-output/battle_hp_20261021/renderer-constants.bin",4299032417666201483ull);
    std::map<unsigned,unsigned> constantOffsets;
    std::ifstream constantMap(ffx_fixture::evidenceRoot()/"offline-output/battle_hp_20261021/renderer-constants-map.txt");
    check(constantMap.good(),"native Switch constant map");
    unsigned constantAddress=0,constantOffset=0;
    while(constantMap>>constantAddress>>constantOffset) {
        check(constantOffset+16<=sizeof(constants.bytes),"native Switch constant bound");
        constantOffsets[constantAddress]=constantOffset;
    }
    const auto load=[&](unsigned va,const auto& body) {
        const auto p=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
        check(p!=nullptr,"native HP replay allocation");memcpy(p,body.bytes,sizeof(body.bytes));functions[va]=reinterpret_cast<uintptr_t>(p);
    };
    load(0x78d330,ffx_fixture::load<291>("offline-output/battle_hp_20261021/0078d330.bin",15421666080208540904ull));
    load(0x78e2f0,ffx_fixture::load<263>("offline-output/battle_hp_20261021/0078e2f0.bin",8449707717570726986ull));
    load(0x78e400,ffx_fixture::load<94>("offline-output/battle_hp_20261021/0078e400.bin",9468633039791497172ull));
    load(0x79a0d0,ffx_fixture::load<26>("offline-output/battle_hp_20261021/0079a0d0.bin",3316147878827660020ull));
    load(0x79ade0,ffx_fixture::load<22>("offline-output/battle_hp_20261021/0079ade0.bin",770586423167526627ull));
    load(0x79ae00,ffx_fixture::load<22>("offline-output/battle_hp_20261021/0079ae00.bin",7756988651344995439ull));
    load(0x79ae60,ffx_fixture::load<22>("offline-output/battle_hp_20261021/0079ae60.bin",8710986050131545508ull));
    load(0x79ae80,ffx_fixture::load<22>("offline-output/battle_hp_20261021/0079ae80.bin",2064281304047716896ull));
    load(0x892c20,ffx_fixture::load<1071>("offline-output/battle_hp_20261021/00892c20.bin",16841974069963862874ull));
    const auto originalTable=ffx_fixture::load<16>("offline-output/battle_hp_20261021/renderer-jump-table.bin",10851866026507597024ull);
    std::array<uintptr_t,4> jumpTable{};
    for(unsigned i=0;i<jumpTable.size();++i) {
        unsigned label=0;memcpy(&label,originalTable.bytes+i*4,4);
        check(label>=0x892c20 && label<0x892c20+1071,"original Switch jump stays inside the qualified body");
        jumpTable[i]=functions.at(0x892c20)+label-0x892c20;
    }
    for(const auto& item:functions) {
        char filename[100]{};std::snprintf(filename,sizeof(filename),"offline-output/battle_hp_20261021/%08x-fixups.txt",item.first);
        std::ifstream fixes(ffx_fixture::evidenceRoot()/filename);check(fixes.good(),"native HP fixup input");
        unsigned offset=0,target=0,relative=0;
        while(fixes>>offset>>target>>relative) {
            uintptr_t value=0;
            if(functions.count(target))value=functions.at(target);
            else if(target==0x794030)value=reinterpret_cast<uintptr_t>(character);
            else if(target==0x781780)value=reinterpret_cast<uintptr_t>(hpEncounter);
            else if(target==0x78d460)value=reinterpret_cast<uintptr_t>(hpAllowed);
            else if(target==0x79fa20)value=reinterpret_cast<uintptr_t>(hpPopup);
            else if(target==0xc421dc)value=reinterpret_cast<uintptr_t>(&hpNativeGlobals[0]);
            else if(target==0x113338c)value=reinterpret_cast<uintptr_t>(&hpNativeGlobals[1]);
            else if(target==0x6449d0 || target==0x644990)value=reinterpret_cast<uintptr_t>(hpDisplayScale);
            else if(target==0x8ac2a0)value=reinterpret_cast<uintptr_t>(hpDisplayLanguage);
            else if(target==0x892880 || target==0x903bb0)value=reinterpret_cast<uintptr_t>(hpDisplayGraphic);
            else if(target==0x905890)value=reinterpret_cast<uintptr_t>(hpDisplayNumber);
            else if(target==0x78bfc0)value=reinterpret_cast<uintptr_t>(hpDisplayColour);
            else if(target==0x893050)value=reinterpret_cast<uintptr_t>(jumpTable.data());
            else if(constantOffsets.count(target))value=reinterpret_cast<uintptr_t>(constants.bytes)+constantOffsets.at(target);
            else check(false,"unmodeled native HP dependency");
            const auto word=relative?static_cast<unsigned>(value-item.second-offset-4):static_cast<unsigned>(value);
            memcpy(reinterpret_cast<void*>(item.second+offset),&word,4);
        }
    }
    constexpr unsigned id=4;const auto actor=character(id);
    std::array<uint8_t,0xf90> saved{};check(copy(actor,saved.data(),saved.size()),"Wakka fixture backup");
    const auto window=address(0xf3c910);
    const auto oldOwner=read<uint8_t>(window+8);const auto oldHp=setting(Setting::ShowMaximumHp),oldMp=setting(Setting::ShowMaximumMp);
    const auto oldType=read<uint16_t>(window+6);
    memset(reinterpret_cast<void*>(actor),0,saved.size());write<uint8_t>(actor+0x10,1);write<uint8_t>(actor+0xdc8,1);
    encoded(static_cast<unsigned>(actor-testBase)+0x540,"Wakka");set<uint8_t>(0xf3c918,id);
    write<int>(actor+0x594,4618);write<int>(actor+0x598,150);write<int>(actor+0x5d0,4618);write<int>(actor+0x5d4,150);
    write<int>(actor+0x6e4,9236);write<int>(actor+0x6e8,300);setSetting(Setting::ShowMaximumHp,1);setSetting(Setting::ShowMaximumMp,1);
    const auto h=[&](const wchar_t* expected) {keys['H'](false);drainSpeechDomainsForTest();expect(expected,"H uses committed native display values, never scratch forecasts");};
    h(L"Wakka HP 4618/4618 MP 150/150. Status: None");
    using Apply=void(__cdecl*)(unsigned,uintptr_t,int,unsigned,unsigned,unsigned,unsigned);
    const auto hp=reinterpret_cast<Apply>(functions.at(0x78e2f0)),mp=reinterpret_cast<Apply>(functions.at(0x78e400));
    const auto switchDraw=[&] {
        hpSwitchDigitCount=0;
        reinterpret_cast<void(__cdecl*)(unsigned,float,float)>(functions.at(0x892c20))(id,0.0f,0.0f);
        check(hpSwitchDigitCount==2,"original Switch renderer emits exactly HP and MP numbers");
        check(hpSwitchDigits[0]==hpSwitchCurrent,"Switch HP number and health-colour current input agree");
    };
    hp(id,actor,4289,0,0,1,0);drainSpeechDomainsForTest();expect(L"Wakka: 4289 damage","accepted native popup amount remains the game's value");
    h(L"Wakka HP 329/4618 MP 150/150. Status: None");
    hp(id,actor,-4618,0,0,1,0);drainSpeechDomainsForTest();
    h(L"Wakka HP 4618/4618 MP 150/150. Status: None");
    check(read<int>(actor+0x6e4)==9236,"original healing leaves the independent forecast untouched");
    hp(id,actor,2815,0,0,0,0);hp(id,actor,-2815,0,0,0,0);h(L"Wakka HP 4618/4618 MP 150/150. Status: None");
    mp(id,actor,140,0,0,2,0);drainSpeechDomainsForTest();h(L"Wakka HP 4618/4618 MP 10/150. Status: None");
    mp(id,actor,-999,0,0,0,0);h(L"Wakka HP 4618/4618 MP 150/150. Status: None");
    setSetting(Setting::ShowMaximumHp,0);setSetting(Setting::ShowMaximumMp,0);
    h(L"Wakka HP 4618 (100%) MP 150. Status: None");
    hp(id,actor,4289,0,0,0,0);h(L"Wakka HP 329 (7%) MP 150. Status: None");
    // The Switch maximum must agree with the original getter for every capacity,
    // including a real raised maximum; no fixed character capacity is guessed.
    setSetting(Setting::ShowMaximumHp,1);
    for(int maximum:{1,500,4618,6001,9236,9999,99999})for(int current:{0,1,maximum/2,maximum}) {
        write<int>(actor+0x594,maximum);write<int>(actor+0x5d0,current);
        const auto nativeCurrent=reinterpret_cast<int(__cdecl*)(unsigned)>(functions.at(0x79ade0))(id);
        const auto nativeMaximum=reinterpret_cast<int(__cdecl*)(unsigned)>(functions.at(0x79ae00))(id);
        switchDraw();
        check(hpSwitchCurrent==nativeCurrent && hpSwitchMaximum==nativeMaximum,"original Switch renderer consumes the native effective maximum");
        TextPlan p;prepareTextPlan(p,"native-switch-hp-proof");p.headline=planBattleSelectionVitals(p,id);check(commitTextPlan(p),"Switch native-vitals capture");drainSpeechDomainsForTest();
        const auto expected=L"HP "+std::to_wstring(nativeCurrent)+L"/"+std::to_wstring(nativeMaximum)+L", MP 150";
        check(said.back().first==expected,"Switch current and maximum exactly match original display getters");
    }
    // Use the game's actual capacity mutation, rather than invent a doubled
    // maximum in the reader. Its base and effective fields intentionally differ.
    write<int>(actor+0x59c,4618);write<int>(actor+0x5a0,150);
    write<uint8_t>(actor+0x640,0);write<uint16_t>(actor+0x6be,0);
    write<int>(actor+0x594,4618);write<int>(actor+0x5d0,1498);
    const auto capacity=reinterpret_cast<int(__cdecl*)(unsigned,uintptr_t,int)>(functions.at(0x78d330));
    capacity(id,actor,1);hp(id,actor,-4618,0,0,0,0);
    check(read<int>(actor+0x594)==9236 && read<int>(actor+0x59c)==4618,"original doubling changes effective maximum while retaining base maximum");
    setSetting(Setting::ShowMaximumHp,1);setSetting(Setting::ShowMaximumMp,1);
    h(L"Wakka HP 6116/9236 MP 150/150. Status: None");switchDraw();
    check(hpSwitchCurrent==6116 && hpSwitchMaximum==9236,"original Switch draws raised HP and uses raised maximum, not4618 base");
    check(battleSelectionVitals(id)==L"HP 6116/9236, MP 150","Switch accessibility follows original effective capacity above6000");
    setSetting(Setting::ShowMaximumHp,0);h(L"Wakka HP 6116 (66%) MP 150/150. Status: None");
    capacity(id,actor,0);setSetting(Setting::ShowMaximumHp,1);
    h(L"Wakka HP 4618/4618 MP 150/150. Status: None");
    memcpy(reinterpret_cast<void*>(actor),saved.data(),saved.size());set<uint8_t>(0xf3c918,oldOwner);set<uint16_t>(0xf3c916,oldType);
    setSetting(Setting::ShowMaximumHp,oldHp);setSetting(Setting::ShowMaximumMp,oldMp);resetBattle();
    for(const auto& item:functions)VirtualFree(reinterpret_cast<void*>(item.second),0,MEM_RELEASE);
    puts("PASS: original263/94-byte HP/MP playback,26-byte clamp,291-byte effective-capacity update,1071-byte Switch renderer and display getters; damage/healing/overheal, stale9236 forecast, native raised9236 maximum versus4618 base, preserved percentages and28 Switch states. Encounter/eligibility/graphics controlled.");
}
}
