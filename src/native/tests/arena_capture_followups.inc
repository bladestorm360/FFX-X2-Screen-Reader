namespace {
void arenaCaptureFixtures() {
    constexpr unsigned table=0x2450000,strings=0x2451000,selected=0x2451800,buffer=0x2452000,result=0x2453100,drawFlag=0x2453110;
    set<uintptr_t>(0xd2a024,address(table)); set<uintptr_t>(0xd2a040,address(strings));
    for(unsigned index=0;index<64;++index) set<unsigned>(table+index*4,index*16);
    set<uintptr_t>(0xd2a084,address(selected));set<int16_t>(selected+0x16,8);set<int16_t>(selected+0x18,-1);
    set<int>(0xd2a088,0);set<uint16_t>(0xd2ca90+0xba,307);scene=11;
    auto render=[&](bool update=false,bool outside=false) {
        auto expand=[&](unsigned index,const char* text) {
            encoded(buffer,text); Event e; e.rva=0x376a50;e.parentRva=outside?0:0x36f960;e.epoch=generation();
            e.args[0]=address(buffer);e.args[1]=address(strings+index*16);e.args[3]=update?address(result):0;e.args[4]=address(drawFlag);
            for(const auto& listener:listeners[e.rva]) if(listener.leave) listener.leave(e);
        };
        auto draw=[&] { expand(12,"Monster Arena");expand(8,"Besaid");expand(40,"Confirm or exit"); };
        if(outside) draw(); else event(0x36f960,0,draw);
        // All native buffers can be reused immediately after the call returns.
        ZeroMemory(reinterpret_cast<void*>(address(buffer)),4096);
        drainCopiedTextForTest();
    };
    for(int kind:{8,11}) {
        set<int>(0x8416b0,kind);setSetting(Setting::AutoDetail,0);event(0x375df0,0);
        auto n=said.size();render(false,true);check(said.size()==n,"unrendered Arena programs stay silent");
        render(true);check(said.size()==n,"Arena update expansion never speaks hidden state programs");
        render();expect(L"Monster Arena. Besaid","Arena title precedes accepted native area focus in both UI modes");
        n=said.size();render();check(said.size()==n,"Arena animation draw does not rearm a selection event");
        check(readSelectionDetail(),"Arena current displayed help has a manual reader");drainCopiedTextForTest();expect(L"Confirm or exit","I reads owned displayed Arena help");
        set<int>(result,1);event(0x376220,address(result));render();expect(L"Besaid","equal labels on distinct native selection events are retained");
        setSetting(Setting::AutoDetail,1);event(0x376220,address(result));n=said.size();render();
        check(said.size()==n+2 && said[n].first==L"Besaid" && said.back().first==L"Confirm or exit","Arena Autodetail follows its current headline");
        event(0x376220,address(result));event(0x36f960,0,[&] {
            encoded(buffer,"Old focus");Event e;e.rva=0x376a50;e.parentRva=0x36f960;e.epoch=generation();e.args[0]=address(buffer);e.args[1]=address(strings+8*16);e.args[4]=address(drawFlag);
            for(const auto& listener:listeners[e.rva]) if(listener.leave) listener.leave(e);
        });
        n=said.size();resetSelectionDetail();drainCopiedTextForTest();check(said.size()==n,"retired Arena selection drops an already copied frame");
        setSetting(Setting::AutoDetail,0);
    }
    set<uint16_t>(0xd2ca90+0xba,35);set<int>(0x8416b0,8);event(0x375df0,0);auto n=said.size();render();
    check(said.size()==n,"generic menus outside Monster Arena remain with their own producers");
    resetSelectionDetail();set<uint16_t>(0xd2ca90+0xba,0);set<int>(0x8416b0,0);scene=0;
    std::puts("PASS: owned Monster Arena renderer snapshots, both UI modes, hidden/update guards, Autodetail and cancellation.");
}
}
