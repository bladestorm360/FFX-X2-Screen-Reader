#include "../owned_message_decoder.hpp"
#include "../text_plan.hpp"
namespace {
uintptr_t __cdecl messageSubstitution(unsigned slot,uintptr_t token) {
    check(slot==0 && read<uint8_t>(token)==0x12,"native window-local substitution context");
    return address(0x2340800);
}
bool glyphSwap=false;
int __cdecl glyphRegion() { return glyphSwap?1:0; }
uintptr_t __cdecl keyGlyphName(unsigned id) { return reinterpret_cast<uintptr_t>(id==35?"E":id==36?"Enter":"Escape"); }
uintptr_t __cdecl padGlyphName(unsigned id) { return reinterpret_cast<uintptr_t>(id==11?"X button":id==12?"A button":"B button"); }
bool primerKnown=false;
int __cdecl primerGlyph(uintptr_t p,uint8_t* out) { out[0]=primerKnown?read<uint8_t>(p):0x70; out[1]=0; return primerKnown?2:1; }
void messageFixtures() {
    const auto ownedPage=[&](unsigned slot,uintptr_t source) {
        prepareCopiedControlNames(); // Scenario-specific immutable-name fixture cache.
        const auto original=fixture_reference::messagePage(slot,source);
        TextPlan p;prepareTextPlan(p,"owned-message-fixture");const auto root=planMessage(p,slot,source);
        check(p.valid && root,"bounded message instructions captured");const auto& node=p.nodes[root-1];
        const auto result=decodeOwnedMessage(p.bytes.data()+node.offset,node.length,copiedControlContext(p.controls));
        if(original.prompt!=result.prompt || original.choices!=result.choices || original.lines!=result.lines || original.selector!=result.selector)
            std::fprintf(stderr,"owned message mismatch original=%ls result=%ls selector=%d/%d lines=%u/%u\n",original.prompt.c_str(),result.prompt.c_str(),original.selector,result.selector,static_cast<unsigned>(original.lines.size()),static_cast<unsigned>(result.lines.size()));
        check(original.prompt==result.prompt && original.choices==result.choices && original.lines==result.lines && original.selector==result.selector,
            "owned decoder retains original substitution/selector/Primer/control/line semantics");
        return result;
    };
    constexpr unsigned window=0x2330000,cursor=0x2330200,stream=0x2340000;
    resetMessages(); resetSelectionDetail(); scene=0; set<int>(0xefbbf8,0);
    set<uintptr_t>(0x14676b0,address(window)); set<uintptr_t>(0x1468a50,address(cursor));
    set<uint16_t>(window+4,0); set<uintptr_t>(window+0x94,address(stream));
    // Never announce the inactive future language variant or the draw cursor.
    encoded(0x2341000,"Wrong variant"); set<uintptr_t>(window+0x5c,address(0x2341000));
    set<uintptr_t>(window+0x8c,address(stream)+3);
    std::vector<uint8_t> bytes;
    auto append=[&](const char* value) {
        auto p=encoded(0x2341800,value);
        for(size_t i=0;read<uint8_t>(p+i);++i) bytes.push_back(read<uint8_t>(p+i));
    };
    auto commit=[&] { bytes.push_back(0); check(copy(reinterpret_cast<uintptr_t>(bytes.data()),reinterpret_cast<void*>(address(stream)),bytes.size()),"encoded message copy"); };
    append("A history book"); bytes.push_back(3); bytes.insert(bytes.end(),{16,0x30}); append("Read");
    bytes.push_back(3); bytes.insert(bytes.end(),{16,0x31}); append("Dont read"); commit();
    auto before=said.size(); event(0x4b8980,0);
    check(said.size()==before+1 && said.back().first==L"A history book","NPC prompt excludes first selectable book row");
    before=said.size(); event(0x4b5f60,0,[&]{set<uint8_t>(cursor+1,1);set<uint8_t>(cursor+5,2);});
    check(said.size()==before+1 && said.back().first==L"Read" && !said.back().second,"initial book row is queued after installed prompt");
    event(0x4b6490,0,[&]{set<uint8_t>(cursor+2,1);}); expect(L"Dont read","last book row speaks");
    before=said.size(); event(0x4b6490,0); check(said.size()==before,"unchanged menu event is silent");
    event(0x4b6490,0,[&]{set<uint8_t>(cursor+2,0);}); expect(L"Read","returning to same option speaks again");
    keys['I'](false); expect(L"A history book","menu detail contains prompt only");
    resetMessages(); bytes.clear();
    const char* options[]={"Rest","Buy weapons","Buy items","Leave"};
    for(unsigned i=0;i<4;++i) { bytes.insert(bytes.end(),{16,static_cast<uint8_t>(0x30+i)}); append(options[i]); bytes.push_back(3); }
    commit(); set<uint8_t>(cursor+1,0); before=said.size(); event(0x4b8980,0);
    check(said.size()==before,"choice-only shop does not speak Rest as NPC text");
    event(0x4b5f60,0,[&]{set<uint8_t>(cursor+1,1);set<uint8_t>(cursor+5,4);}); expect(L"Rest","first shop option is row zero");
    for(unsigned row=1;row<4;++row) { event(0x4b6490,0,[&]{set<uint8_t>(cursor+2,static_cast<uint8_t>(row));}); check(said.back().first==ownedPage(0,address(stream)).choices[row],"every shop option uses native anchor"); }
    expect(L"Leave","last shop option has text");
    resetMessages(); set<uint8_t>(cursor+1,0); bytes.clear();
    append("We have a wide selection of"); bytes.push_back(3); append("quality goods");
    bytes.push_back(1); const auto next=bytes.size(); append("Second page"); commit();
    event(0x4b8980,0); expect(L"We have a wide selection of quality goods","all current dialogue lines, no future page");
    event(0x4de350,address(window),[&]{set<uintptr_t>(window+0x94,address(stream)+next);}); expect(L"Second page","native page advance is spoken without a reinstall");
    before=said.size(); event(0x4de350,address(window)); check(said.size()==before,"unchanged page initializer is silent");
    // Both page initializers use native window identity and reject other objects.
    event(0x4de5b0,address(window),[&]{set<uintptr_t>(window+0x94,address(stream));}); expect(L"We have a wide selection of quality goods","second native page initializer");
    encoded(0x2340800,"Rin"); stub(0x4b6610,reinterpret_cast<void*>(messageSubstitution));
    bytes.clear(); append("Welcome to "); bytes.insert(bytes.end(),{0x12,0x30}); bytes.push_back(0x3b); commit();
    event(0x4b8980,0); expect(L"Welcome to Rin!","native substitutions are resolved rather than omitted");
    encoded(0x2340800,"30"); bytes.clear(); append("Venom Tackle"); bytes.insert(bytes.end(),{0x12,0x30}); append("HP"); commit();
    check(ownedPage(0,address(stream)).prompt==L"Venom Tackle 30 HP","numeric macros retain both displayed label boundaries");
    bytes.clear(); append("Stats"); bytes.push_back(3); append("Move effect"); commit();
    check(ownedPage(0,address(stream)).lines==std::vector<std::wstring>{L"Stats",L"Move effect"},"review buffer preserves native line ordering");
    bytes.clear(); bytes.insert(bytes.end(),{16,0x30}); append("Substituted option"); commit();
    check(copy(address(stream),reinterpret_cast<void*>(address(0x2340800)),bytes.size()),"substituted selector copy");
    bytes.clear(); bytes.insert(bytes.end(),{0x12,0x30}); commit();
    check(ownedPage(0,address(stream)).choices[0]==L"Substituted option","substitution choice anchors retain native selector semantics");
    resetMessages(); testBlitzMirror=true;
    set<uintptr_t>(0x14676b0+8,address(window)); set<uintptr_t>(0x1468a50+8,address(cursor));
    encoded(stream,"Tidus"); set<uint8_t>(cursor+1,0); before=said.size(); event(0x4b8980,2);
    check(said.size()==before,"player-name mirror does not create a second utterance");
    testBlitzMirror=false; event(0x4b8980,2); expect(L"Tidus","same name in ordinary dialogue still speaks");
    bytes.clear(); bytes.insert(bytes.end(),{7,0,16,0x31}); append("Second"); bytes.push_back(3);
    bytes.insert(bytes.end(),{16,0x30}); append("First"); bytes.push_back(2); append("Future"); commit();
    const auto page=ownedPage(0,address(stream));
    check(page.choices[0]==L"First" && page.choices[1]==L"Second" && page.prompt.empty(),"choice marker order, zero payload and page-stop semantics");
    resetMessages(); resetSelectionDetail(); set<uint8_t>(cursor+1,0);
    stub(0x4acee0,reinterpret_cast<void*>(glyphRegion));
    stub(0x5af0,reinterpret_cast<void*>(keyGlyphName)); stub(0x5b10,reinterpret_cast<void*>(padGlyphName));
    const auto oldConfig=read<uintptr_t>(address(0x1efb504)); set<uintptr_t>(0x1efb504,address(0x2342800));
    set<unsigned>(0x2342800+0x60+6*4,35); set<unsigned>(0x2342800+0x60+4*4,36); set<unsigned>(0x2342800+0x60+5*4,37);
    set<unsigned>(0x2342800+0x60+20*4,11); set<unsigned>(0x2342800+0x60+18*4,12); set<unsigned>(0x2342800+0x60+19*4,13);
    bytes.clear(); append("Status by pressing "); bytes.insert(bytes.end(),{11,0x30}); commit();
    check(ownedPage(0,address(stream)).prompt==L"Status by pressing E","tutorial button token resolves to the configured keyboard key");
    testPad=true; fixtureInputDevice(true); check(ownedPage(0,address(stream)).prompt==L"Status by pressing X button","controller glyph uses the configured controller binding");
    testPad=false; fixtureInputDevice(false); glyphSwap=true;
    check(buttonGlyph(0x32)==L"Enter","regional confirm/cancel icon swap follows the native renderer");
    glyphSwap=false; set<uintptr_t>(0x1efb504,oldConfig);
    stub(0x506780,reinterpret_cast<void*>(primerGlyph)); set<uint16_t>(window+0x10,0x4000);
    encoded(stream,"AB"); primerKnown=false;
    check(ownedPage(0,address(stream)).prompt==L"aa","uncollected primers retain the native cipher glyphs");
    primerKnown=true; check(ownedPage(0,address(stream)).prompt==L"AB","collected primers use the native readable glyphs");
    primerKnown=false; set<uint16_t>(window+0x10,0);
    check(ownedPage(0,address(stream)).prompt==L"AB","ordinary dialogue bypasses primer conversion");
    encoded(stream,"Copied message");TextPlan snapshot;prepareTextPlan(snapshot,"message-source-reuse");
    snapshot.headline=planMessage(snapshot,0,address(stream));encoded(stream,"Overwritten source");
    check(renderTextPlan(snapshot,snapshot.headline,copiedControlContext(snapshot.controls))==L"Copied message","message plan owns bytes after source reuse");
    set<uint16_t>(window+0x2c,2); encoded(stream,"Current dialogue"); event(0x4b8980,0);
    check(chords['T'].second(),"T is active within the current dialogue window");
    output(L"Unrelated screen reader status",true,"navigation-selection"); chords['T'].first(false);
    expect(L"Current dialogue","T repeats the current dialogue instead of last mod speech");
    encoded(stream,"New dialogue line"); event(0x4b8980,0); chords['T'].first(false); expect(L"New dialogue line","T follows the last rendered dialogue page");
    event(0x4b8510,0,[&]{set<uint16_t>(window+0x2c,0);});
    check(!chords['T'].second(),"closed dialogue cannot repeat stale text");
    resetMessages();
    std::puts("PASS: native row anchors, line review, macro spacing, nested selectors and Blitzball mirror ownership.");
}
}
