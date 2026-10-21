namespace {
uintptr_t __cdecl statusAction(unsigned) { return address(0x2333300); }
uintptr_t animationImage(unsigned number) {
    char path[200]{};
    std::snprintf(path,sizeof(path),"magic_%04u.dll",number);
    const auto evidenceFile=ffx_fixture::evidenceRoot()/"offline-output/status_animations_20261009_171247_874998"/path;
    std::ifstream stream(evidenceFile,std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(stream)),{});
    check(bytes.size()>4096,"retained approved animation fixture exists");
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(bytes.data());
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(bytes.data()+dos->e_lfanew);
    const auto base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(base!=0,"offline animation image allocation");
    std::memcpy(reinterpret_cast<void*>(base),bytes.data(),nt->OptionalHeader.SizeOfHeaders);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)
        if(sections[i].SizeOfRawData) std::memcpy(reinterpret_cast<void*>(base+sections[i].VirtualAddress),bytes.data()+sections[i].PointerToRawData,sections[i].SizeOfRawData);
    const auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    for(unsigned offset=0;offset<directory.Size;) {
        const auto block=reinterpret_cast<const IMAGE_BASE_RELOCATION*>(base+directory.VirtualAddress+offset);
        check(block->SizeOfBlock>=sizeof(*block),"offline PE relocation block");
        const auto entries=reinterpret_cast<const WORD*>(block+1);
        for(unsigned i=0;i<(block->SizeOfBlock-sizeof(*block))/2;++i) if((entries[i]>>12)==IMAGE_REL_BASED_HIGHLOW) {
            const auto at=base+block->VirtualAddress+(entries[i]&0xfff);
            write<uint32_t>(at,read<uint32_t>(at)+static_cast<uint32_t>(base-nt->OptionalHeader.ImageBase));
        }
        offset+=block->SizeOfBlock;
    }
    return base; // Data only: no DLL entry point or callback is executed.
}
void statusAnimationFixtures() {
    constexpr unsigned numbers[]={3,6,4,5,7,8};
    constexpr const wchar_t* names[]={L"Cheer",L"Aim",L"Focus",L"Reflex",L"Luck",L"Jinx"};
    stub(0x3b09c0,reinterpret_cast<void*>(statusAction)); set<uint8_t>(0x2333302,42);
    set<unsigned>(0x2333200+12,42);
    const auto& calc=listeners[0x38e680][0]; const auto& playback=listeners[0x38f0b0][0];
    const auto& visual=listeners[0x395a30][0];
    for(unsigned buff=0;buff<6;++buff) {
        resetBattle(); const auto base=animationImage(numbers[buff]); setStatusAnimationFixture(base);
        const auto& layout=statusAnimationLayouts[buff];
        write<uintptr_t>(base+layout.api,address(0x2333400));
        set<uintptr_t>(0x2333400+0x3ec,address(0x395a30));
        write<unsigned>(base+layout.owner,0x128000);
        const auto start=said.size();
        for(unsigned id:{0u,4u,2u}) {
            const auto actor=0x2120000+id*0xf90,buffer=actor+0x774;
            set<uint8_t>(actor+0xdc8,1); set<uint8_t>(actor+0x65e+buff,0);
            set<uint8_t>(buffer,0); set<uint8_t>(buffer+1,0); set<uint8_t>(buffer+2,255); set<uint8_t>(buffer+3,255);
            Event e; e.rva=0x38e680; e.args[2]=id; e.args[3]=address(actor); e.args[6]=address(buffer+0x18);
            e.args[8]=address(0x2333200);
            calc.enter(e); set<uint8_t>(actor+0x65e+buff,1); calc.leave(e); drainSpeechDomainsForTest();
            // 7893A0 assigns source/serial after 789800 has calculated hits.
            set<uint8_t>(buffer+1,1); set<uint8_t>(buffer+2,0); set<uint8_t>(buffer+3,42);
        }
        check(said.size()==start,"all-party calculations stay silent before visual creation");
        for(unsigned id:{0u,4u,2u}) {
            const auto actor=0x2120000+id*0xf90,buffer=actor+0x774;
            Event cue; cue.rva=0x395a30; cue.args[0]=id; cue.caller=base+layout.cue;
            const auto n=said.size();
            cue.caller+=1; visual.enter(cue); cue.caller-=1;
            testStatusOwner=false; visual.enter(cue); testStatusOwner=true;
            write<unsigned>(base+layout.owner,0x128001); visual.enter(cue); write<unsigned>(base+layout.owner,0x128000);
            set<uint8_t>(0x2333302,43); visual.enter(cue); set<uint8_t>(0x2333302,42);
            set<uint8_t>(buffer+3,43); visual.enter(cue); set<uint8_t>(buffer+3,42);
            set<uint8_t>(buffer+2,1); visual.enter(cue); set<uint8_t>(buffer+2,0);
            set<uintptr_t>(0x2333400+0x3ec,0); visual.enter(cue); set<uintptr_t>(0x2333400+0x3ec,address(0x395a30));
            const auto byte=read<uint8_t>(base+layout.window); write<uint8_t>(base+layout.window,byte^1);
            visual.enter(cue); write<uint8_t>(base+layout.window,byte);
            drainSpeechDomainsForTest();
            check(said.size()==n,"wrong caller/thread/owner/sequence/API/code cannot announce a predicted buff");
            visual.enter(cue); drainSpeechDomainsForTest(); check(said.size()==n+1,"each actual target creation announces separately during the animation");
            const auto expected=battleName(id)+L": "+names[buff]+L" 1";
            expect(expected.c_str(),"actual copied stack count matches this target's visual cue");
            check(!said.back().second,"animation status queues alongside damage");
            visual.enter(cue); drainSpeechDomainsForTest(); check(said.size()==n+1,"one native packet owns one buff application");
            Event apply; apply.rva=0x38f0b0; apply.args[1]=42; apply.args[2]=id;
            playback.enter(apply); set<uint8_t>(buffer,1); playback.leave(apply); drainSpeechDomainsForTest();
            check(said.size()==n+1,"completion retires the packet without repeating its already-spoken buff");
        }
        // A new identical result remains speakable, independent of its text.
        constexpr unsigned actor=0x2120000,buffer=actor+0x774;
        set<uint8_t>(buffer,0); set<uint8_t>(actor+0x65e+buff,0);
        Event e; e.rva=0x38e680; e.args[3]=address(actor); e.args[6]=address(buffer+0x18);
        e.args[8]=address(0x2333200);
        calc.enter(e); set<uint8_t>(actor+0x65e+buff,1); calc.leave(e); drainSpeechDomainsForTest();
        Event cue; cue.args[0]=0; cue.caller=base+layout.cue; auto n=said.size(); visual.enter(cue); drainSpeechDomainsForTest();
        check(said.size()==n+1,"identical status text from a new effect is preserved");
        resetBattle(); n=said.size(); visual.enter(cue); drainSpeechDomainsForTest();
        check(said.size()==n,"battle cancellation invalidates unplayed target cues");
        // Unrecognized visuals leave the native playback fallback intact.
        set<uint8_t>(actor+0x65e+buff,0); calc.enter(e); set<uint8_t>(actor+0x65e+buff,1); calc.leave(e); drainSpeechDomainsForTest();
        cue.caller+=1; visual.enter(cue); drainSpeechDomainsForTest();
        Event apply; apply.rva=0x38f0b0; apply.args[1]=42;
        playback.enter(apply); set<uint8_t>(buffer,1); playback.leave(apply); drainSpeechDomainsForTest();
        check(said.size()==n+1,"unsupported animation keeps actual completion speech");
        resetBattle(); set<uint8_t>(actor+0x65e+buff,5); set<uint8_t>(buffer,0);
        calc.enter(e); calc.leave(e); drainSpeechDomainsForTest(); cue.caller-=1; n=said.size(); visual.enter(cue); drainSpeechDomainsForTest();
        check(said.size()==n,"capped buff has no invented application");
        for(unsigned id:{0u,4u,2u}) set<uint8_t>(0x2120000+id*0xf90+0x65e+buff,0);
        setStatusAnimationFixture(0); VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);
    }
    resetBattle();
    std::puts("PASS: all six real stack-buff DLL target cues, three separate party applications, relocation/profile ownership, packet fallback, repeated/capped/canceled effects.");
}
}
