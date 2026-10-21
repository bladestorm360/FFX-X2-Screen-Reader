namespace {
const ZanarkandSequence& loadPattern(unsigned monitor,unsigned stage) {
    const ZanarkandSequence* expected=nullptr;
    for(const auto& row:zanarkandSequences) if(row.worker==monitor && (monitor==53 || row.stage==stage)) expected=&row;
    check(expected!=nullptr,"verified monitor stage exists");
    load(trialUtilities[5],{monitor,2,expected->entryPc,expected->entryPc},false);resetPuzzleSolver();
    declaration(0x5c000004,1);put(chunk,0x402,static_cast<uint16_t>(declarations.size()));setValue(0x5c000004,1,stage);
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
    put(target,0x80,ptr(signal));put(target,0x90,ptr(signal));put(signal,8,uint16_t{2});put(signal,14,uint8_t{1});
    put(target,0x12c+0x4c+0x18,ptr(chunk)+0x1000+expected->entryPc);
    Event e;e.epoch=epoch;e.args[0]=ptr(target);e.args[1]=ptr(signal);fire(0x469040,e);
    return *expected;
}
void finishPattern(unsigned pc) {
    put(target,0x12c+0x4c+0x18,ptr(chunk)+0x1000+pc);
    Event e;e.epoch=epoch;e.args[0]=1;fire(0x469170,e,true,false);
    put(target,0x80,uintptr_t{0});put(target,0x90,uintptr_t{0});fire(0x469170,e,false,true);
}
void zanarkandPatternFixtures() {
    constexpr std::array<unsigned,9> originalOrder[]={
        {57,58,59,61,64},{74,73,71,77,80,82},{74,73,79,66,76,78,83},
        {74,73,70,68,72,79,66,80,77},{74,73,70,68,66,76,82}};
    unsigned row=0;
    for(const auto monitor:{53u,54u}) for(unsigned stage=monitor==53?0:1;stage<=(monitor==53?0u:4u);++stage) {
        const auto& source=loadPattern(monitor,stage);ZanarkandPatternSnapshot page;
        check(!captureZanarkandPattern(page),"unshown/in-flight monitor cannot reveal a solution");
        const auto before=replies.size();patternAutoDetail=1;finishPattern(source.endPc);
        check(captureZanarkandPattern(page) && page.count==source.count && page.glyphs==originalOrder[row++] && page.main==(monitor==54),
            "completed original monitor retains its exact displayed order");
        check(replies.size()==before+1 && replies.back().find(L"displayed glyph sequence.")!=std::wstring::npos &&
              replies.back().find(zanarkandGlyphLabel(page.glyphs[page.count-1]))!=std::wstring::npos,
              "AutoDetail speaks the full displayed sequence, including the ninth glyph");
        size_t position=0;
        for(unsigned i=0;i<page.count;++i) {position=replies.back().find(zanarkandGlyphLabel(page.glyphs[i]),position);check(position!=std::wstring::npos,"automatic glyph order matches original display");++position;}
        check(!writes && !requests && !warps && !puzzleArmed(),"reading a monitor never solves, moves, writes or arms assistance");
        const auto count=replies.size();finishPattern(source.endPc);check(replies.size()==count,"one completion cannot publish twice");
    }
    auto source=loadPattern(53,0);patternAutoDetail=0;finishPattern(source.endPc);ZanarkandPatternSnapshot page;
    check(captureZanarkandPattern(page) && replies.back()==L"First chamber displayed glyph sequence.","AutoDetail Off keeps the complete review snapshot without automatic body");
    source=loadPattern(54,1);finishPattern(source.endPc-1);check(!captureZanarkandPattern(page),"early/interrupted return cannot expose a sequence");
    source=loadPattern(54,2);setValue(0x5c000004,1,3);finishPattern(source.endPc);check(!captureZanarkandPattern(page),"changed native pattern cannot publish an earlier display");
    source=loadPattern(54,3);utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());finishPattern(source.endPc);
    check(!captureZanarkandPattern(page),"same-address allocation replacement cannot inherit a display");
    source=loadPattern(53,0);put(signal,8,uint16_t{1});finishPattern(source.endPc);check(!captureZanarkandPattern(page),"different signal entry cannot publish a display");
    source=loadPattern(53,0);
    put(target,0x12c+0x4c+0x18,ptr(chunk)+0x1000+source.endPc);
    Event earlier;earlier.epoch=epoch;earlier.args[0]=1;fire(0x469170,earlier,true,false);
    // Match the original completion's next-signal initialization, retaining
    // even the same worker/signal/allocation/entry identity to test ABA reuse.
    put(target,0x12c+0x4c+0x18,ptr(chunk)+0x1000+source.entryPc);
    Event queued;queued.epoch=epoch;queued.args[0]=ptr(target);queued.args[1]=ptr(signal);fire(0x469040,queued);
    const auto beforeQueued=replies.size();fire(0x469170,earlier,false,true);
    check(!captureZanarkandPattern(page) && replies.size()==beforeQueued,"earlier native completion cannot publish a queued unfinished display");
    finishPattern(source.endPc);
    check(captureZanarkandPattern(page) && replies.size()==beforeQueued+1,"queued display remains tracked until its own normal completion");
    source=loadPattern(53,0);++epoch;finishPattern(source.endPc);check(!captureZanarkandPattern(page),"epoch retirement cancels a display");
    AtelScript catalogScript;catalogScript.name="dome0700";catalogScript.checksum=trialUtilities[5].checksum;catalogScript.codeLength=trialUtilities[5].length;
    const auto* catalog=navigationCatalog(catalogScript);check(catalog!=nullptr,"Zanarkand navigation catalog qualifies");
    for(const auto& glyph:zanarkandGlyphs) {
        const auto* catalogTarget=navigationTarget(catalog,glyph.worker);check(catalogTarget && std::wstring(catalogTarget->label)==glyph.label,"floor and display use identical verified glyph labels");
    }
    patternAutoDetail=0;
    std::puts("PASS: original Zanarkand five display sequences, all27 shared visual labels, AutoDetail On/Off, nine-glyph capacity, normal RET/epoch/stage/allocation/entry guards; no game effects or unseen pattern disclosure.");
}
}
