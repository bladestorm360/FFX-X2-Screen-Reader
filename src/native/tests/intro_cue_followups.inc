namespace ffx { void installIntroCue(); }
namespace {
uintptr_t __cdecl openingKeyName(unsigned id) {return id==32?reinterpret_cast<uintptr_t>("B"):verifiedKeyName(id);}
void introCueFixtures() {
    constexpr unsigned config=0x2511000;
    const auto previousConfig=read<uintptr_t>(address(0x1efb504));
    const auto previousSwap=read<unsigned>(address(0x859554));
    const auto previousMode=read<int>(address(0xefbbf8));
    const auto previousRoom=read<uint16_t>(address(0xd2ca90));
    set<uintptr_t>(0x1efb504,address(config));set<unsigned>(0x859554,0);set<int>(0xefbbf8,23);set<uint16_t>(0xd2ca90,0);scene=0;
    stub(0x5af0,openingKeyName);stub(0x5b10,verifiedPadName);
    constexpr unsigned physical[]={23,25,26,24,12,13,11,14,15,17,16,18,20,19};
    for(unsigned row=0;row<14;++row) {
        set<unsigned>(config+0x60+row*4,28+row);set<unsigned>(config+0x60+(49+row)*4,128);
        set<unsigned>(config+0x60+(14+row)*4,physical[row]);
    }
    // User's actual opening log: Cancel=X, Start=B. Confirm is independent.
    set<unsigned>(config+0x60+4*4,34);set<unsigned>(config+0x60+12*4,32);
    installIntroCue();
    auto loaded=[&] { Event e;e.epoch=generation();listeners[0x257b60].back().enter(e); };
    auto active=[&](uintptr_t result,unsigned callOffset=0x139) { Event e;e.epoch=generation();e.result=result;e.caller=address(0x241870)+callOffset;listeners[0x260580].back().leave(e); };
    auto input=[&](unsigned epoch=0) { Event e;e.epoch=epoch?epoch:generation();listeners[0x225930].back().leave(e); };
    testPad=true;fixtureInputDevice(false);const auto initial=said.size();
    // Production proxy returns native DirectInput immediately while bootstrap
    // installs readers. Replaying only events AFTER attachment must still work.
    active(0);input();drainCopiedTextForTest();check(said.size()==initial,"late inactive getter alone cannot invent a completed source");
    active(1,0x777);active(0,0x777);input();drainCopiedTextForTest();check(said.size()==initial,"unrelated getter callers cannot arm the opening instruction");
    testStatusOwner=false;active(1);active(0xffffff00,0x19f);testStatusOwner=true;
    input();drainCopiedTextForTest();check(said.size()==initial+1,"attachment after the native loader still follows active-to-Finish boot reads");
    expect(L"Press B or Menu/Start for the title menu, or let the intro play.","late startup gives the generic keyboard/controller instruction");
    active(1);active(0);input();drainCopiedTextForTest();check(said.size()==initial+1,"consumed episode is not rearmed by unrelated later active reads");
    const auto n=said.size();
    // Original boot loop: load the SWF, read active=1, native input/tick,
    // Finish clears its byte, read active=0, then next native input event.
    loaded();active(0);input();drainCopiedTextForTest();check(said.size()==n,"failed or never-active startup source stays silent");
    active(1);input();drainCopiedTextForTest();check(said.size()==n,"logos/autosave still active cannot produce the advance cue");
    testStatusOwner=false;active(0xffffff00);input();drainCopiedTextForTest();check(said.size()==n,"AL-only completion capture is safe on a foreign source owner; bindings stay on PInput owner");
    testStatusOwner=true;input(generation()+1);drainCopiedTextForTest();check(said.size()==n,"stale native input event cannot produce controls");
    testForeground=false;input();drainCopiedTextForTest();check(said.size()==n,"background game keeps the startup instruction pending");testForeground=true;
    input();set<unsigned>(config+0x60+12*4,33);drainCopiedTextForTest();
    check(said.size()==n+1,"completed startup produces one owned cue");
    expect(L"Press B or Menu/Start for the title menu, or let the intro play.","startup instruction is independent of device and later config reuse");
    active(0);input();drainCopiedTextForTest();check(said.size()==n+1,"one source completion is announced once");
    fixtureInputDevice(true);loaded();active(1);active(0);input();drainCopiedTextForTest();
    expect(L"Press B or Menu/Start for the title menu, or let the intro play.","controller receives the same generic keyboard/controller instruction");
    const auto repeated=said.size();loaded();active(1);active(0);input();drainCopiedTextForTest();
    check(said.size()==repeated+1,"a distinct equal startup event speaks without text deduplication");
    loaded();active(1);active(0);scene=3;input();drainCopiedTextForTest();check(said.size()==repeated+1,"native overlay delays the instruction");
    scene=0;input();drainCopiedTextForTest();check(said.size()==repeated+2,"closing the overlay retains the pending source");
    loaded();active(1);active(0);loaded();input();drainCopiedTextForTest();check(said.size()==repeated+2,"replaced opening cannot leak the previous ready state");
    active(1);active(0);set<uint16_t>(0xd2ca90,0x15c);input();drainCopiedTextForTest();
    const auto finalCue=said.size();loaded();active(1);active(0);set<uint16_t>(0xd2ca90,191);input();
    set<uint16_t>(0xd2ca90,0x15d);input();drainCopiedTextForTest();check(said.size()==finalCue,"leaving the startup route discards a postponed cue permanently");
    loaded();active(1);active(0);set<uint16_t>(0xd2ca90,23);input();drainCopiedTextForTest();check(said.size()==finalCue+1,"engine mode23 is not room23; the actual startmap room permits the completed opening cue");
    set<uint16_t>(0xd2ca90,previousRoom);set<int>(0xefbbf8,previousMode);
    set<uintptr_t>(0x1efb504,previousConfig);set<unsigned>(0x859554,previousSwap);testPad=false;fixtureInputDevice(false);

    constexpr unsigned keySource=0x2513000,request=0x2513100,warningSource=0x2514000;
    const char* key="Loc_Texture_Saveload_Attention_Before_Game";
    auto beginWarning=[&](const char* requested) {
        strcpy_s(reinterpret_cast<char*>(address(keySource)),128,requested);
        set<uintptr_t>(request+8,address(keySource));Event e;e.epoch=generation();e.args[1]=address(request);
        listeners[0x2e96d0].back().enter(e);return e;
    };
    auto warn=[&](const char* text,uintptr_t caller=0,unsigned parent=0x2e96d0,unsigned epoch=1) {
        strcpy_s(reinterpret_cast<char*>(address(warningSource)),1025,text);
        Event e;e.epoch=epoch;e.parentRva=parent;e.caller=caller?caller:address(0x2e99b0)+0x3e;
        e.args[1]=address(warningSource);e.args[2]=0x3ff;listeners[0x2e9840].back().leave(e);
    };
    auto endWarning=[&](Event& e) {listeners[0x2e96d0].back().leave(e);};
    auto warningCount=said.size();testStatusOwner=false;
    auto unrelated=beginWarning("Unrelated");warn("Unrelated text");endWarning(unrelated);
    auto wrongKey=beginWarning("Loc_Texture_Saveload_Attention_Before_Game_extra");warn("Wrong key");endWarning(wrongKey);
    auto actual=beginWarning(key);warn("Wrong caller",address(0x2e99b0)+0x3d);warn("Wrong parent",0,0);
    warn("Stale warning",0,0x2e96d0,generation()+1);endWarning(actual);
    testStatusOwner=true;drainCopiedTextForTest();check(said.size()==warningCount,"autosave captures only the exact key/caller/parent/current generation");
    testStatusOwner=false;actual=beginWarning(key);
    unrelated=beginWarning("Unrelated nested request");warn("Nested wrong warning");endWarning(unrelated);
    warn("Autosave fixture\nSave donn\xc3\xa9" "es");endWarning(actual);
    ZeroMemory(reinterpret_cast<void*>(address(warningSource)),1025);
    resetSelectionDetail();testStatusOwner=true;drainCopiedTextForTest();
    check(said.size()==warningCount+1,"ready warning survives source reuse and unrelated selection-detail resets");
    expect(L"Autosave fixture\nSave donn\u00e9es","native converted UTF8/newlines decoded on worker after foreign Flash capture");
    actual=beginWarning(key);warn("Equal warning");warn("Equal warning");endWarning(actual);drainCopiedTextForTest();
    check(said.size()==warningCount+3,"equal distinct displayed warning requests remain separate events");
    actual=beginWarning(key);warn("");
    memset(reinterpret_cast<void*>(address(warningSource)),'A',1024);
    Event longWarning;longWarning.epoch=generation();longWarning.parentRva=0x2e96d0;longWarning.caller=address(0x2e99b0)+0x3e;
    longWarning.args[1]=address(warningSource);longWarning.args[2]=0x3ff;
    listeners[0x2e9840].back().leave(longWarning);endWarning(actual);drainCopiedTextForTest();
    check(said.size()==warningCount+3,"empty and unterminated native warnings stay silent");
    puts("PASS: exact native autosave request, copied converted UTF8, owner/parent/caller/epoch/nesting guards, source reuse, selection independence and equal distinct events.");
    std::puts("PASS: native opening active-to-finished event, AL-only result, owner-only copied keyboard/controller controls, no premature/movie-only cue, focus/epoch/overlay/replacement guards and equal distinct events.");
}
}
