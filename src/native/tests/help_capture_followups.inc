namespace ffx { void resetHelp(); }
namespace {
uintptr_t __cdecl helpNameSubstitution(unsigned space,uintptr_t token) {
    check(space==7 && read<uint8_t>(token)==0x13,"Help names use the actual font's native13 getter");
    return address(0x2100000+((read<uint8_t>(token+1)-0x30)&7)*0x20);
}
void helpCaptureFixtures() {
    constexpr unsigned table=0x2460000,strings=0x2461000,selected=0x2462000;
    constexpr unsigned pages=0x2463000,buffer=0x2464000,result=0x2465000,drawFlag=0x2465100;
    const int oldScene=scene,oldKind=read<int>(address(0x8416b0));
    const int oldAuto=setting(Setting::AutoDetail);
    const auto oldTable=read<uintptr_t>(address(0xd2a024));
    const auto oldStrings=read<uintptr_t>(address(0xd2a040));
    const auto oldSelected=read<uintptr_t>(address(0xd2a084));
    const auto oldPages=read<uintptr_t>(address(0xd2a01c));
    const auto oldBase=read<uintptr_t>(address(0xd2a03c));
    const int oldCount=read<int>(address(0xd2a080)),oldPage=read<int>(address(0xd2a088));
    const uint16_t oldRoom=read<uint16_t>(address(0xd2ca90)+0xba);
    set<uintptr_t>(0xd2a024,address(table));set<uintptr_t>(0xd2a040,address(strings));
    set<uintptr_t>(0xd2a01c,address(pages));set<uintptr_t>(0xd2a084,address(selected));
    set<int>(0xd2a080,2);set<int>(0xd2a088,0);set<uint16_t>(0xd2ca90+0xba,35);
    set<int>(0x8416b0,1);scene=11;setSetting(Setting::AutoDetail,0);
    set<uint8_t>(selected,0);set<int16_t>(selected+0x16,0);set<int16_t>(selected+0x18,-1);
    set<int16_t>(pages+4,2);
    for(unsigned i=0;i<64;++i)set<unsigned>(table+i*4,i*32);

    auto dispatch=[&](unsigned rva,uintptr_t arg=0,const std::function<void()>& change={}) {
        auto& ls=listeners[rva];std::vector<Event> calls(ls.size());
        for(size_t i=0;i<ls.size();++i) {auto& e=calls[i];e.rva=rva;e.args[0]=arg;e.epoch=generation();if(ls[i].enter)ls[i].enter(e);}
        if(change)change();
        for(size_t i=0;i<ls.size();++i)if(ls[i].leave)ls[i].leave(calls[i]);
    };
    auto expand=[&](unsigned index,const char* text,unsigned parent=0x36f960,bool update=false,bool ready=true) {
        encoded(buffer,text);Event e;e.rva=0x376a50;e.parentRva=parent;e.epoch=generation();
        e.args[0]=address(buffer);e.args[1]=address(strings+index*32);e.args[3]=update?address(result):0;e.args[4]=ready?address(drawFlag):0;
        for(const auto& l:listeners[e.rva])if(l.leave)l.leave(e);
        if(parent==0x36f960 && !update && ready) {
            Event glyph;glyph.rva=0x4e52e0;glyph.parentRva=0x36f960;glyph.epoch=generation();glyph.args[2]=address(buffer);glyph.args[3]=0;glyph.args[4]=index*16;
            for(const auto& l:listeners[glyph.rva])if(l.leave)l.leave(glyph);
        }
    };
    auto render=[&](std::initializer_list<std::pair<unsigned,const char*>> parts) {
        dispatch(0x36f960,0,[&]{for(auto p:parts)expand(p.first,p.second);});
        ZeroMemory(reinterpret_cast<void*>(address(buffer)),4096);
    };
    auto drain=[&]{drainSpeechDomainsForTest();drainCopiedTextForTest();};
    auto n=said.size();dispatch(0x375df0);render({{0,"First"},{1,"First"}});drain();
    check(said.size()==n+1 && said.back().first==L"First","field-menu Help with ordinary engine state announces the focus with AutoDetail Off");
    n=said.size();expand(0,"Raw expansion",0);expand(0,"Update expansion",0x36f960,true);render({{0,"Idle"}});drain();
    check(said.size()==n,"unrendered expansion and update stay silent and do not rearm Help capture");
    check(readSelectionDetail(),"Help displayed page offers manual detail reading");drain();
    expect(L"First\nFirst","I reads the complete owned Help page after native buffers are overwritten");
    check(contexts.at(VK_DOWN)(),"Help owns the shared review arrows");
    keys[VK_DOWN](false);expect(L"First","equal displayed Help rows remain separate buffer entries");

    // Native text scripts resolve their destination page AFTER the update
    // decoder returns, without any 776220 result1 navigation callback.
    n=said.size();set<int>(result,1);
    Event scriptPage;scriptPage.epoch=generation();scriptPage.parentRva=0x36f580;
    scriptPage.args[0]=address(buffer);scriptPage.args[3]=address(result);
    for(const auto& l:listeners[0x376a50])if(l.leave)l.leave(scriptPage);
    set<int>(0xd2a088,1);set<int16_t>(pages+8+4,2);
    render({{0,"Script page heading"},{1,"Script page paragraph"}});drain();
    check(said.size()==n+1 && said.back().first==L"Script page heading","native update script arms page capture without menu navigation");
    check(readSelectionDetail(),"script-driven information page has a buffer");drain();
    expect(L"Script page heading\nScript page paragraph","script-driven Help page I reads its complete owned content");
    set<int>(0xd2a088,0);

    // A selection-only movement captures just its newly focused source.
    set<int16_t>(selected+0x16,0);dispatch(0x375df0);
    set<int>(result,1);dispatch(0x376220,address(result),[=]{set<int16_t>(selected+0x16,1);});
    n=said.size();render({{0,"Nonfocus rendered"},{1,"Focused line"}});drain();
    check(said.size()==n+1 && said.back().first==L"Focused line","Help focus movement selects only the current native source");

    // A page change keeps whole-page capture enabled.
    set<int>(0xd2a088,0);dispatch(0x376220,address(result),[=]{set<int>(0xd2a088,1);set<int16_t>(selected+0x16,0);});
    set<int16_t>(pages+8+4,2);n=said.size();
    render({{0,"Page two first"},{1,"Page two second"}});drain();
    check(said.size()==n+1 && said.back().first==L"Page two first","Help page change replaces focus and the active buffer");
    check(readSelectionDetail(),"new Help page has manual details");drain();
    expect(L"Page two first\nPage two second","manual Help page contains all and only current visible lines");
    setSetting(Setting::AutoDetail,1);dispatch(0x375df0);n=said.size();
    render({{0,"First current"},{1,"Second current"}});drain();
    check(said.size()==n+2 && said[n].first==L"First current" && said.back().first==L"Second current","Help AutoDetail reads current title then remaining visible page");
    setSetting(Setting::AutoDetail,0);

    // Retirement after native expansion but before the worker drains cancels the queued page.
    dispatch(0x376220,address(result),[]{set<int>(0xd2a088,0);});dispatch(0x375df0);
    n=said.size();dispatch(0x36f960,0,[&]{expand(0,"Cancelled by reset");});
    resetSelectionDetail();drain();check(said.size()==n,"resetSelectionDetail cancels a captured but undrained Help page");

    dispatch(0x375df0);n=said.size();dispatch(0x36f960,0,[&]{expand(0,"Cancelled by close");});
    scene=1;set<int>(result,2);dispatch(0x376220,address(result));drain();
    check(said.size()==n,"native Help close cancels an in-flight captured page");
    check(!contexts.at(VK_DOWN)(),"closed Help returns review arrows to the underlying context");
    scene=11;

    dispatch(0x375df0);n=said.size();render({{0,""},{1,""}});drain();
    check(said.size()==n,"all-ready empty Help text emits nothing and cannot repeat stale narration");
    dispatch(0x375df0);set<int16_t>(pages+4,513);n=said.size();render({{0,"Too many entries"}});drain();
    check(said.size()==n,"Help page above the native 512-entry guard stays silent");

    set<int16_t>(pages+4,1);set<uintptr_t>(0xd2a03c,address(selected));
    set<int>(selected+0x38,128);set<int>(selected+0x44,0);set<int>(selected+0x50,0);set<int>(selected+0x5c,0);
    dispatch(0x375df0);render({{0,"Directional Help"}});drain();
    check(!contexts.at(VK_DOWN)() && !contexts.at(VK_OEM_PERIOD)() && !contexts.at(VK_RIGHT)(),"Help menus keep native keys and never borrow spare arrows for buffer review");

    resetHelp();resetSelectionDetail();setSetting(Setting::AutoDetail,oldAuto);
    set<int>(0x8416b0,oldKind);scene=oldScene;set<uint16_t>(0xd2ca90+0xba,oldRoom);
    set<uintptr_t>(0xd2a024,oldTable);set<uintptr_t>(0xd2a040,oldStrings);
    set<uintptr_t>(0xd2a084,oldSelected);set<uintptr_t>(0xd2a01c,oldPages);
    set<uintptr_t>(0xd2a03c,oldBase);
    set<int>(0xd2a080,oldCount);set<int>(0xd2a088,oldPage);
    std::puts("PASS: native Help owned page snapshots, focus/page selection, worker detail, empty/oversize guards and cancellation.");
}
void realHelpPageFixtures() {
    stub(0x4b6610,reinterpret_cast<void*>(helpNameSubstitution));
    // Original English SPS2 and GN Native page records. Native conditions/VM
    // remain native in production; this replay uses unconditional ready strings,
    // recovered final-position arithmetic and the actual copied text decoder.
    const auto load=[](const char* name) {
        std::ifstream file((ffx_fixture::evidenceRoot()/"offline-output/help_escape_20261014/native-help-original").string()+"/"+name,std::ios::binary);
        check(file.good(),"retained original Help data exists");return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
    };
    const auto text=load("now_help.sps2"),pages=load("gnm/now_help_page.sps2");
    constexpr unsigned pageBase=0x2600000,textBase=0x2700000;
    check(pages.size()<0x100000 && text.size()<0x100000,"original Help fixtures fit their dedicated mapped ranges");
    memcpy(reinterpret_cast<void*>(address(pageBase)),pages.data(),pages.size());
    memcpy(reinterpret_cast<void*>(address(textBase)),text.data(),text.size());
    const auto oldScene=scene.load();const int automatic=setting(Setting::AutoDetail),oldKind=read<int>(address(0x8416b0));
    const unsigned globals[]={0xd2a01c,0xd2a024,0xd2a03c,0xd2a040,0xd2a080,0xd2a084,0xd2a088};
    uintptr_t previous[7]{};for(unsigned i=0;i<7;++i)previous[i]=read<uintptr_t>(address(globals[i]));
    scene=11;set<int>(0x8416b0,11);setSetting(Setting::AutoDetail,0);
    const auto oldLocale=fixtureLocale;fixtureLocale=1;
    const auto directory=address(pageBase)+read<unsigned>(address(pageBase)+8),strings=address(textBase)+read<unsigned>(address(textBase)+16);
    const auto count=read<unsigned>(address(pageBase)+4);check(count==155,"all155 original Help pages retained");
    set<uintptr_t>(0xd2a01c,directory);set<uintptr_t>(0xd2a024,strings);set<uintptr_t>(0xd2a03c,address(pageBase));
    set<uintptr_t>(0xd2a040,address(textBase));set<unsigned>(0xd2a080,count);
    unsigned renderedPages=0,renderedText=0;
    constexpr unsigned fonts[]={0x4e75c0,0x4e52b0,0x4e52e0,0x4ee210};
    for(const auto font:fonts) for(unsigned page=0;page<count;++page) {
        const auto entries=address(pageBase)+read<unsigned>(directory+page*8);const auto n=read<uint16_t>(directory+page*8+4);
        set<int>(0xd2a088,page);set<uintptr_t>(0xd2a084,entries+2*0x138); // Actual information-page focus is off-screen Dummy.
        event(0x375df0,0);const auto before=said.size();
        bool rejectImage=false;
        const auto drawPage=[&] {event(0x36f960,0,[&] {
            for(unsigned row=0;row<n;++row) {
                const auto entry=entries+row*0x138;
                if(read<uint8_t>(entry)==6 && page>=5 && page<=7) {
                    Event image;image.epoch=generation();image.parentRva=0x36f960;image.edi=entry;image.args[4]=1;
                    image.args[0]=read<int16_t>(entry+4);image.args[1]=read<int16_t>(entry+6);
                    if(rejectImage && font==fonts[0]) {
                        auto notReady=image;notReady.args[4]=0;
                        for(auto listener:listeners[0x374630])if(listener.leave)listener.leave(notReady);
                        std::array<uint8_t,0x138> wrongIdentity{};
                        memcpy(wrongIdentity.data(),reinterpret_cast<const void*>(entry),wrongIdentity.size());
                        *reinterpret_cast<int16_t*>(wrongIdentity.data()+0x14)=49;
                        auto wrong=image;wrong.edi=reinterpret_cast<uintptr_t>(wrongIdentity.data());
                        for(auto listener:listeners[0x374630])if(listener.leave)listener.leave(wrong);
                        *reinterpret_cast<int16_t*>(wrongIdentity.data()+0x14)=48;
                        *reinterpret_cast<int16_t*>(wrongIdentity.data()+0x16)=19;
                        for(auto listener:listeners[0x374630])if(listener.leave)listener.leave(wrong);
                        *reinterpret_cast<int16_t*>(wrongIdentity.data()+0x16)=30;
                        *reinterpret_cast<int16_t*>(wrongIdentity.data()+0x18)=0;
                        for(auto listener:listeners[0x374630])if(listener.leave)listener.leave(wrong);
                    } else if(rejectImage && font==fonts[1]) {
                        fixtureLocale=0;
                        for(auto listener:listeners[0x374630])if(listener.leave)listener.leave(image);
                        fixtureLocale=1;
                    } else {
                        for(auto listener:listeners[0x374630])if(listener.leave)listener.leave(image);
                    }
                }
                if(read<uint8_t>(entry)!=0 || read<int16_t>(entry+0x18)!=-1 || read<int16_t>(entry+0x20)!=-1)continue;
                const auto id=read<int16_t>(entry+0x16,-1);if(id<0)continue;
                const auto source=address(textBase)+read<unsigned>(strings+id*4);
                Event expand;expand.epoch=generation();expand.parentRva=0x36f960;expand.edi=entry;expand.args[0]=source;expand.args[1]=source;expand.args[4]=1;
                for(auto listener:listeners[0x376a50])if(listener.leave)listener.leave(expand);
                Event glyph;glyph.rva=font;glyph.epoch=generation();glyph.parentRva=0x36f960;const unsigned textArg=font==0x4e75c0?1:2;glyph.args[textArg]=source;
                glyph.args[textArg-1]=7;
                glyph.args[textArg+1]=read<int16_t>(entry+4)+read<int16_t>(entry+12);glyph.args[textArg+2]=read<int16_t>(entry+6)+read<int16_t>(entry+14);
                for(auto listener:listeners[font])if(listener.leave)listener.leave(glyph);
                ++renderedText;
            }
        });};
        if(page==6 && (font==fonts[0] || font==fonts[1])) {
            rejectImage=true;drawPage();drainSpeechDomainsForTest();
            check(said.size()==before,"unready/wrong module/image/variable or non-English image cannot publish an incomplete English diagram page");
            rejectImage=false;
        }
        drawPage();
        drainSpeechDomainsForTest();if(said.size()>before) {
            ++renderedPages;check(said.back().first!=L"Dummy" && said.back().first.find(L"Cross SE")==std::wstring::npos,"off-screen control/sound rows never become Help headlines");
        }
        if(page==5) {
            keys['I'](false);check(said.back().first.find(L"Options: Pause game (disabled in some areas)")!=std::wstring::npos && said.back().first.find(L"Cross: Talk / Examine / Confirm")!=std::wstring::npos,"Basic Controls buffer includes actual baked diagram labels");
        }
        if(page==6) {
            keys['I'](false);
            const auto& speech=said.back().first;
            check(speech.find(L"L1: Display switch window / Select character to switch")!=std::wstring::npos &&
                  speech.find(L"L2: Select character to switch")!=std::wstring::npos &&
                  speech.find(L"R1 / R2: Scroll CTB window")!=std::wstring::npos &&
                  speech.find(L"Triangle: Defend (pass turn)")!=std::wstring::npos &&
                  speech.find(L"Circle: Cancel")!=std::wstring::npos &&
                  speech.find(L"Cross: Confirm")!=std::wstring::npos &&
                  speech.find(L"Directional pad / left stick: Move cursor / Change command windows")!=std::wstring::npos &&
                  speech.find(L"Touch pad: Display / Hide help window")!=std::wstring::npos,
                  "original Battle Controls diagram buffer includes every verified controller label");
        }
        if(page==7) {
            keys['I'](false);
            const auto& speech=said.back().first;
            check(speech.find(L"L1 / R1: Switch characters")!=std::wstring::npos &&
                  speech.find(L"L2 / R2: Scroll lists")!=std::wstring::npos &&
                  speech.find(L"Directional pad / left stick: Move cursor")!=std::wstring::npos &&
                  speech.find(L"Circle: Cancel")!=std::wstring::npos &&
                  speech.find(L"Cross: Confirm")!=std::wstring::npos,
                  "original Menu Controls diagram buffer includes every verified controller label");
        }
        if(page==9) {
            expect(L"The CTB Window","actual CTB information page selects its displayed heading");
            keys['I'](false);check(said.back().first.find(L"This window shows the turn order for all")!=std::wstring::npos && said.back().first.find(L"Different commands shift a character")!=std::wstring::npos,"CTB buffer retains the complete actual paragraph");
            check(said.back().first.find(L"Dummy")==std::wstring::npos && said.back().first.find(L"Cross SE")==std::wstring::npos,"CTB buffer excludes native off-screen bookkeeping");
        }
        if(page==8) {
            keys['I'](false);
            const auto& speech=said.back().first;
            check(speech.find(L"Battle Mode")!=std::wstring::npos &&
                  speech.find(L"1: Help Window")!=std::wstring::npos &&
                  speech.find(L"Displays various information.")!=std::wstring::npos &&
                  speech.find(L"2: Command Window")!=std::wstring::npos &&
                  speech.find(L"Displays available commands.")!=std::wstring::npos &&
                  speech.find(L"3: CTB Window")!=std::wstring::npos &&
                  speech.find(L"Displays order of all turns.")!=std::wstring::npos &&
                  speech.find(L"4: Status Window")!=std::wstring::npos &&
                  speech.find(L"Displays party's HP & MP.")!=std::wstring::npos,
                  "actual Battle Mode page buffer includes heading62 and all four native description paragraphs");
        }
    }
    check(renderedPages>100 && renderedText>500,"representative original Help catalog exercises all page records");
    resetHelp();resetSelectionDetail();for(unsigned i=0;i<7;++i)write<uintptr_t>(address(globals[i]),previous[i]);
    scene=oldScene;fixtureLocale=oldLocale;set<int>(0x8416b0,oldKind);setSetting(Setting::AutoDetail,automatic);
    std::printf("PASS: actual155-page Help catalog through all4 font ABIs; %u page cases/%u unconditional native text draws, visible order, off-screen exclusion, all3 controller diagrams and full CTB/Battle Mode buffers. Native conditional execution/backend remain synthetic.\n",renderedPages,renderedText);
}

}
