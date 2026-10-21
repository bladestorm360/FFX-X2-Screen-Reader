#include "../gagazet.hpp"
#include "gagazet_bytes.inc"
namespace {
uintptr_t gagIntegerCode=0,gagOperandCode=0;
unsigned gagFailure=0;
void __cdecl gagUnexpected() {check(false,"unmodeled float dependency in original integer predicate");}
unsigned __cdecl gagObservedInteger(uintptr_t w,unsigned index,unsigned element) {
    const auto v=reinterpret_cast<unsigned(__cdecl*)(uintptr_t,unsigned,unsigned)>(gagIntegerCode)(w,index,element);
    Event e;e.epoch=generation();e.parentRva=0x464180;e.args[0]=w;e.args[1]=index;e.args[2]=element;e.result=v;
    e.rva=0x46c6c0;for(const auto h:hooks.at(e.rva))if(h.leave)h.leave(e);return static_cast<unsigned>(e.result);
}
template<size_t N,size_t M> uintptr_t gagCode(const unsigned char (&body)[N],const GagazetFixup (&fixups)[M]) {
    auto* at=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));check(at!=nullptr,"original Gagazet operand code allocation");std::memcpy(at,body,N);
    for(const auto& f:fixups) {
        uintptr_t dependency=0;
        if(f.va==0x86c500)dependency=reinterpret_cast<uintptr_t>(getter);
        if(f.va==0x86c6c0)dependency=reinterpret_cast<uintptr_t>(gagObservedInteger);
        if(f.va==0x86c5a0 || f.va==0x9497e0)dependency=reinterpret_cast<uintptr_t>(gagUnexpected);
        if(f.va==0x1327098)dependency=reinterpret_cast<uintptr_t>(&gagFailure);
        if(f.va==0x86c738) {
            dependency=reinterpret_cast<uintptr_t>(at)+N;
            for(unsigned i=0;i<std::size(gagazetIntegerJumps);++i)write<uintptr_t>(dependency+i*4,reinterpret_cast<uintptr_t>(at)+gagazetIntegerJumps[i]);
        }
        check(dependency!=0,"qualified original operand dependency");
        const auto value=dependency-(f.relative?reinterpret_cast<uintptr_t>(at)+f.offset+4:0);std::memcpy(at+f.offset,&value,4);
    }
    FlushInstructionCache(GetCurrentProcess(),at,N);return reinterpret_cast<uintptr_t>(at);
}
void gagLoad(unsigned entry=6) {
    const TrialUtility t{L"Gagazet","mtgz0700",15853,0xcb3127fb,310,0,0,0,{22,entry,entry==6?9161u:10261u,entry==6?9161u:10261u},{22,entry,0,0},{0x010000d8,3,0},{},0,0};
    load(t,t.treasure,false);resetGagazet();declarations.clear();
    // Original aliases deliberately occupy separate declaration indices.
    // Preserve them; the generic fixture's declaration() helper deduplicates.
    for(const auto& row:gagazetDeclarations) {
        const auto i=static_cast<unsigned>(declarations.size());declarations.emplace_back(row[0],row[1]);
        put(chunk,0x200+i*8,row[0]);put(chunk,0x204+i*8,row[1]);
    }
    put(chunk,0x34,uint16_t{42});put(chunk,0x402,static_cast<uint16_t>(declarations.size()));
    put(chunk,0x500,uint8_t{4});put(chunk,0x502,static_cast<uint16_t>(declarations.size()));put(chunk,0x514,0x200u);
    for(unsigned i=0;i<std::size(gagazetEntries);++i)put(chunk,0x700+i*4,gagazetEntries[i]);
    put(target,0x90,ptr(signal));put(signal,8,static_cast<uint16_t>(entry));put(signal,14,uint8_t{1});put(target,0x32,uint8_t{1});
    gagAutomatic=false;gagCues=gagFocus=true;gagBeeps=0;
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
}
void gagLocation(unsigned pc) {put(target,0x12c+0x4c+0x18,ptr(chunk)+0x1000+pc);}
Event gagQuery(unsigned pc,unsigned physicalResult=0) {
    gagLocation(pc);Event e;e.rva=0x45e0c0;e.epoch=generation();e.parentRva=0x464180;e.args[0]=ptr(target);e.args[2]=1;e.result=physicalResult;
    for(const auto h:hooks.at(e.rva))if(h.leave)h.leave(e);return e;
}
unsigned gagVariable(unsigned index,unsigned element,bool originalOperand) {
    if(!originalOperand)return gagObservedInteger(ptr(target),index,element);
    alignas(4) unsigned stack[32]{};
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,unsigned,unsigned)>(gagOperandCode)(ptr(target),reinterpret_cast<uintptr_t>(stack),index,element);
    check(stack[0]==1 && reinterpret_cast<uint8_t*>(stack)[0x54]==1,"original123-byte operand push preserves integer stack tag and count");
    return stack[1];
}
bool gagPredicate(unsigned pc) {
    std::vector<int> stack;auto pop=[&] {check(!stack.empty(),"original predicate operand stack");const auto v=stack.back();stack.pop_back();return v;};
    for(unsigned work=0;work<40;++work) {
        gagLocation(pc);const auto code=ptr(chunk)+0x1000;const auto op=read<uint8_t>(code+pc);const auto arg=read<uint16_t>(code+pc+1);
        if(op==0xae) {stack.push_back(static_cast<int16_t>(arg));pc+=3;continue;}
        if(op==0x9f || op==0xa2) {const auto element=op==0xa2?pop():0;stack.push_back(static_cast<int>(gagVariable(arg,element,true)));pc+=3;continue;}
        if(op==0x19) {stack.push_back(!pop());++pc;continue;}
        if(op==0xd7) {check(stack.size()==1,"original conditional branch consumes exactly one boolean");return pop()!=0;}
        const int right=pop(),left=pop();if(op==5)stack.push_back(left&right);else if(op==6)stack.push_back(left==right);else if(op==2)stack.push_back(left&&right);else check(false,"unmodeled original Gagazet predicate opcode");++pc;
    }
    check(false,"original Gagazet predicate work bound");return false;
}
void gagazetFixtures() {
    installGagazet();gagIntegerCode=gagCode(gagazetIntegerBody,gagazetIntegerFixups);gagOperandCode=gagCode(gagazetOperandBody,gagazetOperandFixups);
    for(unsigned flags:{0u,64u,128u,192u}) {
        gagLoad();setValue(0x010000d8,3,flags,2);gagQuery(0x24e2,1);
        check(gagPredicate(0x2550)==(flags==192),"manual native hit predicate retains all four exact ring states");
        check(utilityValue(utilityScript(),0x010000d8,3,2)==static_cast<int>(flags),"manual predicate has no flag mutation");
        gagLoad();setValue(0x010000d8,3,flags,2);gagAutomatic=true;
        check(gagQuery(0x24e2).overrideResult && gagPredicate(0x2550),"production automatic throw enters original native success predicate for each ring state");
        check(utilityValue(utilityScript(),0x010000d8,3,2)==static_cast<int>(flags) && !writes,"automatic hit operands do not write saved flags, story or rewards");
    }
    gagLoad(7);gagAutomatic=true;
    for(int first=0;first<3;++first) {
        setValue(0x56000008,1,first);
        check(gagQuery(0x28c7).result==static_cast<unsigned>(first==1),"Rikku confirms only native right opening1");
        for(const auto pair:{std::pair<unsigned,unsigned>{0x2928,0}, {0x2945,0},{0x2962,static_cast<unsigned>(first!=1)},{0x297f,0}})
            check(gagQuery(pair.first).result==pair.second,"Rikku uses original direction queries to select right");
    }
    for(int first=0;first<3;++first)for(int second=0;second<3;++second) {
        setValue(0x56000008,1,first);setValue(0x5600000c,1,second);
        check(gagPredicate(0x2e27)==(first==1 && second==0),"original second-trial bytecode requires Rikku1 and Tidus0");
        const auto q=gagQuery(0x2a7a);check(q.overrideResult==(first==1),"partial manual Rikku choice is preserved rather than silently overwritten");
        if(first==1) {check(q.result==static_cast<unsigned>(second==0),"Tidus confirms only left opening0");check(gagQuery(0x2b1c).result==static_cast<unsigned>(second!=0),"Tidus uses original Left query to select blue opening");}
    }
    gagLoad();gagAutomatic=true;check(!gagQuery(0x279c).overrideResult,"Back remains native and is never auto-accepted");
    gagFocus=false;check(!gagQuery(0x24e2).overrideResult,"focus loss disables automatic input");gagFocus=true;
    auto q=gagQuery(0x24e2);check(q.overrideResult,"qualified active first-trial input resumes");
    ++epoch;check(!gagPredicate(0x2550),"stale shot receipt cannot affect native hit test after generation change");
    gagLoad();gagCues=true;const auto before=replies.size();gagQuery(0x24e2);gagQuery(0x24e2);
    check(replies.size()==before+1,"one native ready phase speaks once; repeated input polling is not a new menu entry");
    check(gagazetCommand(L"guide").find(L"Green")==std::wstring::npos && gagazetCommand(L"guide").find(L"green opening")!=std::wstring::npos,"manual guide includes asset-backed opening mapping");
    check(gagazetCommand(L"guide").find(L"The next path opens in Mountain Cave")!=std::wstring::npos,
        "trial guidance explains the original cave return between disconnected water sections");
    gagLoad(7);setValue(0x56000008,1,1);gagQuery(0x28c7);check(gagazetCommand(L"selected").find(L"Green opening")!=std::wstring::npos,"native Rikku focus is available for explicit review");
    setValue(0x010000d8,3,3,0);check(gagazetCommand(L"status").find(L"Second trial: complete")!=std::wstring::npos,"status uses actual native route-completion bits");
    resetGagazet();gagAutomatic=false;gagCues=gagFocus=true;
    puts("PASS: original Gagazet120-byte integer reader/123-byte operand push and actual hit/selection predicates; automatic native input/operand scope, manual preservation, partial-choice safety, focus/epoch guards and asset-backed guide. Animation/camera/timing remain controlled dependencies.");
}
}
