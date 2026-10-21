#include "../wakka.hpp"
#include "../settings.hpp"
#include "../profile.hpp"
#include "../owned_retirement.hpp"
#include "ready_speech_adapter.hpp"
#include "../ready_game_text.hpp"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <map>
namespace ffx {
std::atomic<int> scene{0};
std::atomic<unsigned> epoch{1};
unsigned calls=0,beeps=0;
bool testForeground=true,battle=true,owner=true,captured=false,actorAvailable=true;
uint16_t clockWords[]={2,0x5ac3};
Handler tableListener=nullptr,effectListener=nullptr;
std::vector<NativeAction> actions;
bool deferNativeAction(NativeAction action) {
    if(!owner || actions.size()>=8) return false;
    actions.push_back(std::move(action)); return true;
}
void finishActions() {
    auto pending=std::move(actions); actions.clear();
    for(auto& action:pending) if(action.epoch==generation()) {
        const bool ready=action.prepare(action);
        if(ready) action.invoke(action);
        if(action.epoch==generation()) action.complete(action,ready);
    }
}
void testWakkaPollIdentity();
bool testWakkaIdentityPending();
std::atomic<bool> holdIdentity{false};
HANDLE identityStarted=nullptr,identityRelease=nullptr;
void wakkaIdentityBeforeFileForTest() {
    if(holdIdentity.exchange(false)) {SetEvent(identityStarted);if(WaitForSingleObject(identityRelease,5000)!=WAIT_OBJECT_0) std::abort();}
}
void finishIdentity() {
    const auto start=GetTickCount64();
    do {testWakkaPollIdentity();if(!testWakkaIdentityPending()) return;Sleep(1);} while(GetTickCount64()-start<5000);
    std::fputs("FAIL: offline identity reply timeout\n",stderr);std::exit(1);
}
void obtained(Event& event) { tableListener(event);finishIdentity(); }
void created(Event& event) { effectListener(event);finishIdentity(); }
uintptr_t globalTable=0,characterTable=0;
alignas(4) uint8_t characterBytes[0x1000]{};
uintptr_t __cdecl character(unsigned id) { if(id!=4) abort(); if(!actorAvailable) return 0; write<uintptr_t>(reinterpret_cast<uintptr_t>(characterBytes)+0xf8c,characterTable); return reinterpret_cast<uintptr_t>(characterBytes); }
std::wstring spoken;
float frequency=0;
unsigned generation() { return epoch; }
bool nativeGameThread() { return owner; }
bool inBattle() { return battle; }
bool utilityConsoleActive() { return false; }
bool modMenuActive() { return wakkaMenuActive(); }
void captureSpeech(bool value) { captured=value; }
void drainController() {}
void log(const char*,...) {}
bool output(const std::wstring& text,bool,const char* producer) {
    // Match the real speech worker's modal filter, rather than accepting
    // battle speech that Tolk never receives while a choice menu is paused.
    if(captured && (!producer || std::strncmp(producer,"mod-",4)!=0)) return false;
    spoken=text; return true;
}
void navAudioOverdriveBeep(float value,unsigned) { ++beeps; frequency=value; }
void navAudioCancelOverdriveBeep() {}
bool copy(uintptr_t source,void* target,size_t size) { __try { memcpy(target,reinterpret_cast<void*>(source),size); return true; } __except(EXCEPTION_EXECUTE_HANDLER) { return false; } }
uintptr_t address(unsigned rva) {
    if(rva==0xf3f73c) return reinterpret_cast<uintptr_t>(clockWords);
    if(rva==wakka_rva::GlobalTable) return reinterpret_cast<uintptr_t>(&globalTable);
    if(rva==0x394030) return reinterpret_cast<uintptr_t>(character);
    abort();
}
bool hook(unsigned rva,Handler before,Handler after) {
    if(before) abort();
    if(rva==0x5da370) tableListener=after;
    else if(rva==wakka_rva::EffectId) effectListener=after;
    else if(rva==0x225930) {}
    else abort();
    return true;
}
std::wstring recordName(unsigned getter,unsigned id) {
    if(getter!=0x39a4c0) abort();
    switch(id) {
    case 0x30f0:return L"Fire Shot"; case 0x30f2:return L"Ice Shot";
    case 0x30f4:return L"Water Shot"; case 0x30f6:return L"Thunder Shot";
    case 0x30f8:return L"Havoc Shot"; case 0x30fa:return L"Time Shot";
    case 0x30fc:return L"Break Shot"; case 0x30fd:return L"Aurochs Shot"; case 0x30fe:return L"Power Shot";
    case 0x312e:return L"Attack Reels"; default:abort();
    }
}
// Fixture-only semantic roots. This deliberately uses owned strings so the
// adapter proves labels and concatenation semantics, not Native allocation or
// production TextPlan performance.
std::map<TextRef,std::wstring> wakkaFixtureText;
TextRef wakkaFixtureNextText=1;
TextRef keepWakkaFixtureText(std::wstring text) {
    const auto root=wakkaFixtureNextText++;
    wakkaFixtureText.emplace(root,std::move(text));
    return root;
}
void prepareTextPlan(TextPlan& plan,const char* producer,bool interrupt) {
    wakkaFixtureText.clear();wakkaFixtureNextText=1;
    plan=TextPlan{};plan.epoch=generation();plan.interrupt=interrupt;
    unsigned i=0;
    if(producer) while(i+1<sizeof(plan.producer) && producer[i]) {plan.producer[i]=producer[i];++i;}
    plan.producer[i]=0;
}
TextRef planRecordName(TextPlan&,unsigned getter,unsigned id) {
    return keepWakkaFixtureText(recordName(getter,id));
}
TextRef planLiteral(TextPlan&,const wchar_t* text) {
    return keepWakkaFixtureText(text?std::wstring(text):std::wstring());
}
TextRef planConcat(TextPlan&,TextRef left,TextRef right) {
    const auto l=wakkaFixtureText.find(left),r=wakkaFixtureText.find(right);
    if(l==wakkaFixtureText.end() || r==wakkaFixtureText.end()) return 0;
    return keepWakkaFixtureText(l->second+r->second);
}
bool commitTextPlan(const TextPlan& plan) {
    if(plan.epoch!=generation() || !plan.headline) return false;
    const auto text=wakkaFixtureText.find(plan.headline);
    return text!=wakkaFixtureText.end() && output(text->second,plan.interrupt,plan.producer);
}
}
using namespace ffx;
void check(bool value,const char* label) { if(!value) { fprintf(stderr,"FAIL: %s\n",label); exit(1); } }
uintptr_t transitionTable=0,transitionControl=0;
int nativeCallback() {
    ++calls;
    if(transitionTable) { write<uintptr_t>(transitionTable+0x10,transitionControl); transitionTable=0; }
    return 37;
}
uintptr_t mappedImage(const wchar_t* path) {
    std::ifstream file(path,std::ios::binary); std::vector<char> data((std::istreambuf_iterator<char>(file)),{});
    check(data.size()>4096,"copied offline PE evidence exists");
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(data.data());
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(data.data()+dos->e_lfanew);
    check(nt->Signature==IMAGE_NT_SIGNATURE && nt->FileHeader.Machine==IMAGE_FILE_MACHINE_I386,"actual x86 Wakka PE fixture");
    const auto base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,nt->OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    check(base!=0,"non-executable module byte mapping");
    memcpy(reinterpret_cast<void*>(base),data.data(),nt->OptionalHeader.SizeOfHeaders);
    const auto sections=IMAGE_FIRST_SECTION(nt);
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i) if(sections[i].SizeOfRawData)
        memcpy(reinterpret_cast<void*>(base+sections[i].VirtualAddress),data.data()+sections[i].PointerToRawData,sections[i].SizeOfRawData);
    const auto reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    for(unsigned offset=0;offset<reloc.Size;) {
        const auto block=reinterpret_cast<const IMAGE_BASE_RELOCATION*>(base+reloc.VirtualAddress+offset);
        const auto entries=reinterpret_cast<const uint16_t*>(block+1);
        for(unsigned i=0;i<(block->SizeOfBlock-8)/2;++i) if((entries[i]>>12)==IMAGE_REL_BASED_HIGHLOW)
            *reinterpret_cast<uintptr_t*>(base+block->VirtualAddress+(entries[i]&0xfff))+=base-nt->OptionalHeader.ImageBase;
        offset+=block->SizeOfBlock;
    }
    return base;
}
unsigned symbolAt(uintptr_t reel) {
    const auto pattern=read<uintptr_t>(reel),count=read<unsigned>(pattern),bytes=read<uintptr_t>(pattern+4);
    return read<uint8_t>(bytes+((read<unsigned>(reel+4)>>16)+1)%count);
}
void cases(const wchar_t* path) {
    const auto base=mappedImage(path);
    const auto filename=wcsrchr(path,L'_'); check(filename!=nullptr,"native module filename");
    const auto number=wcstoul(filename+1,nullptr,10);
    const bool hd=number==644 || (number>=691 && number<=698 && number%2==0),observed=number==697,moved=number==698;
    const unsigned table=hd?(moved?0x144020:0xe190):observed?0xeb38:0xeb40,begin=hd?0x5210:0x1000,control=hd?0x5480:0x1270,running=hd?0x5550:0x1340;
    const unsigned windows=hd?0x14c460:0x14cf28,set=hd?0x14c650:0x14d118,count=hd?0x14c654:0x14d11c;
    const unsigned sequence=hd?0x14c658:0x14d120,actor=hd?0x331a44:0x14d184;
    const unsigned patterns=hd?(moved?0x1441ec:0xe340):observed?0xeae0:0xeae8;
    const unsigned getter=hd?0x4280:0xadb0;
    check(read<uint8_t>(base+getter+8)==0xb8 && read<uintptr_t>(base+getter+9)==base+table,"actual exported getter returns the qualified table; export is never executed");
    testWakkaModule(base,path,nativeCallback); Event event; event.result=base+table;
    write<uintptr_t>(base+table,base+begin+1); obtained(event);
    check(read<uintptr_t>(base+table+0x10)==base+control,"changed begin callback refuses attachment"); write<uintptr_t>(base+table,base+begin);
    write<uintptr_t>(base+table+0x10,base+control+1); obtained(event);
    check(read<uintptr_t>(base+table)==base+begin,"changed control callback refuses attachment"); write<uintptr_t>(base+table+0x10,base+control);
    Event wrong; wrong.result=base+table+8; obtained(wrong);
    check(read<uintptr_t>(base+table)==base+begin && read<uintptr_t>(base+table+0x10)==base+control,"nearby pointer cannot bypass exact table/base qualification");
    wchar_t executablePath[MAX_PATH]{}; check(GetModuleFileNameW(nullptr,executablePath,MAX_PATH)!=0,"fixture executable identity");
    testWakkaModule(base,executablePath,nativeCallback); obtained(event);
    check(read<uintptr_t>(base+table+0x10)==base+control,"unsupported exact file hash refuses attachment"); testWakkaModule(base,path,nativeCallback);
    owner=false; obtained(event); owner=true;
    check(read<uintptr_t>(base+table+0x10)==base+control,"foreign loader getter cannot mutate native callback tables");
    holdIdentity=true;ResetEvent(identityStarted);ResetEvent(identityRelease);
    const auto identityStart=GetTickCount64();tableListener(event);
    check(testWakkaIdentityPending() && read<uintptr_t>(base+table+0x10)==base+control && GetTickCount64()-identityStart<200,"native qualification publishes without waiting for filename/hash worker");
    check(WaitForSingleObject(identityStarted,5000)==WAIT_OBJECT_0,"identity file worker held");
    ++epoch;SetEvent(identityRelease);finishIdentity();
    check(read<uintptr_t>(base+table+0x10)==base+control,"resource transition retires queued module qualification");
    testWakkaModule(base,path,nativeCallback);
    tableListener(event); resetWakka(); finishIdentity();
    check(read<uintptr_t>(base+table+0x10)==base+control,"cancelled qualification cannot later wrap a callback");
    globalTable=characterTable=base+table;
    write<uint16_t>(reinterpret_cast<uintptr_t>(characterBytes)+0xf5c,0x3000);
    Event ordinary; ordinary.result=0x120204; created(ordinary);
    check(read<uintptr_t>(base+table+0x10)==base+control,"ordinary Wakka effect activation does not attach reel assistance");
    write<uint16_t>(reinterpret_cast<uintptr_t>(characterBytes)+0xf5c,0x3074);
    Event start; start.result=0x128004;
    actorAvailable=false; created(start); actorAvailable=true;
    battle=false; created(start); battle=true;
    owner=false; created(start); owner=true;
    check(read<uintptr_t>(base+table+0x10)==base+control,"missing actor, outside battle and foreign start refuse attachment");
    created(start);
    check(read<uintptr_t>(base+table+0x10)!=base+control,"actual global effect start discovers a table skipped by foreign loader");
    check(read<uintptr_t>(base+table+0x10)!=base+control && read<uintptr_t>(base+table)!=base+begin,"verified module callback wrapping");
    transitionTable=base+table; transitionControl=base+running;
    testWakkaControl();
    check(!transitionTable && read<uintptr_t>(base+table+0x10)!=base+running,"native initialization switches to running callback without losing assistance");
    const auto win=base+windows;
    write<unsigned>(base+set,0); write<unsigned>(base+count,1); write<unsigned>(base+sequence,1);
    // Actual MsMakeEffId(serial,type,character) handles are not bare IDs.
    write<unsigned>(base+actor,0x128004);
    write<unsigned>(win+0x68,1); write<unsigned>(win+0x64,0);
    for(unsigned col=0;col<3;++col) { write<uintptr_t>(win+col*0x1c,base+patterns+col*8); write<unsigned>(win+col*0x1c+4,0); write<unsigned>(win+col*0x1c+0x18,2); }
    clockWords[0]=2; setSetting(Setting::OverdriveAssist,1); const auto before=calls;
    write<unsigned>(base+actor,0x128005);
    testWakkaControl(); check(!wakkaMenuActive() && clockWords[0]==2 && calls==before+1,"packed handle for another character cannot open Wakka choices");
    write<unsigned>(base+actor,0x128004); const auto choosing=calls;
    testWakkaControl(); check(wakkaMenuActive() && captured && clockWords[0]==1 && calls==choosing,"choice freezes native sequence and countdown");
    testWakkaControl(); check(calls==choosing,"paused choice does not execute native control callback");
    const auto forwarded=calls;
    owner=false; testWakkaControl();testWakkaBegin(4);
    check(calls==forwarded+2 && wakkaMenuActive() && captured && clockWords[0]==1,
        "foreign wrappers forward native stubs without waiting or touching owner choice/timer state");owner=true;
    owner=false; wakkaMenuKey(VK_RETURN); check(wakkaMenuActive() && clockWords[0]==1,"foreign-thread key cannot mutate paused native state"); owner=true;
    wakkaMenuKey(VK_END); check(spoken==L"Thunder Shot","native valid Element matches and last choice");
    wakkaMenuKey(VK_HOME); check(spoken==L"Fire Shot","first native match");
    wakkaMenuKey(VK_DOWN); check(spoken==L"Ice Shot","next native match");
    wakkaMenuKey(VK_RETURN); check(!wakkaMenuActive() && !captured && clockWords[0]==2 && clockWords[1]==0x5ac3,"Confirm resumes exact clock word and preserves adjacent field");
    for(unsigned col=0;col<3;++col) check(symbolAt(win+col*0x1c)==1 && read<unsigned>(win+col*0x1c+0x18)==0,"choice sets only matching accepted native symbol positions");
    check(read<unsigned>(win+0x64)==3 && testWakkaControl()==37 && calls==forwarded+3,"native match/result continuation executes normally");
    setSetting(Setting::OverdriveAssist,0); setSetting(Setting::OverdriveCues,1);
    write<unsigned>(win+0x64,0); write<unsigned>(win+0x18,2); write<unsigned>(win+4,0);
    const auto oldBeeps=beeps; testWakkaControl(); check(beeps==oldBeeps+1 && frequency==660,"manual cue identifies the symbol native Confirm accepts");
    testWakkaControl(); check(beeps==oldBeeps+1,"one symbol traversal has one short beep");
    write<unsigned>(win+4,1u<<16); testWakkaControl(); check(beeps==oldBeeps+2,"new native selectable symbol produces its own beep");
    setSetting(Setting::OverdriveCues,0); write<unsigned>(win+4,2u<<16); testWakkaControl(); check(beeps==oldBeeps+2,"manual beep toggle is authoritative");
    resetWakka(); check(read<uintptr_t>(base+table+0x10)==base+running && read<uintptr_t>(base+table)==base+begin,"reset restores both native callbacks including the running transition");
    testWakkaModule(base,path,nativeCallback); obtained(event); setSetting(Setting::OverdriveAssist,1); testWakkaControl();
    check(wakkaMenuActive(),"a new cast opens its choice again");
    wakkaMenuKey(VK_ESCAPE); check(!wakkaMenuActive() && clockWords[0]==2,"Escape resumes manual reels");
    testWakkaControl(); check(!wakkaMenuActive(),"cancel does not reopen during the same cast");
    const unsigned typePatterns[]={patterns,hd?(moved?0x144338u:0xe48cu):observed?0xec6cu:0xec74u,hd?(moved?0x144ea0u:0xeff4u):observed?0xeed0u:0xeed8u,hd?(moved?0x144320u:0xe474u):observed?0xec54u:0xec5cu};
    const unsigned lastSymbols[]={3,6,9,10};
    for(unsigned type=0;type<4;++type) {
        const auto handle=((type+1)<<16)|0x8004;
        write<uint16_t>(reinterpret_cast<uintptr_t>(characterBytes)+0xf5c,static_cast<uint16_t>(0x3074+type));
        resetWakka(); testWakkaModule(base,path,nativeCallback); start.result=handle; created(start);
        check(read<uintptr_t>(base+table+0x10)!=base+running,"all four exact native reel commands attach at effect activation");
        write<unsigned>(base+actor,handle); testWakkaBegin(handle);
        for(unsigned col=0;col<3;++col) write<uintptr_t>(win+col*0x1c,base+typePatterns[type]+col*8);
        testWakkaControl();
        if(type==1) check(!wakkaMenuActive() && clockWords[0]==2,"only Attack Reels applies its automatic 12-hit match without a list");
        else {
            check(wakkaMenuActive() && clockWords[0]==1,"Element, Status and Aurochs reels pause for selectable matches");
            if(type==2) {
                check(spoken==L"Match reels. Havoc Shot","first native Status result");
                wakkaMenuKey(VK_DOWN); check(spoken==L"Time Shot","second native Status result");
            }
            wakkaMenuKey(VK_END);
            if(type==2) check(spoken==L"Break Shot","last native Status result");
            if(type==3) check(spoken==L"Aurochs Shot","last native Aurochs choice has the actual result name");
            wakkaMenuKey(VK_RETURN);
        }
        for(unsigned col=0;col<3;++col) check(symbolAt(win+col*0x1c)==lastSymbols[type],"Element, Attack, Status and Aurochs match exact native pattern symbols");
    }
    constexpr unsigned aurochsSymbols[]={0,1,2,3,7,8,9,10};
    constexpr const wchar_t* aurochsNames[]={L"Fire Shot",L"Ice Shot",L"Water Shot",L"Thunder Shot",L"Havoc Shot",L"Time Shot",L"Break Shot",L"Aurochs Shot"};
    for(unsigned choice=0;choice<8;++choice) {
        testWakkaBegin(0x48004); clockWords[0]=37;
        write<unsigned>(win+0x64,0);
        for(unsigned col=0;col<3;++col) {
            write<uintptr_t>(win+col*0x1c,base+typePatterns[3]+col*8);
            write<unsigned>(win+col*0x1c+4,0); write<unsigned>(win+col*0x1c+0x18,2);
        }
        const auto pausedCalls=calls;
        testWakkaControl();
        check(wakkaMenuActive() && captured && clockWords[0]==1 && calls==pausedCalls && read<unsigned>(win+0x64)==0,
            "Aurochs activation pauses countdown and native control without accepting the default icon");
        check(spoken==L"Match reels. Fire Shot","every Aurochs cast announces its first native choice");
        for(unsigned col=0;col<3;++col) check(read<unsigned>(win+col*0x1c+0x18)==2,"Aurochs choice leaves all wheels spinning until selection");
        testWakkaControl(); check(calls==pausedCalls && clockWords[0]==1 && clockWords[1]==0x5ac3,"Aurochs list remains paused and preserves adjacent timer state");
        wakkaMenuKey(VK_UP); check(spoken==L"Aurochs Shot","Aurochs list wraps to its last of eight choices");
        wakkaMenuKey(VK_DOWN); check(spoken==L"Fire Shot","Aurochs list wraps back to its first choice");
        for(unsigned row=0;row<choice;++row) wakkaMenuKey(VK_DOWN);
        check(spoken==aurochsNames[choice],"all eight actual elemental/status/Aurochs outcomes can be selected");
        wakkaMenuKey(choice%2?VK_SPACE:VK_RETURN);
        check(!wakkaMenuActive() && !captured && clockWords[0]==37 && clockWords[1]==0x5ac3 && spoken==aurochsNames[choice],
            "Aurochs selection resumes the exact saved countdown and names the chosen result");
        for(unsigned col=0;col<3;++col) check(symbolAt(win+col*0x1c)==aurochsSymbols[choice] && read<unsigned>(win+col*0x1c+0x18)==0,
            "each Aurochs choice stops all three native reels on the requested accepted symbol");
        check(read<unsigned>(win+0x64)==3 && testWakkaControl()==37 && calls==pausedCalls+1 && !wakkaMenuActive(),
            "Aurochs chosen result continues through native callbacks without reopening its list");
    }
    clockWords[0]=2;
    testWakkaBegin(0x48004); testWakkaControl();
    check(wakkaMenuActive(),"a new Aurochs cast opens a new paused list");
    wakkaMenuKey(VK_ESCAPE); testWakkaControl();
    check(!wakkaMenuActive() && !captured && clockWords[0]==2,"cancelled Aurochs cast resumes manual play without reopening the list");
    setSetting(Setting::OverdriveAssist,0); setSetting(Setting::OverdriveCues,1);
    write<unsigned>(win+0x64,0); write<unsigned>(win+0x18,2); write<unsigned>(win+4,0);
    testWakkaControl(); check(frequency==1980,"Aurochs symbol has a distinct highest beep in Aurochs Reels");
    write<unsigned>(win+4,5u<<16); testWakkaControl();
    check(frequency==1320,"Havoc beep is distinct from Fire when both occur in Aurochs Reels");
    write<uintptr_t>(win,base+typePatterns[2]); write<unsigned>(win+4,0);
    const auto repeatBeeps=beeps; testWakkaControl(); write<unsigned>(win+4,1u<<16); testWakkaControl();
    check(beeps==repeatBeeps+2 && frequency==1320,"adjacent identical icons each get their own native traversal beep");
    testWakkaBegin(4);
    for(unsigned col=0;col<3;++col) write<uintptr_t>(win+col*0x1c,base+typePatterns[1]+col*8);
    const auto attackPattern=read<uintptr_t>(win),attackCount=read<unsigned>(attackPattern),attackBytes=read<uintptr_t>(attackPattern+4);
    for(unsigned icon=0;icon<attackCount;++icon) {
        write<unsigned>(win+4,icon<<16);
        const auto symbol=read<uint8_t>(attackBytes+(icon+1)%attackCount);
        const auto beforeBeep=beeps;
        testWakkaControl();
        check(beeps==beforeBeep+(symbol==5 || symbol==6?1u:0u),"Attack Reels manual cues sound only for attack icons, with blank icons silent");
    }
    for(unsigned col=0;col<3;++col) write<uintptr_t>(win+col*0x1c,base+typePatterns[0]+col*8);
    setSetting(Setting::OverdriveAssist,1);
    testWakkaBegin(4); testWakkaControl(); testForeground=false; testWakkaControl();
    check(!wakkaMenuActive() && clockWords[0]==2,"focus loss releases the paused native sequence"); testForeground=true;
    testWakkaBegin(4); testWakkaControl(); owner=false; resetWakka();
    check(wakkaMenuActive() && clockWords[0]==1,"foreign-thread reset defers native mutations"); owner=true; testWakkaControl();
    check(!wakkaMenuActive() && clockWords[0]==2,"owner callback completes deferred cancellation safely");
    testWakkaModule(base,path,nativeCallback); obtained(event);
    resetWakka(); testWakkaModule(base,path,nativeCallback); start.result=0x120204; created(start);
    check(read<uintptr_t>(base+table+0x10)!=base+running,"actual per-character effect start attaches its consumed callback table");
    resetWakka(); testWakkaModule(base,path,nativeCallback); obtained(event); testWakkaControl(); ++epoch;
    testWakkaControl(); check(!wakkaMenuActive() && clockWords[0]==2,"generation invalidation safely resumes an owned paused sequence");
    resetWakka(); VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);
}
int wmain(int argc,wchar_t** argv) {
    check(argc==11,"all ten copied module paths required");
    identityStarted=CreateEventW(nullptr,TRUE,FALSE,nullptr);identityRelease=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(identityStarted && identityRelease,"identity worker fixture events");
    retirement::start(nullptr);installWakka();
    const auto executable=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    check(testWakkaLoadedModule(executable)==executable && !testWakkaLoadedModule(executable+0x1000),"loaded module ownership uses its exact PE base independently of its filename");
    for(int i=1;i<argc;++i) cases(argv[i]);
    puts("PASS: all ten real Wakka variants, activation and identity refusal, eight paused Aurochs choices, Element/Status choices, Attack-only automatic 12 hits, manual cues, countdown resume, cancellation and callback restoration; no DLL entry point executed.");
}
