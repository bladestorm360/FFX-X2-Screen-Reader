#include "../airship_destinations.hpp"
#include "../airship_destinations_data.hpp"
#include "airship_render_bytes.inc"
namespace {
constexpr unsigned destChunk=0x2410000,destController=0x2430000,destWorkers=0x2440000;
constexpr unsigned destRoot=0x2490000,destGraphic=0x2490100,destTable=0x2490200,destLayers=0x2491000,destScalars=0x24a0000;
unsigned destLanguage=1,destNativeReads=0;
void __cdecl airshipTranslation(int,int,int) {} // Phyre component translation is a modeled boundary.
uintptr_t __cdecl destinationOperand(uintptr_t worker,int index,int element) {
    check(testStatusOwner,"airship operand getter stays on the native owner");++destNativeReads;
    check(worker==airshipWorkers[9] && index>=0 && index<30 && element>=0 && element<31,"qualified W9 operand bounds");
    return address(destScalars+static_cast<unsigned>(index)*0x100+static_cast<unsigned>(element)*4);
}
unsigned __cdecl destinationLanguage() {check(testStatusOwner,"airship UI language captured on native owner");return destLanguage;}
void destValue(unsigned index,int value,unsigned element=0) {set<int>(destScalars+index*0x100+element*4,value);}
void destFocus(unsigned index,unsigned scroll,unsigned id) {
    check(index>=scroll && index-scroll<15,"controlled visible destination row");
    destValue(7,static_cast<int>(index));destValue(16,30);destValue(25,static_cast<int>(scroll));destValue(29,1);
    destValue(18,static_cast<int>(id),index);destValue(10,20);destValue(11,84+18*static_cast<int>(index-scroll));
    set<unsigned>(destLayers+10*0xc0,0);set<unsigned>(destLayers+26*0xc0,0);
    set<int>(destLayers+26*0xc0+0xc,20);set<int>(destLayers+26*0xc0+0x10,84+18*static_cast<int>(index-scroll));
    const auto row=11+index-scroll;set<unsigned>(destLayers+row*0xc0,0);set<uint16_t>(destLayers+row*0xc0+0x96,static_cast<uint16_t>(id));
}
void destEvent(unsigned rva,unsigned layer=26) {
    Event e;e.rva=rva;e.epoch=generation();e.args[0]=layer;
    for(const auto& listener:listeners[rva]) if(listener.leave) listener.leave(e);
}
void airshipDestinationFixtures() {
    // The original EBP/immutable resource service supplies actual declarations
    // and hash identity. SDK getters and accepted layer records are controlled
    // boundaries; this is not a live frame/GPU or audible-latency measurement.
    const auto oldController=read<uintptr_t>(address(0xf26ae8)),oldLayerRoot=read<uintptr_t>(address(0x153bf80));
    const auto oldWorkers=airshipWorkers;const int oldScene=scene;const auto oldIndex=setting(Setting::MenuIndex);
    const auto oldFocus=testForeground;
    const auto oldRoom=read<int>(address(0xf30804));set<int>(0xf30804,382);
    std::array<uint8_t,7> oldLanguageStub{};
    std::memcpy(oldLanguageStub.data(),reinterpret_cast<void*>(address(0x241290)),oldLanguageStub.size());
    std::ifstream file(ffx_fixture::evidenceRoot()/"offline-output/airship_assets_20261019/files/ffx_ps2/ffx/master/jppc/event/obj/hi/hiku2100/hiku2100.ebp",std::ios::binary);
    check(file.good(),"approved original airship script retained");
    unsigned offsets[11]{};file.seekg(4);file.read(reinterpret_cast<char*>(offsets),sizeof(offsets));
    file.seekg(0,std::ios::end);unsigned end=static_cast<unsigned>(file.tellg());
    for(unsigned i=1;i<11;++i) if(offsets[i] && offsets[i]>=offsets[0] && offsets[i]<end) {end=offsets[i];break;}
    check(offsets[0]==64 && end>64 && end-64<0x20000,"original airship part extent");
    file.seekg(64);file.read(reinterpret_cast<char*>(address(destChunk)),end-64);
    check(file.gcount()==end-64,"original airship part copied completely");
    const auto script=readScript(address(destChunk));
    check(script.name=="hiku2100" && script.codeLength==24785 && script.checksum==0xde956a03,"actual airship code identity");
    airshipWorkers.clear();
    for(unsigned i=0;i<script.workers.size();++i) {
        const auto off=destWorkers+i*0x1000;ZeroMemory(reinterpret_cast<void*>(address(off)),0x1000);
        airshipWorkers.push_back(address(off));set<uintptr_t>(off,address(destChunk+script.workers[i].header));
        set<uintptr_t>(off+4,address(destChunk));set<uint16_t>(off+0x2e,static_cast<uint16_t>(i));
    }
    fixturePhysicalPart(destChunk,end-64);
    set<uintptr_t>(0xf26ae8,address(destController));set<uint16_t>(destController+0xc,41);set<uint16_t>(destController+0xa,9);
    set<uint16_t>(destController+0x1a,0);set<uintptr_t>(destController+0x48,address(destChunk));
    set<uintptr_t>(0x153bf80,address(destRoot));set<uintptr_t>(destRoot,address(destGraphic));set<uintptr_t>(destGraphic+8,address(destTable));
    set<unsigned>(destTable+4,48);set<uintptr_t>(destTable+8,address(destLayers));
    ZeroMemory(reinterpret_cast<void*>(address(destScalars)),30*0x100);
    for(unsigned i=0;i<48;++i) set<unsigned>(destLayers+i*0xc0,1);
    stub(0x46a7c0,airshipWorker);stub(0x46c500,destinationOperand);stub(0x241290,destinationLanguage);
    scene=0;testForeground=true;setSetting(Setting::MenuIndex,0);installAirshipDestinations();
    replayCode(0x51a3d0,airshipCountBody,airshipCountFixups);
    const auto nativePosition=replayCode(0x51ab30,airshipPositionBody,airshipPositionFixups);
    const auto nativeTexture=replayCode(0x51a930,airshipTextureBody,airshipTextureFixups);
    if(read<uint8_t>(nativePosition+4)!=0xe8 || nativePosition+9+read<int>(nativePosition+5)!=nativeReplayAddress(0x51a3d0))
        std::fprintf(stderr,"Airship replay size%zu sourceCall%d copiedCall%d target%x expected%x fixup%u/%x/%d\n",sizeof(airshipPositionBody),airshipPositionBody[4]==0xe8,read<uint8_t>(nativePosition+4)==0xe8,static_cast<unsigned>(nativePosition+9+read<int>(nativePosition+5)-testBase),static_cast<unsigned>(nativeReplayAddress(0x51a3d0)-testBase),airshipPositionFixups[0].offset,airshipPositionFixups[0].rva,airshipPositionFixups[0].relative);
    check(read<uint8_t>(nativePosition+4)==0xe8 && nativePosition+9+read<int>(nativePosition+5)==nativeReplayAddress(0x51a3d0),"externalized position replay retains its qualified count-call relocation");
    check(std::memcmp(reinterpret_cast<void*>(nativeReplayAddress(0x51a3d0)),airshipCountBody,1)==0,"externalized count replay starts with its original qualified opcode");
    replayStub(0x239ef0,reinterpret_cast<void*>(airshipTranslation));
    using Position=void(__cdecl*)(int,int,int);
    // Execute original count/position storage and its exact first/last-row
    // arithmetic before entering the real production capture/consumer pair.
    for(unsigned row=0;row<15;++row) {
        resetAirshipDestinations();destFocus(row,0,2);
        reinterpret_cast<Position>(nativePosition)(26,20,84+18*row);
        check(read<int>(address(destLayers+26*0xc0+0x10))==84+18*static_cast<int>(row)+(row==0?-1:row==14?1:0),"original room382 native cursor adjustment");
        destEvent(0x51ab30);drainSpeechDomainsForTest();
        check(said.back().first==L"Besaid Island","all original native cursor rows reach the image producer");
    }
    resetAirshipDestinations();destLanguage=1;destFocus(0,0,2);
    auto before=said.size();destEvent(0x51a830);
    check(said.size()==before,"native cursor event only queues owned airship output");
    const auto reads=destNativeReads;drainSpeechDomainsForTest();
    check(said.size()==before+1 && said.back().first==L"Besaid Island" && !said.back().second,"initial selected image follows native prompt without interruption");
    check(destNativeReads==reads,"preparation never enters native operand getters");
    before=said.size();destEvent(0x51a830);drainSpeechDomainsForTest();check(said.size()==before,"same native focus has one announcement");
    destFocus(1,0,2);destEvent(0x51ab30);drainSpeechDomainsForTest();
    check(said.size()==before+1 && said.back().first==L"Besaid Island","equal labels in distinct focused rows are preserved");
    for(unsigned language:{0u,1u,2u,3u,4u,5u,9u,10u,11u,18u,255u}) {
        destLanguage=language;
        for(unsigned id=1;id<=26;++id) {
            const auto index=id-1,scroll=index>14?index-14:0;
            destFocus(index,scroll,id);before=said.size();destEvent(0x51a830);drainSpeechDomainsForTest();
            check(said.size()==before+1 && said.back().first==airshipLabels::label(language,id),"only focused label uses original native locale-map/texture ID");
        }
    }
    destLanguage=1;destFocus(18,4,19);setSetting(Setting::MenuIndex,1);destEvent(0x51a830);drainSpeechDomainsForTest();
    check(said.back().first==L"Besaid Ruins 1, 19 of 31","absolute focus indexing survives scrolling");setSetting(Setting::MenuIndex,0);
    const auto refused=[&](const char* reason) {before=said.size();destEvent(0x51a830);drainSpeechDomainsForTest();check(said.size()==before,reason);};
    destFocus(0,0,2);set<unsigned>(destLayers+11*0xc0,1);refused("hidden selected row has no destination speech");
    destFocus(0,0,2);set<uint16_t>(destLayers+11*0xc0+0x96,26);before=said.size();destEvent(0x51a830);drainSpeechDomainsForTest();
    check(said.size()==before+1 && said.back().first==L"Penance","actual focused image wins over unready/inconsistent script metadata");
    destFocus(0,0,2);set<int>(destLayers+26*0xc0+0x10,100);refused("cursor must lie on an actual displayed row");
    destFocus(0,0,2);destValue(7,31);destNativeReads=0;destEvent(0x51a830);drainSpeechDomainsForTest();
    check(said.back().first==L"Besaid Island" && destNativeReads<7,"invalid metadata cannot overrun the array or silence a visible image");
    destFocus(0,0,27);refused("unknown/unverified texture labels are refused");
    destFocus(0,0,2);destValue(29,2);set<unsigned>(destLayers+26*0xc0,1);refused("native confirmation hides the list cursor before using layer29");
    destFocus(0,0,2);set<uint16_t>(destController+0xa,0);destNativeReads=0;destEvent(0x51a830);drainSpeechDomainsForTest();
    check(said.back().first==L"Besaid Island" && !destNativeReads,"accepted rendered focus survives another active worker without entering its operands");
    set<uint16_t>(destController+0xa,9);
    destFocus(0,0,2);set<int>(0xf30804,381);refused("another map cannot reuse the airship image catalog");set<int>(0xf30804,382);
    destFocus(0,0,2);testForeground=false;refused("background airship speech is refused");testForeground=true;
    destFocus(0,0,2);destEvent(0x51a830);before=said.size();destEvent(0x51a7c0,11);drainSpeechDomainsForTest();
    check(said.size()==before,"hiding the selected image retires queued output");
    destFocus(0,0,2);destEvent(0x51a830);before=said.size();resetAirshipDestinations();drainSpeechDomainsForTest();
    check(said.size()==before,"same-generation reset retires queued menu output");
    destFocus(0,0,2);destEvent(0x51a830);before=said.size();destEvent(0x51a7c0);drainSpeechDomainsForTest();check(said.size()==before,"closing cursor retires queued focus");
    // Actual W9 closes list10/26 before showing29, then changes texture on the
    // next scheduler pass. Execute the original setter, not a modeled selector.
    using Texture=void(__cdecl*)(int,uint16_t);
    const auto choose=[&](unsigned option) {
        reinterpret_cast<Texture>(nativeTexture)(29,static_cast<uint16_t>(option));
        check(read<uint16_t>(address(destLayers+29*0xc0+0x96))==option,"original native setter commits option variant");
        destEvent(0x51a930,29);
    };
    const auto openConfirmation=[&]() {
        set<unsigned>(destLayers+10*0xc0,1);destEvent(0x51a7c0,10);
        set<unsigned>(destLayers+26*0xc0,1);destEvent(0x51a7c0,26);
        set<unsigned>(destLayers+29*0xc0,0);destEvent(0x51a830,29);
    };
    set<uint16_t>(destController+0xa,0); // No prepared/current-W9 dependency.
    destLanguage=1;set<uint16_t>(destLayers+29*0xc0+0x96,1);before=said.size();
    openConfirmation();drainSpeechDomainsForTest();check(said.size()==before,"showing confirmation never speaks its stale previous texture");
    choose(0);check(said.size()==before,"native option event only queues owned output");
    auto optionReads=destNativeReads;drainSpeechDomainsForTest();
    check(said.size()==before+1 && said.back().first==L"Yes" && !said.back().second,"first accepted Yes appends after native Proceed prompt");
    check(destNativeReads==optionReads,"option preparation never calls native getters");
    before=said.size();choose(0);drainSpeechDomainsForTest();check(said.size()==before,"idle accepted variant is a local event guard");
    choose(1);drainSpeechDomainsForTest();check(said.back().first==L"No" && said.back().second,"changed highlighted No interrupts");
    choose(0);drainSpeechDomainsForTest();check(said.size()==before+2 && said.back().first==L"Yes","Yes No Yes preserves repeated text on different selection events");
    for(unsigned language:{0u,1u,2u,3u,4u,5u,9u,10u,11u,18u,255u}) {
        destLanguage=language;openConfirmation();
        for(unsigned option=0;option<2;++option) {
            before=said.size();choose(option);drainSpeechDomainsForTest();
            check(said.size()==before+1 && said.back().first==airshipLabels::confirmation(language,option),"highlighted option follows visually verified native locale images");
        }
    }
    destLanguage=1;openConfirmation();choose(0);before=said.size();destEvent(0x51a7c0,29);drainSpeechDomainsForTest();
    check(said.size()==before,"hiding29 retires queued option output");
    choose(1);drainSpeechDomainsForTest();check(said.size()==before,"texture changes without shown lifetime cannot speak");
    openConfirmation();choose(0);before=said.size();resetAirshipDestinations();drainSpeechDomainsForTest();
    check(said.size()==before,"same-generation reset retires queued confirmation output");
    openConfirmation();choose(2);choose(0);drainSpeechDomainsForTest();check(said.size()==before,"unverified texture retires confirmation lifetime");
    openConfirmation();testForeground=false;choose(0);drainSpeechDomainsForTest();testForeground=true;
    check(said.size()==before,"background confirmation is refused");
    openConfirmation();set<int>(0xf30804,381);choose(0);drainSpeechDomainsForTest();set<int>(0xf30804,382);
    check(said.size()==before,"another map cannot reuse confirmation images");
    openConfirmation();set<uintptr_t>(destTable+8,address(destLayers+0x6000));choose(0);drainSpeechDomainsForTest();set<uintptr_t>(destTable+8,address(destLayers));
    check(said.size()==before,"changed native layer ownership retires option lifetime");
    openConfirmation();choose(0);destEvent(0x51a7c0,26);drainSpeechDomainsForTest();
    check(said.size()==before+1 && said.back().first==L"Yes","list closure does not retire independent confirmation output");
    resetAirshipDestinations();setSetting(Setting::MenuIndex,oldIndex);scene=oldScene;testForeground=oldFocus;
    set<int>(0xf30804,oldRoom);
    airshipWorkers=oldWorkers;set<uintptr_t>(0xf26ae8,oldController);set<uintptr_t>(0x153bf80,oldLayerRoot);
    stub(0x46a7c0,navigationWorker);stub(0x46c500,unboundGuard);
    std::memcpy(reinterpret_cast<void*>(address(0x241290)),oldLanguageStub.data(),oldLanguageStub.size());
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(0x241290)),oldLanguageStub.size());
    Event freed;freed.args[0]=address(destChunk-0x30);
    for(const auto& listener:listeners[0x48d490]) if(listener.enter) listener.enter(freed);
    puts("PASS: room382 accepted rendered image/cursor, optional exact script indexing, 26 IDs/nine languages, initial/change/scroll/equal-label events, pending/different-worker metadata, hidden/map/coordinate guards and close/reset retirement. Renderer/getters/transport are modeled boundaries.");
    puts("PASS: original60-byte native confirmation texture setter; all localized Yes/No options, opening/movement/reopen order, independent lifetimes, no stale show, hidden/reset/map/focus/owner guards. Native Proceed text remains the sole prompt producer; live graphics/audio timing untested.");
}
}
