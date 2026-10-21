#include "../log_capture.hpp"
#include <atomic>
#include <cstdlib>
#include <new>
#include <stdexcept>
namespace {
thread_local bool watched=false;
std::atomic<unsigned> allocations{0};
ffx::LogCapture captured[2];
unsigned captures=0;
void accept(const ffx::LogCapture& packet) noexcept {captured[captures++%2]=packet;}
void require(bool value,const char* why) {if(!value) throw std::runtime_error(why);}
}
void* operator new(size_t size) {if(watched) ++allocations;if(auto p=std::malloc(size?size:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace ffx {void log(const char*,...) {std::abort();}}
int main() {
    try {
        ffx::logCaptureHandler.store(&accept);
        char narrow[]="owned narrow";wchar_t wide[]=L"owned wide";
        const auto pointer=static_cast<void*>(narrow);
        char expected[4096]{},actual[4096]{};
        _snprintf_s(expected,sizeof(expected),_TRUNCATE,"%d %u %lld %llu %.3f %c %ls %s %p",
            -19,27u,-9000000000ll,18000000000ull,1.375,'Z',wide,narrow,pointer);
        watched=true;
        ffx::recordLog("%d %u %lld %llu %.3f %c %ls %s %p",-19,27u,-9000000000ll,18000000000ull,1.375,'Z',wide,narrow,pointer);
        watched=false;
        strcpy_s(narrow,"changed");wcscpy_s(wide,L"changed");
        require(allocations==0,"native typed capture allocated");
        ffx::formatLogCapture(captured[0],actual,sizeof(actual));
        require(!strcmp(expected,actual),"mixed scalar/string/pointer formatting changed");
        char bounded[4]={'A','B','C','D'};
        ffx::recordLog("%.*s",4,bounded);
        memset(bounded,'X',sizeof(bounded));
        ffx::formatLogCapture(captured[1],actual,sizeof(actual));
        require(!strcmp(actual,"ABCD"),"non-terminated array lost its final byte");
        ffx::recordLog("equal %u",37u);ffx::recordLog("equal %u",37u);
        char second[4096]{};ffx::formatLogCapture(captured[0],actual,sizeof(actual));
        ffx::formatLogCapture(captured[1],second,sizeof(second));
        require(!strcmp(actual,"equal 37") && !strcmp(actual,second),"equal distinct log events lost");
        const auto inaccessible=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
        require(inaccessible!=nullptr,"guard page unavailable");
        auto bad=static_cast<const char*>(inaccessible);
        ffx::recordLog("%s",bad);ffx::formatLogCapture(captured[0],actual,sizeof(actual));
        require(!*actual,"faulted borrowed string reached worker formatting");
        ffx::recordLog("%p",bad);ffx::formatLogCapture(captured[1],actual,sizeof(actual));
        _snprintf_s(expected,sizeof(expected),_TRUNCATE,"%p",bad);
        require(!strcmp(expected,actual),"opaque pointer address changed");
        const auto before=captures;ffx::logCaptureHandler.store(nullptr);
        ffx::recordLog(bad,bad);
        require(captures==before,"unavailable logger accessed borrowed arguments or formatted on caller");
        VirtualFree(inaccessible,0,MEM_RELEASE);
        puts("PASS: zero-allocation owned typed log capture, formatting parity, caller reuse, equal events and fault/pointer isolation.");return 0;
    } catch(const std::exception& error) {fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
