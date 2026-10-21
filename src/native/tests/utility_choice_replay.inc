#include "collision_replay_bytes.inc"
namespace {
std::array<uint8_t,64> choiceWindow{};
std::array<unsigned,16> choiceStack{};unsigned choiceTop=0,choiceClosed=0;
uintptr_t __cdecl choiceGetWindow(unsigned id) {check(id==1,"original owned choice window identity");return reinterpret_cast<uintptr_t>(choiceWindow.data());}
int __cdecl choiceWaiting(unsigned id) {check(id==1,"original waiting choice window");return 0;}
uintptr_t __cdecl choiceOptions(unsigned) {static uint8_t option=0;return reinterpret_cast<uintptr_t>(&option);}
int __cdecl choiceCount(unsigned) {return 0;}
void __cdecl choiceClose(unsigned id) {check(id==1,"original ret-int closes the actual window");++choiceClosed;write<uint16_t>(reinterpret_cast<uintptr_t>(choiceWindow.data())+0x14,0);}
void __cdecl choicePush(uintptr_t,uintptr_t,unsigned value) {check(choiceTop<choiceStack.size(),"original choice stack bound");choiceStack[choiceTop++]=value;}
unsigned __cdecl choicePop(uintptr_t,uintptr_t) {check(choiceTop>0,"original choice pops its restored window operand");return choiceStack[--choiceTop];}
template<size_t N,size_t M> uintptr_t choiceReplayCode(const unsigned char (&body)[N],const CollisionReplayFixup (&fixups)[M]) {
    auto* code=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));check(code!=nullptr,"original choice replay allocation");std::memcpy(code,body,N);
    for(const auto& fix:fixups) {
        uintptr_t target=0;
        switch(fix.va) {
        case 0x86be20:target=reinterpret_cast<uintptr_t>(choiceGetWindow);break;
        case 0x8b7c50:case 0x8b7f60:target=reinterpret_cast<uintptr_t>(choiceWaiting);break;
        case 0x8b7f80:target=reinterpret_cast<uintptr_t>(choiceOptions);break;
        case 0x8b7fa0:target=reinterpret_cast<uintptr_t>(choiceCount);break;
        case 0x864090:target=reinterpret_cast<uintptr_t>(choiceClose);break;
        case 0x86e380:target=reinterpret_cast<uintptr_t>(choicePush);break;
        case 0x86de90:target=reinterpret_cast<uintptr_t>(choicePop);break;
        }
        check(target!=0 && fix.relative,"original choice has no unmodeled dependency");const auto value=target-(reinterpret_cast<uintptr_t>(code)+fix.offset+4);std::memcpy(code+fix.offset,&value,4);
    }
    FlushInstructionCache(GetCurrentProcess(),code,N);return reinterpret_cast<uintptr_t>(code);
}
void replayWaitingChoice(Event& event) {
    static const auto exec=choiceReplayCode(choiceExecBody,choiceExecFixups);
    choiceTop=choiceClosed=0;choiceWindow.fill(0);write<uint16_t>(reinterpret_cast<uintptr_t>(choiceWindow.data())+0x14,2);
    using Exec=unsigned(__cdecl*)(uintptr_t,uintptr_t,uintptr_t);
    event.result=reinterpret_cast<Exec>(exec)(event.args[0],event.args[1],0);
    check(event.result==0 && choiceClosed==0,"original167-byte choice exec waits for input without closing");
}
void replayAcceptedChoice(Event& event) {
    static const auto ret=choiceReplayCode(choiceReturnBody,choiceReturnFixups);
    using Return=int(__cdecl*)(uintptr_t,uintptr_t,uintptr_t);
    check(event.result==1 && event.overrideResult,"production adapter completes owned native choice");
    check(reinterpret_cast<Return>(ret)(event.args[0],event.args[1],0)==0 && choiceClosed==1 && choiceTop==0,
        "original94-byte ret-int returns Insert and performs native menu/operand cleanup");
}
}
