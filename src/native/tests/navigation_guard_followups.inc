namespace {
unsigned ownedGuardGetterCalls=0;
uintptr_t __cdecl guardVariable(uintptr_t,int index,int) { ++ownedGuardGetterCalls; return index==0?address(0x2390000):0; }
void navigationGuardFixtures(unsigned worker) {
    const auto originalHeader=read<uintptr_t>(address(worker)),originalChunk=read<uintptr_t>(address(worker+4));
    const auto flags=read<uint16_t>(address(worker+0x34)); const auto channels=read<uint8_t>(address(worker+0xab));
    const auto table=read<uintptr_t>(address(worker+0x44));
    uint8_t getterCode[7]{}; check(copy(address(0x46c500),getterCode,7),"save getter fixture");
    stub(0x46c500,reinterpret_cast<void*>(guardVariable));
    std::ifstream file(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/ka/kami0000/kami0000.ebp",std::ios::binary);
    check(file.good(),"retained Thunder Plains story callbacks");
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    const auto base=reinterpret_cast<uintptr_t>(raw.data()); const auto chunk=base+read<unsigned>(base+4);
    set<uintptr_t>(worker+4,chunk); set<uintptr_t>(worker+0x44,0); set<uint16_t>(worker+0x34,0x80);
    set<uintptr_t>(worker,chunk+read<unsigned>(chunk+0x38+46*4)); set<uint8_t>(worker+0xab,0x10);
    const auto ownedScript=readScript(chunk);
    auto ownedProof=[&](unsigned local,unsigned entry,bool nativeAvailable,const char* label) {
        // Copy declaration metadata and fake getter values before entering
        // the owned proof; its reader captures no original chunk address.
        const auto header=read<uintptr_t>(address(worker));
        const auto declarationOffset=read<unsigned>(header+0x14);
        const auto declarationCount=read<uint16_t>(header+2);
        struct SavedDeclaration { unsigned low,high; };
        std::vector<SavedDeclaration> declarations(declarationCount);
        for(unsigned i=0;i<declarationCount;++i) {
            const auto at=chunk+declarationOffset+i*8;
            declarations[i]={read<unsigned>(at),read<unsigned>(at+4)};
        }
        std::vector<std::optional<int>> savedValues(declarationCount);
        for(unsigned i=0;i<declarationCount;++i) {
            const auto low=declarations[i].low,high=declarations[i].high;
            const unsigned format=low>>28;
            if((low>>25&7)!=0 || !(high&0xffff) || format>5 || i!=0) continue;
            const auto source=address(0x2390000);
            if(!source) continue;
            int value=0;
            switch(format) {
            case 0: value=read<uint8_t>(source); break;
            case 1: value=static_cast<int8_t>(read<uint8_t>(source)); break;
            case 2: value=read<uint16_t>(source); break;
            case 3: value=static_cast<int16_t>(read<uint16_t>(source)); break;
            case 4: case 5: value=read<int>(source); break;
            }
            savedValues[i]=value;
        }
        const auto calls=ownedGuardGetterCalls;
        const bool available=ffx::callbackAvailableOwned(ownedScript,local,entry,[&](unsigned index)->std::optional<int> {
            if(index>=declarations.size()) return {};
            const auto& declaration=declarations[index];
            if((declaration.low>>25&7)!=0 || !(declaration.high&0xffff) || (declaration.low>>28)>5) return {};
            return savedValues[index];
        });
        check(ownedGuardGetterCalls==calls,"owned guard proof does not call native variable getter");
        check(available==nativeAvailable,label);
    };
    set<uint16_t>(0x2390000,2500);
    check(interactionEntries(address(worker),1u<<4)==std::vector<unsigned>{4},
        "Tidus scene remains available within its native story range");
    ownedProof(46,4,true,"owned guard proof matches available Tidus story callback");
    set<uint16_t>(0x2390000,3300);
    check(interactionEntries(address(worker),1u<<4).empty(),
        "already-played Tidus scene is removed when its actual callback returns without an event");
    ownedProof(46,4,false,"owned guard proof matches completed Tidus story callback");
    set<uint16_t>(0x2390000,2000);
    check(interactionEntries(address(worker),1u<<4).empty(),"future Tidus scene remains unavailable before its native story range");
    ownedProof(46,4,false,"owned guard proof matches future Tidus story callback");
    const auto header=read<uintptr_t>(address(worker)),declaration=chunk+read<unsigned>(header+0x14);
    const auto saved=read<unsigned>(declaration);
    check(write<unsigned>(declaration,saved|0x0c000000),"model transient event-data guard");
    check(interactionEntries(address(worker),1u<<4)==std::vector<unsigned>{4},
        "transient motion/proximity guards cannot remove an enabled distant destination");
    ownedProof(46,4,true,"owned guard proof retains transient unknown-guard interaction");
    check(write<unsigned>(declaration,saved),"restore actual saved-world guard");
    set<uintptr_t>(worker,chunk+read<unsigned>(chunk+0x38+56*4)); set<uint8_t>(worker+0xab,1);
    set<uint16_t>(0x2390000,1250);
    check(interactionEntries(address(worker),1u<<2)==std::vector<unsigned>{2},"Menu-labelled story trigger keeps its genuine playable condition");
    ownedProof(56,2,true,"owned guard proof matches available Menu-labelled story callback");
    set<uint16_t>(0x2390000,3300);
    check(interactionEntries(address(worker),1u<<2).empty(),"inactive Menu-labelled callback is filtered by current native story state");
    ownedProof(56,2,false,"owned guard proof matches completed Menu-labelled story callback");
    set<uint16_t>(0x2390000,1250); set<uint16_t>(worker+0x34,0);
    check(interactionEntries(address(worker),1u<<2).empty(),"native collision active flag wins over a satisfied script guard");
    set<uintptr_t>(worker,originalHeader); set<uintptr_t>(worker+4,originalChunk);
    set<uint16_t>(worker+0x34,flags); set<uint8_t>(worker+0xab,channels); set<uintptr_t>(worker+0x44,table);
    check(copy(reinterpret_cast<uintptr_t>(getterCode),reinterpret_cast<void*>(address(0x46c500)),7),"restore getter fixture");
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(0x46c500)),7);
}
}
