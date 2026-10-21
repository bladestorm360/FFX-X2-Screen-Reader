#include "../prism_backend.hpp"
#include <windows.h>
#include <cstdio>
#include <stdexcept>
#include <string>

using SetU64=void(__cdecl*)(uint64_t); using SetBool=void(__cdecl*)(bool);
using SetU32=void(__cdecl*)(uint32_t); using SetAvailability=void(__cdecl*)(bool,bool);
using Counter=int(__cdecl*)(int); using Count=int(__cdecl*)(); using Read=DWORD(__cdecl*)(int,wchar_t*,unsigned);
using Lifecycle=int(__cdecl*)(int); using LifeCount=int(__cdecl*)(); using Thread=DWORD(__cdecl*)();
static void require(bool ok,const char* why) { if(!ok)throw std::runtime_error(why); }
static FARPROC proc(HMODULE m,const char* n) { auto p=GetProcAddress(m,n);require(p!=nullptr,n);return p; }
static void check(const wchar_t* text,int index,Read read,Count count) {
 wchar_t got[256]{};require(index<count(),"missing output record");DWORD caller=read(index,got,256);require(caller==GetCurrentThreadId(),"output call left the caller thread");require(wcscmp(got,text)==0,"output text mismatch");
}
int wmain(int argc,wchar_t** argv) {
 try {
  wchar_t absolute[32768]{}; require(GetFullPathNameW(argc>1?argv[1]:L".",32768,absolute,nullptr)!=0,"fixture absolute path");
  std::wstring root=absolute; if(root.back()!=L'\\')root+=L'\\';
  auto fake=LoadLibraryW((root+L"PrismRuntime\\prism.dll").c_str());require(fake!=nullptr,"fake prism load");
  auto setFeatures=(SetU64)proc(fake,"Test_SetFeatures");auto setFail=(SetBool)proc(fake,"Test_SetInitFailure");
  auto setVersion=(SetU32)proc(fake,"Test_SetVersion");auto setAvail=(SetAvailability)proc(fake,"Test_SetAvailability");
  auto counter=(Counter)proc(fake,"Test_Counter");auto count=(Count)proc(fake,"Test_Count");auto read=(Read)proc(fake,"Test_Read");
  auto lifecycle=(Lifecycle)proc(fake,"Test_Lifecycle");auto lifeCount=(LifeCount)proc(fake,"Test_LifecycleCount");auto thread=(Thread)proc(fake,"Test_LastLifecycleThread");
  setVersion(0x1203);setFail(false);setAvail(true,false);setFeatures(PRISM_BACKEND_IS_SUPPORTED_AT_RUNTIME|PRISM_BACKEND_SUPPORTS_SPEAK|PRISM_BACKEND_SUPPORTS_BRAILLE|PRISM_BACKEND_SUPPORTS_OUTPUT|PRISM_BACKEND_SUPPORTS_STOP);
  {
   ffx::PrismSpeechBackend b;require(b.open(root,0),"automatic open");
   require(counter(0)==1&&counter(2)==0,"automatic backend must not initialize twice");
   require(b.output(L"Za\u017c\u00f3\u0142\u0107 \xD83D\xDE42",true),"UTF-8 Polish and supplementary output");
   require(b.output(L"Repeated event",false)&&b.output(L"Repeated event",false),"repeated valid events");
   require(count()==3 && counter(4)==3 && counter(5)==0 && counter(6)==0,"combined output uses one exact channel call");check(L"Za\u017c\u00f3\u0142\u0107 \xD83D\xDE42",0,read,count);check(L"Repeated event",2,read,count);
   wchar_t invalid[]={0xD800,0};require(!b.output(invalid,false),"unpaired surrogate must reject");require(count()==3,"invalid UTF-16 reached Prism");
   require(b.stop(),"supported stop");require(counter(7)==1,"supported stop called once");
   int beforeClose=lifeCount();b.close();require(lifecycle(beforeClose)==4&&lifecycle(beforeClose+1)==2,"backend freed before shutdown");
   require(thread()==GetCurrentThreadId(),"adapter lifecycle thread changed");
  }
  {
   ffx::PrismSpeechBackend b;require(b.open(root,3),"explicit SAPI open");
   require(counter(2)==1,"explicit backend initialized exactly once");b.close();
  }
  {
   ffx::PrismSpeechBackend b;require(b.open(root,1),"initial NVDA");auto old=b.name();setAvail(false,false);
   require(!b.choose(2)&&b.name()==old,"unavailable JAWS replaced current backend");b.close();setAvail(true,false);
  }
  {
   setAvail(false,false);setFail(true);int frees=counter(3),downs=counter(1);
   ffx::PrismSpeechBackend b;require(!b.open(root,3),"init failure unexpectedly opened");
   require(counter(3)==frees+1&&counter(1)==downs+1,"init failure did not free candidate and shut context");
   setFail(false);setAvail(true,false);
  }
  {
   setVersion(0x1204);int downs=counter(1);ffx::PrismSpeechBackend b;
   require(!b.open(root,0),"version mismatch opened");require(counter(1)==downs,"version mismatch initialized context");setVersion(0x1203);
  }
  {
   setFeatures(PRISM_BACKEND_SUPPORTS_SPEAK|PRISM_BACKEND_SUPPORTS_BRAILLE);ffx::PrismSpeechBackend b;
   require(b.open(root,0),"fallback backend open");int s=counter(5),br=counter(6),out=counter(4);
   require(b.output(L"Both channels",true),"speech plus braille");
   require(counter(5)==s+1&&counter(6)==br+1&&counter(4)==out,"fallback must call each channel once");
   require(b.output(L"Braille only",false,true),"braille-only supported");
   require(counter(5)==s+1 && counter(6)==br+2,"braille only never calls speech");
   
   b.close();
  }
  {
   setFeatures(PRISM_BACKEND_SUPPORTS_SPEAK);ffx::PrismSpeechBackend b;require(b.open(root,0),"speech-only backend");
   int s=counter(5),br=counter(6),st=counter(7);require(!b.output(L"No braille",false,true),"unsupported braille-only succeeded");
   require(counter(5)==s&&counter(6)==br,"unsupported braille-only called a channel");
   require(!b.stop()&&counter(7)==st,"unsupported stop called Prism");b.close();
  }
  {
   ffx::PrismSpeechBackend b;require(!b.open(root+L"missing\\",0),"missing DLL open succeeded");require(!b.output(L"safe",false),"missing DLL accepted output");
  }
  FreeLibrary(fake);std::puts("Prism adapter fixture passed");return 0;
 } catch(const std::exception& e) { std::fprintf(stderr,"%s\n",e.what());return 1; }
}



