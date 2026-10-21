// Production display observer fed by the original618-byte input controller.
// Rendering/frame scheduling, command validation, sound and window pop are
// controlled boundaries; no game process, real wallet or input device is used.
namespace {
void __cdecl amountSound(int) {}
int __cdecl amountCommand(int,int) {return 1;}
void __cdecl amountPop() {}
void __cdecl amountCookie(unsigned) {}
void battleAmountFixtures() {
    const auto original=ffx_fixture::load<618>("offline-output/battle_amount_prompts_20261021/amount-cursor.bin",14627878370210359373ull);
    const auto memory=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    check(memory!=nullptr,"numeric original controller allocation");
    const auto entry=reinterpret_cast<uintptr_t>(memory);memcpy(memory,original.bytes,618);
    std::ifstream fixes(ffx_fixture::evidenceRoot()/"offline-output/battle_amount_prompts_20261021/amount-cursor-fixups.txt");
    check(fixes.good(),"original numeric controller fixups");
    unsigned offset=0,va=0,relative=0;
    while(fixes>>offset>>va>>relative) {
        uintptr_t target=0;
        if(va==0x886b00)target=reinterpret_cast<uintptr_t>(&amountSound);
        else if(va==0x89a2a0)target=reinterpret_cast<uintptr_t>(&amountCommand);
        else if(va==0x89a5a0)target=reinterpret_cast<uintptr_t>(&amountPop);
        else if(va==0x949240)target=reinterpret_cast<uintptr_t>(&amountCookie);
        else {
            check(!relative,"every original controller call has an explicit fixture boundary");
            target=(va==0x1340424 || va==0x1340418 || va==0x133d6a0)?address(va-0x400000):testBase+va-0x400000;
        }
        check(offset+4<=618,"numeric fixup extent");write<uintptr_t>(entry+offset,target-(relative?entry+offset+4:0));
    }
    FlushInstructionCache(GetCurrentProcess(),memory,618);
    const int oldScene=scene;scene=0;setSetting(Setting::AutoDetail,0);resetBattle();
    set<uint8_t>(0x1fcc092,0);const auto window=address(0xf3c910);
    write<uint8_t>(window+1,4);write<uint16_t>(window+6,0x15);write<uint8_t>(window+8,0);
    encoded(0x24a6000,"Gil to use");write<uintptr_t>(window+0xb4,address(0x24a6000));
    set<int>(0xf3d6a0,0);set<int>(0xf40424,0);set<int16_t>(0xf40418,0);
    // Non-presentational operands belong only to this archived controller replay.
    write<int>(testBase+0xf4041c,123456789);write<int>(testBase+0xf40420,123456789);
    write<int16_t>(testBase+0xf40416,8);
    const auto input=reinterpret_cast<void(__cdecl*)(uintptr_t)>(entry);
    const auto key=testBase+0x21d09d6;
    const auto draw=[&] {event(0x4a3250,window);};
    draw();expect(L"Gil to use: 0 Gil, step 1","first visible amount speaks with Autodetail Off");
    auto before=said.size();draw();check(said.size()==before,"same rendered amount/digit is one focus event");
    write<uint16_t>(key,0x8000);input(window);draw();
    expect(L"Gil to use: 0 Gil, step 10","native Right moves to tens without changing amount");
    write<uint16_t>(key,0x1000);input(window);draw();
    expect(L"Gil to use: 10 Gil, step 10","native Up changes actual selected amount");
    write<uint16_t>(key,0x4000);input(window);draw();
    expect(L"Gil to use: 0 Gil, step 10","native Down decrements amount");
    write<uint16_t>(key,0x2000);input(window);draw();
    expect(L"Gil to use: 0 Gil, step 1","native Left returns to ones");
    // Every decimal position, including the original wallet cap and zero clamp.
    for(int digit=0;digit<9;++digit) {
        set<int16_t>(0xf40418,static_cast<int16_t>(digit));set<int>(0xf40424,0);
        write<uint16_t>(key,0x1000);input(window);draw();
        unsigned step=1;for(int i=0;i<digit;++i)step*=10;
        check(read<unsigned>(address(0xf40424))==step,"original controller decimal arithmetic");
    }
    set<int>(0xf40424,123456789);write<uint16_t>(key,0x1000);input(window);draw();
    expect(L"Gil to use: 123456789 Gil, step 100000000","native wallet ceiling is spoken exactly");
    set<int>(0xf40424,0);write<uint16_t>(key,0x4000);input(window);draw();
    expect(L"Gil to use: 0 Gil, step 100000000","native subtraction clamps to zero");
    before=said.size();set<int16_t>(0xf40418,9);draw();set<int16_t>(0xf40418,-1);draw();
    set<int16_t>(0xf40418,0);set<int>(0xf40424,-1);draw();set<int>(0xf40424,1000000000);draw();
    check(said.size()==before,"invalid native numeric bounds refuse speech");
    set<int>(0xf40424,123);write<uint16_t>(window+6,0x16);resetBattle();draw();
    expect(L"Gil to use: 123 Gil, step 1","Yojimbo uses the same actual native amount screen");
    before=said.size();set<int>(0xf3d6a0,1);draw();check(said.size()==before,"automatic native battle branch does not render the amount");
    set<int>(0xf3d6a0,0);draw();check(said.size()==before+1,"returning to the visible screen starts a fresh amount focus");
    before=said.size();write<uint8_t>(window+1,3);draw();write<uint8_t>(window+1,4);draw();
    check(said.size()==before+1,"inactive numeric phase cannot announce and reentry is fresh");
    write<uint16_t>(key,0);
    const auto stamp=std::chrono::steady_clock::now();
    nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
    Event e;e.args[0]=window;e.epoch=generation();
    for(unsigned i=0;i<1000;++i)for(auto h:listeners.at(0x4a3250))if(h.leave)h.leave(e);
    nativeHeapFixture::observing=false;
    check(!nativeHeapFixture::allocations && !nativeHeapFixture::frees,"unchanged render observer is heap-free");
    printf("METRIC numeric unchanged1000 render observations %.3fms; heap0/0; native graphics/usable audio unmeasured.\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-stamp).count());
    VirtualFree(memory,0,MEM_RELEASE);resetBattle();scene=oldScene;
    puts("PASS: original618-byte numeric controller, real decimal/cap/clamp operands, amount/digit render events, phase/automatic guards and both pay window types; native graphics/sound/validation controlled.");
}
}
