namespace {
void navigationIdentityFixtures() {
    NavigationTarget staleCid{3,5,-1,-1,L"Cid",4,3};
    check(navigationEntityName(&staleCid,5,true,[](int i){return i==4?L"Custom Wakka":L"";})==L"Custom Wakka",
        "actual Wakka body overrides the airship catalog's Cid dialogue attribution");
    NavigationTarget staleWakka{2,5,-1,4,L"Wakka",4,1};
    check(navigationEntityName(&staleWakka,0x2101,true)==L"Cid",
        "dedicated native Cid body cannot inherit Wakka's catalog/saved-name identity");
    staleWakka.category=6;
    check(navigationEntityCategory(&staleWakka,0x2101,true)==5,"Cid's actual character body cannot disappear into a stale Object category");
    AtelScript innScript; innScript.name="guad0100"; innScript.code.resize(22852); innScript.checksum=0x02e78409;
    const auto innCatalog=navigationCatalog(innScript);
    const auto inn=innCatalog?navigationTarget(innCatalog,20):nullptr;
    check(inn && inn->category==7 && std::wstring(inn->label)==L"Innkeeper",
        "actual Guadosalam inn script resolves its rest-choice worker to Shops and Innkeeper");
    check(navigationEntityName(nullptr,0x29,true)==L"Rikku","salvage-ship RIKK_A costume retains Rikku's identity");
    check(navigationEntityName(nullptr,0x65,true)==L"Tidus" && navigationEntityName(nullptr,0x66,true)==L"Yuna",
        "other confirmed party costume variants retain their character identities");
    check(navigationEntityName(nullptr,0x2050,true)==L"Al Bhed","live Al Bhed sprite has a general identity");
    check(navigationEntityName(nullptr,0x207a,true)==L"Soldier","soldier sprite has a concise identity");
    check(navigationEntityName(nullptr,0x207e,true)==L"Priest","priest sprite has a concise identity");
    check(navigationEntityName(nullptr,0x2002,true)==L"Villager","generic townsfolk is a villager");
    check(navigationEntityName(nullptr,0xf117,true)==L"Object","unknown sprite cannot become an NPC");
    check(navigationEntityCategory(nullptr,0xf117,true)==6,"unknown actor is an object until verified");
    check(navigationEntityCategory(nullptr,0x2050,true)==5,"actual character sprite is an NPC");
    check(navigationEntityName(nullptr,0x500b,true)==L"Book","ALBDBOOK is a book not Al Bhed");
    check(navigationEntityCategory(nullptr,0x500b,true)==6,"book goes in Objects");
    check(navigationEntityCategory(nullptr,0x10d0,true)==5 && navigationEntityName(nullptr,0x10d0,true)==L"Cactuar","verified creature stays an NPC with its actual sprite identity");
    check(navigationEntityName(nullptr,0x5033,true)==L"Chocobo","verified character need not be a human model");
    check(navigationEntityCategory(nullptr,0x5002,true)==4 && navigationEntityName(nullptr,0x5002,true)==L"Treasure chest","actual treasure-chest model has correct class and label");
    NavigationTarget target{1,5,-1,-1,L"Maechen",4,3};
    for(unsigned model:{0x5001u,0x50abu}) {
        NavigationTarget stale{12,6,-1,-1,L"Inscription",4,2};
        check(navigationSaveSphereModel(model) && navigationEntityCategory(&stale,model,true)==3 &&
            navigationEntityName(&stale,model,true)==L"Save Sphere","both native save models override a stale inscription label/category");
        check(navigationEntityCategory(&stale,model,false)==6 && navigationEntityName(&stale,model,false)==L"Inscription",
            "a non-actor inscription cannot be classified from an unbound model");
    }
    check(navigationEntityName(&target,0x2050,true)==L"Maechen","actor-local speaker overrides a stock body");
    check(navigationEntityName(&target,0x500b,true)==L"Book" && navigationEntityCategory(&target,0x500b,true)==6,"current object model overrides stale NPC catalog");
    target.label=L"Primer"; target.category=6; target.identity=2;
    check(navigationEntityName(&target,0x500b,true)==L"Primer","verified collection identity overrides the generic book sprite");
    target.label=L"Book"; target.identity=1;
    check(navigationEntityName(&target,0x500b,true)==L"Book","ordinary readable books retain their distinct label");
    target.category=5;
    target.label=L"Shopkeeper"; target.identity=2;
    check(navigationEntityName(&target,0x2050,true)==L"Shopkeeper","service role overrides general sprite");
    target.label=L"Yuna"; target.properName=1; target.identity=4;
    check(navigationEntityName(&target,2,true)==L"Yuna","authored character identity uses saved native name without a talk prefix");
    target.properName=-1; target.identity=1; target.label=L"Villager";
    check(navigationEntityName(&target,0x2050,true)==L"Al Bhed","current sprite corrects a generic catalog body label");
    target.label=L"Switch"; target.identity=2; target.category=2;
    check(navigationEntityName(&target,0,false)==L"Switch","events use concise functional labels");
    check(navigationEntityName(nullptr,0,false)==L"Event","unknown events have no invented interaction phrase");
}
void navigationIdentityRuntimeFixtures() {
    constexpr unsigned npc=0x2240880;
    set<uint16_t>(npc,0x500b); keys[VK_OEM_3](false);
    expect(L"Book, 10.8 steps unknown, 1 of 1","actual book model survives unknown-script direct-interaction scan");
    keys[VK_OEM_4](true); expect(L"Book, 10.8 steps unknown, 1 of 1","book appears in Objects");
    keys[VK_OEM_4](true); expect(L"category: NPC, 0","book cannot appear in NPCs");
    resetNavigation(); set<uint16_t>(npc,0xf117); keys[VK_OEM_3](false);
    expect(L"0 NPCs","unknown model remains excluded from NPC category on rescan");
    keys[VK_OEM_6](true); keys[VK_OEM_6](true);
    expect(L"Object, 10.8 steps unknown, 1 of 1","unknown interactive model is not guessed as a villager");
    resetNavigation(); set<uint16_t>(npc,0x2002); keys[VK_OEM_3](false);
    expect(L"Villager, 10.8 steps unknown, 1 of 1","verified human actor returns to NPC identity");
}

}
