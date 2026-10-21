#include "help_renderer_bytes.inc"
#include "help_decoder_bytes.inc"
#include "../status_review.hpp"
#include <intrin.h>
namespace {
uintptr_t replayLastCall=0;
uintptr_t replayHelpEntry=0;
uintptr_t replayHelpDecoder=0;
unsigned replayHelpProgress=0;
unsigned __cdecl helpStory(unsigned) {return replayHelpProgress;}
unsigned __cdecl helpInput(unsigned) {return 0;}
uintptr_t __cdecl helpGraphics() {replayLastCall=reinterpret_cast<uintptr_t>(_ReturnAddress());return fixtureAddress(0x25e0000);}
unsigned __cdecl helpNoop() {replayLastCall=reinterpret_cast<uintptr_t>(_ReturnAddress());return 0;}
unsigned __cdecl helpConditional(uintptr_t entry) {replayHelpEntry=entry;replayLastCall=reinterpret_cast<uintptr_t>(_ReturnAddress());return read<uint8_t>(entry)!=0;}
// Numeric runtime boundary: consume ST0 and truncate to signed64, preserving
// the control word, as the original MSVC conversion helper requires.
__declspec(naked) void helpFloatInteger() {
    __asm {
        sub esp,16
        fnstcw word ptr [esp]
        mov ax,word ptr [esp]
        or ax,0c00h
        mov word ptr [esp+2],ax
        fldcw word ptr [esp+2]
        fistp qword ptr [esp+8]
        fldcw word ptr [esp]
        mov eax,dword ptr [esp+8]
        mov edx,dword ptr [esp+12]
        add esp,16
        ret
    }
}
void helpDecoded(uintptr_t out,uintptr_t source,unsigned parameter,uintptr_t updating,uintptr_t ready) {
    replayLastCall=reinterpret_cast<uintptr_t>(_ReturnAddress());
    if(replayHelpDecoder) reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,unsigned,uintptr_t,uintptr_t)>(replayHelpDecoder)(out,source,parameter,updating,ready);
    else {std::memcpy(reinterpret_cast<void*>(out),reinterpret_cast<void*>(source),4096);if(ready) write<int>(ready,0);}
    Event e;e.rva=0x376a50;e.parentRva=0x36f960;e.epoch=generation();
    e.edi=replayHelpEntry;
    e.args[0]=out;e.args[1]=source;e.args[2]=parameter;e.args[3]=updating;e.args[4]=ready;
    for(const auto& listener:listeners[e.rva]) if(listener.leave) listener.leave(e);
}
uintptr_t helpFont(unsigned rva,uintptr_t text,int x,int y) {
    Event e;e.rva=rva;e.parentRva=0x36f960;e.epoch=generation();const auto index=rva==0x4e75c0?1:2;
    e.args[index-1]=7;
    e.args[index]=text;e.args[index+1]=x;e.args[index+2]=y;
    for(const auto& listener:listeners[e.rva]) if(listener.leave) listener.leave(e);
    return helpGraphics()+16;
}
uintptr_t __cdecl helpFont11(uintptr_t,unsigned,uintptr_t text,int x,int y,unsigned,unsigned,unsigned,unsigned,unsigned) {return helpFont(0x4e52e0,text,x,y);}
uintptr_t __cdecl helpFont8(uintptr_t,unsigned,uintptr_t text,int x,int y,unsigned,unsigned,unsigned,unsigned,unsigned) {return helpFont(0x4e52b0,text,x,y);}
uintptr_t __cdecl helpFontOther(uintptr_t,unsigned,uintptr_t text,int x,int y,unsigned,unsigned,unsigned,unsigned,unsigned) {return helpFont(0x4ee210,text,x,y);}
void __cdecl helpFont5(unsigned,uintptr_t text,int x,int y,unsigned,unsigned,unsigned,unsigned,unsigned) {helpFont(0x4e75c0,text,x,y);}
void nativeHelpRendererFixture() {
    const auto load=[](const char* name) {
        std::ifstream file((ffx_fixture::evidenceRoot()/"offline-output/help_escape_20261014/native-help-original").string()+"/"+name,std::ios::binary);
        check(file.good(),"original Help source exists");return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
    };
    const auto page=load("gnm/now_help_page.sps2"),text=load("now_help.sps2");
    constexpr unsigned pageBase=0x2600000,textBase=0x2700000,buffer=0x25e4000;
    std::memcpy(reinterpret_cast<void*>(address(pageBase)),page.data(),page.size());
    std::memcpy(reinterpret_cast<void*>(address(textBase)),text.data(),text.size());
    const auto directory=address(pageBase)+read<unsigned>(address(pageBase)+8);
    const auto strings=address(textBase)+read<unsigned>(address(textBase)+16);
    set<uintptr_t>(0xd2a01c,directory);set<uintptr_t>(0xd2a024,strings);set<uintptr_t>(0xd2a03c,address(pageBase));
    set<uintptr_t>(0xd2a040,address(textBase));set<uintptr_t>(0xd2a02c,address(buffer));set<unsigned>(0xd2a080,155);
    // Only original text entries pass the simulated rendering condition.
    // Graphics, native conditions, text expansion and backend are boundaries;
    // position arithmetic, loop, font selection and stack arguments are actual.
    for(const auto& fixup:helpFixups) if(fixup.relative) replayStub(fixup.rva,helpNoop);
    replayStub(0x43b6d0,helpGraphics);replayStub(0x374b60,helpConditional);replayStub(0x376a50,helpDecoded);
    replayStub(0x5497e0,helpFloatInteger);
    replayStub(0x4e52e0,helpFont11);replayStub(0x4e52b0,helpFont8);replayStub(0x4ee210,helpFontOther);replayStub(0x4e75c0,helpFont5);
    const auto code=fixtureAddress(0x36f960);
    std::memcpy(reinterpret_cast<void*>(code),helpRendererBody,sizeof(helpRendererBody));
    for(const auto& fixup:helpFixups) {
        const auto value=nativeReplayAddress(fixup.rva)-(fixup.relative?code+fixup.offset+4:0);
        std::memcpy(reinterpret_cast<void*>(code+fixup.offset),&value,4);
    }
    for(unsigned i=0;i<7;++i) write<uintptr_t>(nativeReplayAddress(0x370414)+i*4,nativeReplayAddress(helpJumpTargets[i]));
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(code),sizeof(helpRendererBody));
    scene=11;setSetting(Setting::AutoDetail,0);
    for(const int kind:{5,8,11,0}) {
        set<int>(0x8416b0,kind);set<int>(0xd2a088,9);
        set<uintptr_t>(0xd2a084,address(pageBase)+read<unsigned>(directory+9*8)+2*0x138);
        event(0x375df0,0);event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
        expect(L"The CTB Window","original renderer itself selects the visible Help page heading");
        keys['I'](false);check(said.back().first.find(L"Different commands shift a character")!=std::wstring::npos,
            "original renderer stack/coordinates feed the whole CTB paragraph to production buffer");
        check(said.back().first.find(L"all\nenemies and allies")!=std::wstring::npos && said.back().first.find(L": Exit")==std::wstring::npos,
            "CTB I retains native newlines and excludes the controls footer");
        keys[VK_DOWN](false);expect(L"This window shows the turn order for all","CTB Down reads one native line");
        keys[VK_DOWN](false);expect(L"enemies and allies involved in a battle.","CTB next Down reads the next native line");
        keys['I'](true);drainSpeechDomainsForTest();
        check(said.back().first.find(L": Exit")!=std::wstring::npos && said.back().first.find(L"CTB")==std::wstring::npos,"Shift I reads only actively rendered controls");
        keys['I'](false);check(said.back().first.find(L"Different commands shift")!=std::wstring::npos,"control help leaves whole-page I intact");
        set<int>(0xd2a088,8);
        set<uintptr_t>(0xd2a084,address(pageBase)+read<unsigned>(directory+8*8)+2*0x138);
        event(0x375df0,0);event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
        expect(L"Battle Mode","original renderer selects the Battle Mode page heading");
        for(const auto vk:{VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT})
            check(contexts.at(vk)(),"original Battle Mode zero directional links leave all four arrows with its buffer");
        auto before=said.size();keys[VK_DOWN](false);drainSpeechDomainsForTest();
        check(said.size()==before+1 && said.back().first.find(L"1: Help Window")!=std::wstring::npos,
            "Battle Mode Down reads the first description without I");
        keys[VK_UP](false);expect(L"Battle Mode","Battle Mode Up returns to the buffer heading");
        keys['I'](false);
        const auto& speech=said.back().first;
        check(speech.find(L"1: Help Window")!=std::wstring::npos &&
              speech.find(L"Displays various information.")!=std::wstring::npos &&
              speech.find(L"2: Command Window")!=std::wstring::npos &&
              speech.find(L"Displays available commands.")!=std::wstring::npos &&
              speech.find(L"3: CTB Window")!=std::wstring::npos &&
              speech.find(L"Displays order of all turns.")!=std::wstring::npos &&
              speech.find(L"4: Status Window")!=std::wstring::npos &&
              speech.find(L"Displays party's HP & MP.")!=std::wstring::npos,
              "original renderer sends heading62 and paragraphs63-66 through each actual font ABI");
        setSetting(Setting::AutoDetail,1);event(0x375df0,0);before=said.size();
        event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
        check(said.size()==before+2 && said[before].first==L"Battle Mode" &&
              said.back().first.find(L"Displays various information.")!=std::wstring::npos &&
              said.back().first.find(L"Displays available commands.")!=std::wstring::npos &&
              said.back().first.find(L"Displays order of all turns.")!=std::wstring::npos &&
              said.back().first.find(L"Displays party's HP & MP.")!=std::wstring::npos,
              "opening original Battle Mode with AutoDetail reads all four descriptions without I");
        before=said.size();keys[VK_DOWN](false);drainSpeechDomainsForTest();
        check(said.size()==before+1 && said.back().first.find(L"1: Help Window")!=std::wstring::npos,
            "AutoDetail keeps arrow review to the selected paragraph");
        keys['I'](false);
        check(said.back().first.find(L"Displays party's HP & MP.")!=std::wstring::npos,
            "optional I still reads the whole Help page after arrow review");
        before=said.size();keys[VK_UP](false);drainSpeechDomainsForTest();
        check(said.size()==before+1 && said.back().first==L"Battle Mode",
            "returning to Help heading does not replay the whole automatic page");
        setSetting(Setting::AutoDetail,0);
    }
    // Execute the original conditional decoder as well as the renderer. Native
    // SDK story/input getters and final font/backend are the remaining boundaries.
    const auto decoderCode=nativeReplayAddress(0x25dd000);
    std::memcpy(reinterpret_cast<void*>(decoderCode),helpDecoderBody,sizeof(helpDecoderBody));
    for(const auto& fix:helpDecoderFixups) {
        const auto value=nativeReplayAddress(fix.rva)-(fix.relative?decoderCode+fix.offset+4:0);
        std::memcpy(reinterpret_cast<void*>(decoderCode+fix.offset),&value,4);
    }
    for(unsigned i=0;i<11;++i) write<uintptr_t>(nativeReplayAddress(0x376fe0)+i*4,decoderCode+helpDecoderJumps[i]-0x376a50);
    replayStub(0x549240,replayCookie);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(decoderCode),sizeof(helpDecoderBody));
    set<uintptr_t>(0x8416c0,address(0x25dc000));
    for(unsigned i=0;i<512;++i) write<uintptr_t>(address(0x25dc000)+i*4,reinterpret_cast<uintptr_t>(helpInput));
    write<uintptr_t>(address(0x25dc000)+10*4,reinterpret_cast<uintptr_t>(helpStory));
    write<uintptr_t>(address(0x25dc000)+58*4,reinterpret_cast<uintptr_t>(helpInput));
    write<uintptr_t>(address(0x25dc000)+284*4,reinterpret_cast<uintptr_t>(helpInput));
    replayHelpDecoder=decoderCode;replayHelpProgress=0;
    const auto menu=address(pageBase)+read<unsigned>(directory+24*8);
    for(const int kind:{5,8,11,0}) {
        resetHelp();set<int>(0x8416b0,kind);set<int>(0xd2a088,24);set<uintptr_t>(0xd2a084,menu+19*0x138);
        const auto before=said.size();
        event(0x375df0,0);event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
        check(said.size()==before+2 && said[before].first==L"Select a character." && said.back().first==L"Tidus",
            "character selector opens with its actual prompt then native visible highlighted name");
        for(const auto vk:{VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT}) check(!contexts.at(vk)(),"character menu leaves all arrows with native selection");
        for(const unsigned row:{9u,10u,12u,13u,16u,17u}) {
            const auto old=said.size();set<uintptr_t>(0xd2a084,menu+row*0x138);
            event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
            check(said.size()==old+1 && said.back().first==L"Unknown character","original locked character branch speaks its placeholder without a hidden name or prompt replay");
        }
        keys['I'](true);drainSpeechDomainsForTest();check(said.back().first.find(L": Exit")!=std::wstring::npos,"character menu control help reads its native control bar");
    }
    // Actual empty native name, distinct from a visible locked ??? token.
    const auto savedName=read<uint8_t>(address(0x2100000));write<uint8_t>(address(0x2100000),0);
    set<uintptr_t>(0xd2a084,menu+19*0x138);
    event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});expect(L"Empty slot","actual blank character name has an accessible empty-slot label");
    write<uint8_t>(address(0x2100000),savedName);
    resetHelp();set<int>(0x8416b0,5);set<int>(0xd2a088,23);
    const auto topics=address(pageBase)+read<unsigned>(directory+23*8);uintptr_t topic=0;
    for(unsigned row=0;row<read<uint16_t>(directory+23*8+4);++row) {
        const auto entry=topics+row*0x138;
        if(read<uint8_t>(entry)==0 && read<int16_t>(entry+0x16)==12) topic=entry;
    }
    check(topic!=0,"original Reference menu contains the selected Overdrives entry");set<uintptr_t>(0xd2a084,topic);
    auto beforeTopics=said.size();event(0x375df0,0);event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
    check(said.size()==beforeTopics+2 && said[beforeTopics].first==L"Select a topic." && said.back().first==L"Overdrives",
        "Reference opens with its actual prompt before its native initial option");
    check(!contexts.at(VK_LEFT)() && !contexts.at(VK_RIGHT)(),"Reference never treats spare Left/Right as a text buffer");
    // Empty/bodyless Help still owns its controls and retires old page content.
    auto controlsOnly=beginStatusRows(0,154,false,true);controlsOnly.help=controlsOnly.present=true;
    TextPlan footer;prepareTextPlan(footer,"help-control-only",false);footer.context[5]=HelpControls;
    addStatusRow(controlsOnly,footer,planLiteral(footer,L"X: Exit"));check(finishStatusRows(controlsOnly),"controls-only Help accepted");
    keys['I'](true);expect(L"X: Exit","a controls-only Help screen can still read its active controls");
    replayHelpDecoder=0;
    // Execute a real F70B page-link from the retained SPS2. Native update
    // calls this decoder directly, bypassing the input dispatcher376220.
    set<int>(0x8416b0,11);set<int>(0xd2a088,10);
    set<uintptr_t>(0xd2a084,address(pageBase)+read<unsigned>(directory+10*8)+2*0x138);
    event(0x375df0,0);event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
    // A caller's accepted page replacement is detected by render identity.
    // No initializer/input result is supplied here.
    set<int>(0xd2a088,9);set<uintptr_t>(0xd2a084,address(pageBase)+read<unsigned>(directory+9*8)+2*0x138);
    event(0x36f960,0,[&]{reinterpret_cast<void(__cdecl*)()>(code)();});
    expect(L"The CTB Window","actual native renderer replaces page10 with CTB without synthetic initialization/input");
    keys['I'](false);check(said.back().first.find(L"Different commands shift a character")!=std::wstring::npos,"accepted information page replaces the previous review buffer");
    uintptr_t link=0;unsigned destination=0;
    for(unsigned i=0;i+6<text.size();++i) if(text[i]==0x0b && text[i+1]==0xf7) {
        std::memcpy(&destination,text.data()+i+2,4);
        if(destination<page.size()) {link=address(textBase)+i;break;}
    }
    check(link!=0,"original Help contains an update-script page link");
    const auto transition=replayCode(0x25d9000,helpTransitionBody,helpTransitionFixups);
    replayStub(0x549240,replayCookie);int changed=0;
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,unsigned,int*,uintptr_t)>(transition)(address(buffer),link,0,&changed,0);
    check(changed==1 && read<uintptr_t>(address(0xd2a084))==address(pageBase)+destination,"actual native decoder selects its original page-link target and returns update-result1");
    Event update;update.epoch=generation();update.parentRva=0x36f580;update.args[0]=address(buffer);update.args[1]=link;update.args[3]=reinterpret_cast<uintptr_t>(&changed);
    for(const auto& h:listeners[0x376a50]) if(h.leave) h.leave(update);
    resetHelp();resetSelectionDetail();
    std::puts("PASS: original2740-byte Help renderer and1422-byte conditional decoder, four font ABIs, CTB native line rows, footer-only Shift I, Reference prompt/native arrows, character name/locked slots and controls-only pages; SDK getters/font/backend simulated.");
}
}
