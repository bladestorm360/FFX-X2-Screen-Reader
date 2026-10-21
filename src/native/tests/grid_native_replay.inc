#include "grid_replay_bytes.inc"
#include "../sphere_sections_data.hpp"
#include "../sphere_expert_data.hpp"
#include "../sphere_jump.hpp"
namespace ffx {bool sphereJumpForTest(const wchar_t*);}
namespace {
uintptr_t nativeReplayAddress(unsigned rva) {
    // Non-production helper bodies/boundaries have private fixture slots.
    // Every production feature address still uses its qualified profile map.
    try {return fixtureAddress(rva);}catch(const std::out_of_range&) {return testBase+rva;}
}
void replayStub(unsigned rva,void* function) {
    auto* code=reinterpret_cast<unsigned char*>(nativeReplayAddress(rva));code[0]=0xb8;
    const auto target=reinterpret_cast<uintptr_t>(function);std::memcpy(code+1,&target,4);code[5]=0xff;code[6]=0xe0;
    FlushInstructionCache(GetCurrentProcess(),code,7);
}
template<size_t N,size_t M> uintptr_t replayCode(unsigned rva,const unsigned char (&bytes)[N],const ReplayFixup (&fixups)[M]) {
    const auto code=nativeReplayAddress(rva);std::memcpy(reinterpret_cast<void*>(code),bytes,N);
    for(const auto& fix:fixups) {const auto value=nativeReplayAddress(fix.rva)-(fix.relative?code+fix.offset+4:0);std::memcpy(reinterpret_cast<void*>(code+fix.offset),&value,4);}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(code),N);return code;
}
__declspec(naked) void replayCookie() {
    __asm {
        ret
    }
}
__declspec(naked) void replaySqrt() {
    __asm {
        fsqrt
        ret
    }
}
void gridNativeReplayFixture() {
    const auto raw=fixtureAddress(0x2320000);
    set<uintptr_t>(0x1f05834,raw);write<float>(nativeReplayAddress(0x792184),0.5f);write<float>(nativeReplayAddress(0x80a004),0.0f);
    // The ordinary fixture trampoline uses EAX. Security-cookie success must
    // preserve the selector's pointer return in EAX, so use a direct RET.
    write<uint8_t>(nativeReplayAddress(0x549240),0xc3);replayStub(0x5497b8,replaySqrt);
    std::memcpy(reinterpret_cast<void*>(nativeReplayAddress(0x22fe90)),gridDotBody,sizeof(gridDotBody));
    const auto native=replayCode(0x656860,gridSelectorBody,gridSelectorFixups);
    using Selector=uintptr_t(__cdecl*)(const float*,const float*,uintptr_t);
    const int dx[]={0,1,0,-1,-1,1,-1,1},dy[]={-1,0,1,0,-1,-1,1,1};
    unsigned comparisons=0,missingLinks=0,wakkaMissing=0;
    const auto audit=[&](const GridSnapshot& grid,bool standard) {
        set<int16_t>(0x2320002,static_cast<int16_t>(grid.nodes.size()));
        for(unsigned i=0;i<grid.nodes.size();++i) {const auto& n=grid.nodes[i];write<int16_t>(raw+0x808+i*0x28,static_cast<int16_t>(n.x));write<int16_t>(raw+0x80a+i*0x28,static_cast<int16_t>(n.y));write<int16_t>(raw+0x80e+i*0x28,n.valid?0:-1);}
        for(unsigned from=0;from<grid.nodes.size();++from) if(grid.nodes[from].valid) {
            const auto choices=sphereArrowChoices(grid,from);float origin[]={static_cast<float>(grid.nodes[from].x),static_cast<float>(grid.nodes[from].y),0,1};
            for(unsigned k=0;k<8;++k) {
                float input[]={static_cast<float>(dx[k]),static_cast<float>(dy[k]),0,1};const auto pointer=reinterpret_cast<Selector>(native)(origin,input,0);
                const int selected=pointer?static_cast<int>((pointer-raw-0x808)/0x28):-1;
                if(selected!=choices[k]) std::fprintf(stderr,"Grid audit %s node%u key%u native%d mod%d\n",standard?"standard":"expert",from+1,k,selected+1,choices[k]+1);
                check(selected==choices[k],"production directions agree with original selector for every real grid node and all eight inputs");++comparisons;
            }
            for(const auto& edge:grid.links) {const int to=edge.a==static_cast<int>(from)?edge.b:edge.b==static_cast<int>(from)?edge.a:-1;
                if(to<0 || !grid.nodes[to].valid || std::find(choices.begin(),choices.end(),to)!=choices.end()) continue;
                ++missingLinks;if(standard && sphereSectionSites[from].section==4) ++wakkaMissing;
                GridRoute step;step.nodes={static_cast<int>(from),to};const auto text=sphereNextText(grid,step);
                check(text.find(L"No direct arrow combination")!=std::wstring::npos,"crowded neighbour is described as position, with F6 recovery, never a false arrow instruction");
            }
        }
    };
    GridSnapshot standard,expert;
    for(const auto& n:sphereSectionSites) {GridNode row;row.x=n.x;row.y=n.y;row.valid=true;standard.nodes.push_back(row);}
    for(const auto& e:sphereSectionEdges) standard.links.push_back({e.a,e.b,0});
    for(const auto& n:sphereExpertSites) {GridNode row;row.x=n.x;row.y=n.y;row.valid=n.present;expert.nodes.push_back(row);}
    for(const auto& e:sphereExpertEdges) expert.links.push_back({e.a,e.b,0});
    audit(standard,true);audit(expert,false);
    std::printf("PASS: actual474-byte x86 grid selector + original37-byte dot; %u standard/expert input comparisons, zero mismatches; %u directed links lack a direct8-key selection (%u in standard Wakka section). Predicate unrestricted; live mode masks tested separately.\n",comparisons,missingLinks,wakkaMissing);
    // Invoke the actual cursor wrapper + body through the production request.
    replayCode(0x648d70,gridCursorInnerBody,gridCursorInnerFixups);replayCode(0x648d20,gridCursorBody,gridCursorFixups);
    set<uint8_t>(0x1685f70,1);set<int16_t>(0x2320002,4);set<uint8_t>(0x2320000+0x115bc,0);set<uintptr_t>(0x2320000+0x115b0,0);
    set<uintptr_t>(0x1686108,address(pool));set<int16_t>(pool+0x2700,-1);set<uintptr_t>(0x2320000+0x115a8,address(0x645010));
    for(unsigned i=0;i<4;++i) {set<int16_t>(0x2320000+0x80e+i*0x28,i==3?-1:0);set<uint8_t>(0x2320000+0x82a+i*0x28,i==1?1:0);}
    scene=9;testForeground=true;event(0x225930,0);
    check(sphereJumpContext() && chords.at(VK_F6).second==sphereJumpContext,"F6 belongs to grid only under a qualified native browse/Move/Use context");
    set<int16_t>(pool+0x2700,7);event(0x225930,0);check(sphereJumpContext(),"F6 remains reserved for the grid while its action menu is open, without toggling the field beacon");
    set<int16_t>(pool+0x2700,-1);event(0x225930,0);
    for(const auto bad:{L"",L"0",L"5",L"-1",L"2x",L"1.5",L"99999"}) check(!sphereJumpForTest(bad),"numeric entry rejects empty, invalid and out of range input");
    check(sphereJumpForTest(L"3"),"one-based node accepted");event(0x225930,0);
    check(read<int16_t>(raw+0x112fc)==2 && read<uintptr_t>(raw+0x115b0)==address(0x645010),"actual original wrapper moves cursor and preserves return mode");
    set<uintptr_t>(0x2320000+0x115b0,0);set<uintptr_t>(0x2320000+0x115a8,address(0x644ef0));event(0x225930,0);
    check(sphereJumpForTest(L"3"),"Move destination queued");event(0x225930,0);check(read<int16_t>(raw+0x112fc)==2 && !read<uintptr_t>(raw+0x115b0),"native unavailable Move node does not run cursor function");
    check(sphereJumpForTest(L"2"),"native enabled Move destination queued");event(0x225930,0);check(read<int16_t>(raw+0x112fc)==1,"Move retains native CAN_TARGET predicate");
    set<uintptr_t>(0x2320000+0x115b0,0);set<uintptr_t>(0x2320000+0x115a8,address(0x6452d0));event(0x225930,0);
    check(sphereJumpForTest(L"4"),"absent-node number fits count but still requires native validation");event(0x225930,0);check(read<int16_t>(raw+0x112fc)==1,"absent node never selected");
    check(sphereJumpForTest(L"2"),"old context request accepted for cancellation test");set<uint8_t>(0x1685f70,0);event(0x225930,0);
    check(!sphereJumpContext() && read<int16_t>(raw+0x112fc)==1,"closed grid cancels command and releases F6 context");
    scene=0;resetSphereGrid();std::puts("PASS: F6 number parsing, original cursor-only native action, Browse/Move/Use flags, absent nodes, stale closure and context release.");
}
}
