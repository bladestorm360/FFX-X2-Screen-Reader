#include "../utilities.hpp"
#include "../speech_capture.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <new>
thread_local bool native=false;
unsigned allocated=0,freed=0;
void* operator new(size_t n) {if(native) ++allocated;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc{};}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(native && p) ++freed;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace {
std::array<std::array<wchar_t,256>,8> history{},speech{};
unsigned histories=0,spoken=0;
wchar_t over[ffx::speechq::CharacterLimit+2];
void check(bool condition,const char* why) {if(!condition) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
}
namespace ffx {
void consoleReplyReady(const wchar_t* text,size_t n) {
    check(native && n<256 && histories<8,"actual adapter sends only ready bytes to console transport");
    std::wmemcpy(history[histories++].data(),text,n);
}
bool output(const wchar_t* text,bool interrupt,const char* producer) {
    check(native && interrupt && !std::strcmp(producer,"utility-reply") && spoken<8,"actual ready adapter retains producer/interruption");
    wcscpy_s(speech[spoken++].data(),256,text);return true;
}
}
int main() {
    std::wstring owned=L"An already formatted copied response";
    wchar_t source[]=L"Equal utility response";
    native=true;
    ffx::utilityReply(source);ffx::utilityReply(source);ffx::utilityReply(owned);
    std::wmemset(source,L'?',22);owned[0]=L'?';
    ffx::utilityReply(static_cast<const wchar_t*>(nullptr));
    native=false;
    std::wmemset(over,L'x',ffx::speechq::CharacterLimit+1);over[ffx::speechq::CharacterLimit+1]=0;
    native=true;ffx::utilityReply(over);native=false;
    const auto fault=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS);
    check(fault!=nullptr,"guard-page fixture");native=true;ffx::utilityReply(static_cast<const wchar_t*>(fault));native=false;
    VirtualFree(fault,0,MEM_RELEASE);
    check(histories==3 && spoken==3 && !allocated && !freed,"equal ready/caller-owned output has no Native C++ allocation or destruction; malformed/whole overflow refused");
    check(!std::wcscmp(history[0].data(),L"Equal utility response") && !std::wcscmp(history[1].data(),history[0].data()) &&
        !std::wcscmp(speech[2].data(),L"An already formatted copied response"),"caller reuse never changes accepted ready output");
    std::puts("PASS: actual ready utility feedback, equal responses, caller reuse, whole oversize/fault rejection, Native new/delete=0/0; console/speech boundaries mocked.");
}
