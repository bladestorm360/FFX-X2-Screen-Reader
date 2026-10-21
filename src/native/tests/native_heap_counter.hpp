#pragma once
#include <new>
#include <cstdlib>
namespace nativeHeapFixture {inline bool observing=false;inline unsigned allocations=0,frees=0;}
void* operator new(size_t bytes) {if(nativeHeapFixture::observing) ++nativeHeapFixture::allocations;if(auto* p=std::malloc(bytes?bytes:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t bytes) {return ::operator new(bytes);}
void operator delete(void* p) noexcept {if(p && nativeHeapFixture::observing) ++nativeHeapFixture::frees;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
