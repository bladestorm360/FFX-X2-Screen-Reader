namespace {
void battleTargetPreviewFixtures() {
    constexpr unsigned parent=0xf3c910,activeWindow=0x1fcc092;
    const auto target=address(parent)+0xf0;
    auto select=[&](unsigned type,unsigned id) {
        set<uint8_t>(activeWindow,0);
        set<uint16_t>(parent+6,static_cast<uint16_t>(type));
        set<uint16_t>(0x2211402,static_cast<uint16_t>(id));
        event(0x4a0d70,address(parent));
        event(0x49c870,address(parent));
    };
    auto preview=[&](unsigned command) {
        set<uint8_t>(activeWindow,1);
        write<uint16_t>(target+0xc,static_cast<uint16_t>(command));
        event(0x49d300,target);
    };
    write<uint8_t>(target+8,0);write<uint8_t>(target+2,0);
    write<uint16_t>(target+0xd8,1);write<uint16_t>(target+0xda,0);
    select(7,0x2002); // This equipment handle collides with the fixture's Ether ID.
    preview(0x2001);expect(L"Tidus: HP 100/200","Potion uses the accepted action after a weapon highlight");
    select(15,0x2002);
    preview(0x2001);expect(L"Tidus: HP 100/200","the other equipment window cannot choose the MP preview");
    select(10,2);
    preview(0x2002);expect(L"Tidus: MP 17/30","Ether uses the accepted action after a Switch highlight");
    auto calls=commandRecordReads;
    auto spoken=said.size();
    preview(0x2002);
    check(commandRecordReads==calls && said.size()==spoken,"unchanged target and command reuse the copied preview mode");
    preview(0x2001);expect(L"Tidus: HP 100/200","a new command refreshes identical target masks");
    preview(0x3012);expect(L"Tidus: MP 17/30","a full native ability record can select MP");
    select(5,8);
    calls=commandRecordReads;
    preview(0xfffe);expect(L"Tidus: HP 100/200","native sentinel after Summon defaults to HP");
    check(commandRecordReads==calls,"a sentinel is never sent to the full-record getter");
    preview(0x0002);expect(L"Tidus: HP 100/200","name-only ID also defaults to HP");
    check(commandRecordReads==calls,"a name-only entry is never read as a full record");
    preview(0x2002);expect(L"Tidus: MP 17/30","target window has active MP state before the next turn");

    // The new turn reuses the same window address, owner, command and mask.
    set<uint8_t>(activeWindow,0);set<uint16_t>(parent+6,0);
    event(0x4a0d70,address(parent));
    spoken=said.size();
    preview(0x2002);
    check(said.size()==spoken+1,"new turn retires the cached target identity even for an identical command");
    expect(L"Tidus: MP 17/30","new turn recomputes the preview");
    set<uint8_t>(activeWindow,0);event(0x4a0d70,address(parent));
    preview(0x2001);expect(L"Tidus: HP 100/200","the next turn does not inherit MP from the previous turn");
    set<uint8_t>(activeWindow,0);event(0x49a5a0,0);
    spoken=said.size();calls=commandRecordReads;
    event(0x49d300,target);
    check(said.size()==spoken && commandRecordReads==calls,"return from a retired target window reads no stale command");
    preview(0x2001);expect(L"Tidus: HP 100/200","reopening the target produces fresh speech");
    write<uint16_t>(target+0xd8,0);set<uint16_t>(parent+0xc,0);
    set<uint8_t>(activeWindow,0);resetBattle();
    puts("PASS: native target command ownership, HP/MP records, equipment and Switch isolation, same-target turns and window retirement.");
}
}
