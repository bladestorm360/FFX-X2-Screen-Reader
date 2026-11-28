// Standalone fixture: the battle resume target is handed over without a lock or allocation.
#include "../nav_resume.hpp"
#include <cstdio>
#include <cstdlib>
#include <new>

// Counts heap allocations so the handoff can prove it allocates nothing.
static unsigned allocations=0;
void* operator new(std::size_t size) {++allocations;if(void* p=std::malloc(size?size:1)) return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}

namespace {
void check(bool ok,const char* what) {if(!ok) {std::printf("FAIL: %s\n",what);std::exit(1);}}
}

int main() {
    static ffx::nav::ResumeMailbox mailbox;
    const wchar_t name[]=L"Save Sphere";
    const unsigned before=allocations;
    mailbox.publish(name,7,0,{1,2,3});
    check(allocations==before,"publishing the battle resume target allocates nothing");
    ffx::nav::ResumeRequest got;
    check(mailbox.take(got) && std::wstring(got.name.data())==L"Save Sphere" && got.id==7 && got.position.z==3,"the preparation owner reads the published resume target");
    std::wstring longName(3000,L'x');mailbox.publish(longName.c_str(),1,0,{});
    check(mailbox.take(got) && std::wcslen(got.name.data())==got.name.size()-1,"an over-long name is cut to the fixed buffer");
    std::puts("PASS: battle resume target handoff allocates nothing and cuts over-long names.");
}
