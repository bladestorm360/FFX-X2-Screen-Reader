namespace {
std::string auronBinding;
uintptr_t __cdecl auronKeyName(unsigned id) {
    auronBinding="Key "+std::to_string(id); return reinterpret_cast<uintptr_t>(auronBinding.c_str());
}
uintptr_t __cdecl auronPadName(unsigned id) {
    auronBinding="Pad "+std::to_string(id); return reinterpret_cast<uintptr_t>(auronBinding.c_str());
}
void auronSequenceFixtures() {
    const auto previousConfig=read<uintptr_t>(address(0x1efb504));
    uint8_t keyCode[7]{},padCode[7]{};
    check(copy(address(0x5af0),keyCode,7) && copy(address(0x5b10),padCode,7),"save configured-name fixture stubs");
    stub(0x5af0,reinterpret_cast<void*>(auronKeyName)); stub(0x5b10,reinterpret_cast<void*>(auronPadName));
    prepareCopiedControlNames(); // Fixture-only replacement of otherwise immutable module tables.
    const auto config=address(0x2370000),source=address(0x2371000);
    set<uintptr_t>(0x1efb504,config);
    for(unsigned i=0;i<14;++i) {
        write<unsigned>(config+0x60+i*4,28+i); write<unsigned>(config+0x60+(i+49)*4,0);
        write<unsigned>(config+0x60+(i+14)*4,11+i);
    }
    const std::vector<unsigned> actions[]={{6,5,7,4,2,3,4},{1,2,0,3,8,10,5,4},
        {0,8,1,10,3,2,6},{4,3,10,2,8,6}};
    std::ifstream archive(ffx_fixture::evidenceRoot()/"offline-output/reader_reference_20261003_235502/block_00c0a000.bin",std::ios::binary);
    check(archive.good(),"retained Auron native table exists");
    set<uint8_t>(0xd2a8e0,1); scene=0; testForeground=true;
    setSetting(Setting::OverdriveAssist,0); setSetting(Setting::OverdriveCues,1);
    set<float>(0xf3f750,3.25f);
    for(unsigned sequence=0;sequence<4;++sequence) {
        uint16_t native[16]{}; archive.seekg(0xc86b70-0xc0a000+sequence*32);
        archive.read(reinterpret_cast<char*>(native),sizeof(native)); check(archive.gcount()==sizeof(native),"full native sequence bytes copied");
        check(copy(reinterpret_cast<uintptr_t>(native),reinterpret_cast<void*>(source),sizeof(native)),"native sequence fixture copy");
        const unsigned count=static_cast<unsigned>(actions[sequence].size());
        check(native[count]==0xffff,"native array terminator matches its independently expected length");
        for(bool pad:{false,true}) {
            testPad=pad; fixtureInputDevice(pad);
            std::vector<std::wstring> expected;
            for(auto action:actions[sequence]) expected.push_back((pad?L"Pad ":L"Key ")+std::to_wstring((pad?11:28)+action));
            const auto before=said.size();
            event(auron_rva::Initialize,0,[&] {
                set<uint16_t>(auron_rva::State,1); set<unsigned>(auron_rva::Count,count); set<uintptr_t>(auron_rva::Sequence,source);
            });
            check(said.size()==before+1,"each native initializer return emits one complete sequence");
            expect((L"Sequence: "+join(expected)).c_str(),"all four native arrays preserve configured keyboard/controller order");
            check(read<uint16_t>(address(auron_rva::State))==1 && read<float>(address(0xf3f750))==3.25f,"sequence speech does not start or alter the native timer/input phase");
        }
    }
    testPad=false; fixtureInputDevice(false);
    auto before=said.size(); event(auron_rva::Initialize,0); event(auron_rva::Initialize,0);
    check(said.size()==before+2,"two casts with the same sequence each speak, with no text dedup");
    set<unsigned>(auron_rva::Count,1); write<uint16_t>(source,0x1000); write<uint16_t>(source+2,0xffff);
    write<unsigned>(config+0x60,88); event(auron_rva::Initialize,0);
    expect(L"Sequence: Key 88","current native keyboard remap is read afresh");
    write<unsigned>(config+0x60,0); write<unsigned>(config+0x60+49*4,89); event(auron_rva::Initialize,0);
    expect(L"Sequence: Key 89","configured alternate key is used when primary is unassigned");
    testPad=true; fixtureInputDevice(true); write<unsigned>(config+0x60+14*4,75); event(auron_rva::Initialize,0);
    expect(L"Sequence: Pad 75","current native physical-controller remap is read afresh"); testPad=false; fixtureInputDevice(false);
    before=said.size(); set<uint16_t>(auron_rva::State,4); event(auron_rva::Initialize,0);
    event(0x490ee0,0); event(0x490ee0,0);
    check(said.size()==before,"per-frame input updates never repeat initialization speech"); set<uint16_t>(auron_rva::State,1);
    setSetting(Setting::OverdriveCues,0); event(auron_rva::Initialize,0); setSetting(Setting::OverdriveCues,1);
    setSetting(Setting::OverdriveAssist,1); event(auron_rva::Initialize,0); setSetting(Setting::OverdriveAssist,0);
    testForeground=false; event(auron_rva::Initialize,0); testForeground=true;
    set<uint8_t>(0xd2a8e0,0); event(auron_rva::Initialize,0); set<uint8_t>(0xd2a8e0,1);
    scene=4; event(auron_rva::Initialize,0); scene=0;
    set<unsigned>(auron_rva::Count,0); event(auron_rva::Initialize,0);
    set<unsigned>(auron_rva::Count,16); event(auron_rva::Initialize,0); set<unsigned>(auron_rva::Count,1);
    set<uintptr_t>(auron_rva::Sequence,1); event(auron_rva::Initialize,0); set<uintptr_t>(auron_rva::Sequence,source);
    write<uint16_t>(source+2,0); event(auron_rva::Initialize,0); write<uint16_t>(source+2,0xffff);
    write<uint16_t>(source,0x2001); event(auron_rva::Initialize,0); write<uint16_t>(source,0x1000);
    write<unsigned>(config+0x60+49*4,0); event(auron_rva::Initialize,0);
    check(said.size()==before,"toggles, focus/battle/scene, malformed arrays and unavailable bindings suppress Auron speech");
    memcpy(reinterpret_cast<void*>(address(0x5af0)),keyCode,7); memcpy(reinterpret_cast<void*>(address(0x5b10)),padCode,7);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(0x5af0)),7);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(0x5b10)),7);
    set<uintptr_t>(0x1efb504,previousConfig); set<uint16_t>(auron_rva::State,0);
    set<unsigned>(auron_rva::Count,0); set<uintptr_t>(auron_rva::Sequence,0); testPad=false; fixtureInputDevice(false);
    puts("PASS: actual Auron arrays, configured keyboard/controller sequence order, alternate bindings, per-cast events, unchanged timer and context/data guards.");
}
}
