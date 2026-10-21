#include "../runtime.hpp"
#include "../process_storage.hpp"
#include <map>
#include <deque>
#include <cstdlib>
#include <cstdio>
namespace {
std::atomic<unsigned> allocations{0},destructions{0};
thread_local bool native=true;
struct Value {~Value() {++destructions;}};
ffx::ProcessStorage<std::map<unsigned,Value>> dictionary;
ffx::ProcessStorage<std::deque<unsigned>> queue;
DWORD WINAPI bootstrap(void*) {
    native=false;dictionary.prepare();queue.prepare();
    dictionary.get().try_emplace(7);queue.get().push_back(29);return 0;
}
}
void* operator new(size_t size) {if(native) ++allocations;if(auto p=std::malloc(size?size:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
int main() {
    bool okay=allocations==0 && !dictionary.ready() && !queue.ready();
    const auto worker=CreateThread(nullptr,0,bootstrap,nullptr,0,nullptr);
    if(!worker || WaitForSingleObject(worker,2000)!=WAIT_OBJECT_0) return 1;
    CloseHandle(worker);
    okay=okay && dictionary.ready() && queue.ready() && dictionary.get().count(7)==1 && queue.get().front()==29;
    okay=okay && allocations==0;
    {ffx::ProcessStorage<std::deque<Value>> lifetime;native=false;lifetime.prepare();lifetime.get().emplace_back();native=true;}
    // The local prepare above is an explicit fixture owner, not DLL attach;
    // its allocating live value must also avoid a native teardown destructor.
    okay=okay && destructions==0 && allocations==0;
    puts(okay?"PASS: zero-heap cold storage, bootstrap publication and process-lifetime destructor isolation.":"FAIL: allocating cold initializer or native teardown destructor.");
    return okay?0:1;
}
