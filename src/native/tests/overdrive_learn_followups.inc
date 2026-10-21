// Replay the original104-byte learned-mode initializer, retaining its actual
// namespace7 writes and banner0 submission. Text getters/registration and the
// renderer are controlled boundaries; production message expansion is exercised.
namespace {
uintptr_t learnedStrings[2]{};
void __cdecl learnedClear(unsigned space,unsigned index,unsigned kind) {
    check(space==7 && index<2 && !kind,"original learned-mode string namespace/type");learnedStrings[index]=0;
}
void __cdecl learnedRegister(unsigned space,unsigned index,uintptr_t text) {
    check(space==7 && index<2,"original learned-mode registration");learnedStrings[index]=text;
}
uintptr_t __cdecl learnedActor(unsigned id) {return address(id==1?0x2100020:0x2100000);}
uintptr_t __cdecl learnedMode(unsigned space,unsigned id,unsigned lang) {
    check(space==1 && (id==0x1033 || id==0x1034) && lang==0,"original mode ID/localized getter contract");
    return address(id==0x1033?0x2499000:0x2499040);
}
uintptr_t __cdecl learnedTemplate(unsigned id,unsigned lang) {
    check(id==0x300f && lang==0,"original learned-message template");return address(0x2499100);
}
uintptr_t __cdecl learnedResolve(unsigned space,uintptr_t token) {
    check(space==7 && read<uint8_t>(token)==0x12,"battle macros use renderer namespace7 rather than banner0");
    const auto index=read<uint8_t>(token+1)-0x30;check(index<2 && learnedStrings[index],"native copied text is registered before use");
    return learnedStrings[index];
}
int __cdecl learnedDraw(unsigned banner,uintptr_t text) {
    check(!banner,"original Overdrive announcement submits banner0");
    Event e;e.rva=0x49db10;e.epoch=generation();e.args[0]=banner;e.args[1]=text;
    for(auto h:listeners.at(e.rva))if(h.enter)h.enter(e);return 1;
}
void overdriveLearnFixtures() {
    const auto original=ffx_fixture::load<104>("offline-output/battle_amount_prompts_20261021/overdrive-learn.bin",13203755760411709477ull);
    auto memory=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
    check(memory!=nullptr,"original learned-mode initializer allocation");memcpy(memory,original.bytes,104);
    std::ifstream fixes(ffx_fixture::evidenceRoot()/"offline-output/battle_amount_prompts_20261021/overdrive-learn-fixups.txt");
    check(fixes.good(),"original learned-mode fixups");unsigned offset=0,target=0;
    while(fixes>>offset>>target) {
        uintptr_t call=0;
        switch(target) {
        case 0x8b5770:call=reinterpret_cast<uintptr_t>(learnedClear);break;
        case 0x8b57a0:call=reinterpret_cast<uintptr_t>(learnedRegister);break;
        case 0x8ac800:call=reinterpret_cast<uintptr_t>(learnedActor);break;
        case 0x78fd40:call=reinterpret_cast<uintptr_t>(learnedMode);break;
        case 0x78f940:call=reinterpret_cast<uintptr_t>(learnedTemplate);break;
        case 0x89db10:call=reinterpret_cast<uintptr_t>(learnedDraw);break;
        default:check(false,"unmodeled original learned-mode dependency");
        }
        check(offset+4<=104,"learned-mode relocation bounds");
        const auto relative=static_cast<int32_t>(call-reinterpret_cast<uintptr_t>(memory+offset+4));memcpy(memory+offset,&relative,4);
    }
    encoded(0x2499000,"Stoic");encoded(0x2499040,"Comrade");
    const auto end=encoded(0x2499102," has learned Overdrive mode ")+strlen(" has learned Overdrive mode ");
    set<uint8_t>(0x2499100,0x12);set<uint8_t>(0x2499101,0x30);
    write<uint8_t>(end,0x12);write<uint8_t>(end+1,0x31);write<uint8_t>(end+2,0x3b);write<uint8_t>(end+3,0);
    stub(0x4b6610,learnedResolve);const auto prior=read<uintptr_t>(address(0xd34460));set<uintptr_t>(0xd34460,0);resetBattle();
    const auto call=reinterpret_cast<int(__cdecl*)(unsigned,int)>(memory);
    call(0,0);drainSpeechDomainsForTest();expect(L"Tidus has learned Overdrive mode Stoic!","original initializer expands both registered strings");
    auto n=said.size();call(1,1);call(0,0);drainSpeechDomainsForTest();
    check(said.size()==n+2 && said[n].first==L"Yuna has learned Overdrive mode Comrade!" && said[n+1].first==L"Tidus has learned Overdrive mode Stoic!",
        "rapid announcements retain their own character and mode before namespace reuse");
    n=said.size();call(0,0);drainSpeechDomainsForTest();check(said.size()==n+1,"equal learned messages remain distinct native events");
    stub(0x4b6610,messageSubstitution);set<uintptr_t>(0xd34460,prior);resetBattle();VirtualFree(memory,0,MEM_RELEASE);
    puts("PASS: original104-byte learned-mode initializer, namespace7/banner0, both copied substitutions, rapid reuse and repeated native events; native text getters/renderer controlled.");
}
}
