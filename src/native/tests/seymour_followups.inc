// Original enemy ATEL headers and all256 m131 bytecode-entry outcomes are
// retained outside Git. Native renderer/animation/VM scheduling remain gaps.
namespace {
void seymourFixtures() {
    const auto matrix=ffx_fixture::load<3072>("offline-output/seymour_20261021/wheel-matrix.bin",933641065304129285ull);
    const auto boss=ffx_fixture::load<236>("offline-output/seymour_20261021/header-131.bin",2392996735338987473ull);
    const auto wheel=ffx_fixture::load<180>("offline-output/seymour_20261021/header-106.bin",16857170179518402222ull);
    const auto actors=address(0x2120000)+20*0xf90;
    std::array<uint8_t,5*0xf90> original{};check(copy(actors,original.data(),original.size()),"Seymour actor fixture backup");
    const auto priorArray=read<uintptr_t>(address(0xd34460));
    const auto priorScene=scene.load();scene=0;
    memcpy(reinterpret_cast<void*>(address(0x2498000)),boss.bytes,236);
    memcpy(reinterpret_cast<void*>(address(0x2498200)),wheel.bytes,180);
    for(unsigned i=0;i<5;++i) {
        const auto chr=actors+i*0xf90;
        write<unsigned>(chr+4,i?0x106a:0x1083);write<uint8_t>(chr+0xdc8,1);
        write<uintptr_t>(chr+0xf78,address(i?0x2498200:0x2498000));
        write<int>(chr+0x5d0,i?1:80000);write<int>(chr+0x6e4,i?2:160000);write<int>(chr+0x594,i?1:80000);
        encoded(static_cast<unsigned>(chr-testBase)+0x540,i?"Mortiphasm":"Seymour Omnis");
    }
    set<uintptr_t>(0xd34460,actors);set<uint16_t>(0xf3c9e8,0);set<uint8_t>(0xf3c912,0);
    constexpr const wchar_t* elements[]={L"Fire",L"Lightning",L"Water",L"Ice"};
    for(unsigned row=0;row<256;++row) {
        const auto data=matrix.bytes+row*12;
        for(unsigned i=0;i<4;++i)write<int>(address(0xd34568)+4+i*4,data[i]);
        for(unsigned i=0;i<4;++i) {
            set<uint16_t>(0xf3d1b4,static_cast<uint16_t>(21+i));
            set<uint16_t>(0xf3c9ea,static_cast<uint16_t>(1u<<(5+i)));
            event(0x49d300,address(0xf3c910));
            const auto expected=std::wstring(L"Mortiphasm: 100%, Energizing Seymour Omnis with ")+elements[data[4+i]]+
                L". Physical attacks turn left; magic turns right.";
            check(said.back().first==expected,"every wheel color agrees with original m131 control flow");
        }
        set<uint16_t>(0xf3d1b4,20);set<uint16_t>(0xf3c9ea,16);event(0x49d300,address(0xf3c910));
        std::wstring mix;
        for(unsigned i=0;i<4;++i)if(data[8+i]) {
            if(!mix.empty())mix+=L", ";mix+=elements[i];mix+=L" ";mix+=std::to_wstring(data[8+i]);
        }
        check(said.back().first==L"Seymour Omnis: 100%, Energizing wheels: "+mix,"all256 aggregate wheel mixtures match original bytecode counts");
    }
    auto count=said.size();event(0x49d300,address(0xf3c910));check(said.size()==count,"unchanged wheel event is silent");
    const auto measure=[&]() {
        const auto start=std::chrono::steady_clock::now();
        nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
        Event e;e.args[0]=address(0xf3c910);e.epoch=generation();
        for(unsigned i=0;i<1000;++i)for(auto h:listeners.at(0x49d300))if(h.leave)h.leave(e);
        nativeHeapFixture::observing=false;
        check(!nativeHeapFixture::allocations && !nativeHeapFixture::frees,"stable target observation stays heap-free");
        return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    };
    const auto wheelsMs=measure();
    set<uint16_t>(0xf3c9ea,0);set<uint16_t>(0xf3c9e8,1);event(0x49d300,address(0xf3c910));
    const auto regularMs=measure();
    set<uint16_t>(0xf3c9ea,16);set<uint16_t>(0xf3c9e8,0);event(0x49d300,address(0xf3c910));count=said.size();
    printf("METRIC stable1000 native target observations regular %.3fms, Seymour %.3fms; heap0/0; native frame/audio latency unmeasured.\n",regularMs,wheelsMs);
    // A changed visible position at the same target must refresh automatically.
    write<int>(address(0xd34568)+4,0);event(0x49d300,address(0xf3c910));check(said.size()==count+1,"same target refreshes on wheel rotation");
    write<int>(address(0xd34568)+4,4);event(0x49d300,address(0xf3c910));expect(L"Seymour Omnis: 100%","invalid position omits the mechanic");
    write<int>(address(0xd34568)+4,0);write<unsigned>(actors+4,0x1082);event(0x49d300,address(0xf3c910));
    count=said.size();write<unsigned>(actors+4,0x1083);event(0x49d300,address(0xf3c910));check(said.size()==count+1,"model qualification restores only the actual Seymour encounter");
    write<uint8_t>(actors+0xf90+0xdc8,0);event(0x49d300,address(0xf3c910));expect(L"Seymour Omnis: 100%","incomplete encounter cannot read unrelated shared words");
    memcpy(reinterpret_cast<void*>(actors),original.data(),original.size());set<uintptr_t>(0xd34460,priorArray);
    set<uint16_t>(0xf3c9e8,0);set<uint16_t>(0xf3c9ea,0);resetBattle();scene=priorScene;
    puts("PASS: original m131 all256 positions,1024 individual wheel colors, aggregate counts, original m106/131 headers, position/model/lifetime guards and same-target rotation events; graphics/VM timing unmeasured.");
}
}
