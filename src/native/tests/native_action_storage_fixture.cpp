#include "../native_action.hpp"
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <new>
namespace {thread_local bool watched=false;std::atomic<unsigned> allocated{0},deleted{0};}
void* operator new(size_t n) {if(watched) ++allocated;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(p && watched) ++deleted;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
int main() {
    bool okay=true;unsigned invoked=0,completed=0;
    struct Data {unsigned input,result;};
    watched=true;
    {
        ffx::NativeActionQueue queue;
        unsigned caller=17;
        ffx::FixedCallback<bool(),64> qualify=[caller] {return caller==17;};
        ffx::NativeAction action;
        action.prepare=[qualify](ffx::NativeAction& value) mutable {
            if(!qualify()) return false;auto data=value.data<Data>();data.result=data.input+5;value.data(data);return true;
        };
        action.invoke=[&invoked](ffx::NativeAction& value) {if(value.data<Data>().result==22) ++invoked;};
        action.complete=[&completed](bool okay) {if(okay) ++completed;};
        action.data(Data{caller,0});caller=99;
        for(unsigned i=0;i<8;++i) okay=queue.push(action) && okay;
        okay=!queue.push(action) && okay;
        while(!queue.empty()) {auto value=queue.take();const bool ready=value.prepare(value);if(ready)value.invoke(value);value.complete(value,ready);}
        okay=queue.push(action) && okay;auto reused=queue.take();okay=reused.prepare(reused) && okay;
    }
    watched=false;
    okay=okay && invoked==8 && completed==8 && !allocated && !deleted;
    puts(okay?"PASS: owned fixed action/callback capture, caller reuse, exact eight-event pressure/recovery and zero native allocation/destruction.":"FAIL: fixed action storage ownership or allocation.");
    return okay?0:1;
}
