#include "wakka.hpp"
#include "wakka_identity.hpp"
#include "profile.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "nav_audio.hpp"
#include "utilities.hpp"
#include "controller.hpp"
#include "native_image.hpp"
#include "ready_game_text.hpp"
#include <atomic>


namespace ffx {
namespace {
struct Layout {
    const char* digest;
    unsigned table,begin,initial,control,window,set,count,sequence,actor;
};
constexpr Layout legacy(const char* digest,unsigned table=0xeb40) {
    return {digest,table,0x1000,0x1270,0x1340,0x14cf28,0x14d118,0x14d11c,0x14d120,0x14d184};
}
constexpr Layout hd(const char* digest,unsigned table=0xe190) {
    return {digest,table,0x5210,0x5480,0x5550,0x14c460,0x14c650,0x14c654,0x14c658,0x331a44};
}
constexpr Layout layouts[]={
    legacy("4fba9fe86fd2821830f89263a301fabb95f056afc94459d0fa8809056c2d165e"), //565
    hd("e72045bca7d99cc0e9f28038e66115afad38573c22db6c88cf1d7b5ae23b0ad4"), //644
    legacy("f32eade649473f572d8c3264fc734497a9868944fb101e8eb97ee99817edf5e2"), //691
    hd("2b2239b2d77c7d2c405e403d41b96469953063e1c3649025b587f436f6cc43b7"), //692
    legacy("fbc36be07ab889a533679bc916f2a7ddcdd5f08315797aee8b6ffa6477aae9d0"), //693
    hd("7f31d83adf1612a8285167cb589540c36a1fac02303bb768a3f8069c4ad98db5"), //694
    legacy("56b38bba958696ae30ddbf966b35a4e5f02a846c3e77df0e267474656defb886"), //695
    hd("7cd18cb9ead45ff68ad0861369b8472076c7b2e10e1d57399b0f8f9b7a989b14"), //696
    legacy("3882e56d017d48cba5eb4dbd908b40fa8e702311846e8c59176091a70484ab8d",0xeb38), //697
    hd("576c873b5c8231113ce71c047367808c7d82ea20a9a72e156c0b7ce8b992566f",0x144020) //698
};
std::atomic<uintptr_t> forwardControl{0},forwardBegin{0};
std::atomic<bool> active{false};
std::atomic<bool> cancelRequested{false};
uintptr_t module=0,table=0,original=0,selectedWindow=0;
const Layout* layout=nullptr;
unsigned epoch=0,row=0,lastIcon=255,lastColumn=255,lastSet=255;
uint16_t savedClock=0;
bool chosen=false,dismissed=false;
std::array<unsigned,11> symbols{};unsigned symbolCount=0;
WakkaIdentity pendingQualification;
unsigned identitySerial=0;
#ifdef FFX_READER_TEST
uintptr_t fixtureModule=0;
const wchar_t* fixturePath=nullptr;
int(*fixtureInvoke)()=nullptr;
#endif
int __cdecl controlled();
int __cdecl begun(unsigned actor);
uintptr_t loadedModule(uintptr_t candidate) {
#ifdef FFX_READER_TEST
    if(candidate==fixtureModule) return fixtureModule;
#endif
    // The callback table identifies its loaded PE image. Resolve that address
    // rather than relying on a loader's filename; exact file hashes and table
    // entries below still qualify the two supported native reel layouts.
    return candidate && loadedImageBase(candidate)==candidate?candidate:0;
}
bool current() { return module && layout && loadedModule(module)==module && epoch==generation(); }
uintptr_t nativeWindow() {
    if(!module || !layout || loadedModule(module)!=module) return 0;
    const auto set=read<unsigned>(module+layout->set,255),count=read<unsigned>(module+layout->count);
    // MsMakeEffId packs the effect serial/type above the low-byte character.
    // The native MsSetReelResult likewise masks the owner with 0xff.
    return set<count && count<=4 && (read<unsigned>(module+layout->actor,255)&0xff)==4?
        module+layout->window+set*0x78:0;
}
uintptr_t window() { return current()?nativeWindow():0; }
bool interactive(uintptr_t win) {
    return win && read<unsigned>(module+layout->sequence)==1 && read<unsigned>(win+0x68)==1;
}
int position(uintptr_t win,unsigned column,unsigned symbol) {
    const auto pattern=read<uintptr_t>(win+column*0x1c);
    const auto count=read<unsigned>(pattern),bytes=read<uintptr_t>(pattern+4);
    if(!pattern || !bytes || count<1 || count>32) return -1;
    for(unsigned i=0;i<count;++i) if(read<uint8_t>(bytes+i,255)==symbol) return static_cast<int>(i);
    return -1;
}
unsigned selectedSymbol(uintptr_t win,unsigned column) {
    const auto pattern=read<uintptr_t>(win+column*0x1c);
    const auto count=read<unsigned>(pattern),bytes=read<uintptr_t>(pattern+4);
    const auto position=read<unsigned>(win+column*0x1c+4)>>16;
    if(!pattern || !bytes || count<1 || count>32 || position>=count) return 255;
    // Native stop snaps down to an integral icon; result/renderer select +1.
    return read<uint8_t>(bytes+(position+1)%count,255);
}
bool attackReels(uintptr_t win) {
    for(unsigned col=0;col<3;++col) {
        const auto pattern=read<uintptr_t>(win+col*0x1c);
        const auto count=read<unsigned>(pattern),bytes=read<uintptr_t>(pattern+4);
        if(!bytes || !count || count>32 || position(win,col,6)<0) return false;
        for(unsigned i=0;i<count;++i) {
            const auto symbol=read<uint8_t>(bytes+i,255);
            if(symbol<4 || symbol>6) return false;
        }
    }
    return true;
}
bool applyMatch(uintptr_t win,unsigned symbol) {
    int positions[3]{};
    for(unsigned col=0;col<3;++col) if((positions[col]=position(win,col,symbol))<0) return false;
    for(unsigned col=0;col<3;++col) {
        const auto reel=win+col*0x1c,pattern=read<uintptr_t>(reel);
        const auto count=read<unsigned>(pattern);
        write<unsigned>(reel+4,static_cast<unsigned>((positions[col]+count-1)%count)<<16);
        write<unsigned>(reel+8,0); write<unsigned>(reel+0x18,0);
    }
    write<unsigned>(win+0x64,3); chosen=true;
    return true;
}
unsigned automaticMatch(uintptr_t win) {
    // Only Attack Reels has an unconditional best match. Element, Status and
    // Aurochs choices pause through openChoice, preserving player selection.
    return attackReels(win)?6:255;
}
TextRef label(TextPlan& plan,unsigned symbol) {
    constexpr unsigned commands[]={0x30f0,0x30f2,0x30f4,0x30f6,0,0x312e,0x312e,0x30f8,0x30fa,0x30fc,0x30fd};
    if(symbol>=11 || !commands[symbol]) return 0;
    auto name=planRecordName(plan,0x39a4c0,commands[symbol]);
    if(symbol==5 || symbol==6) name=planConcat(plan,name,planLiteral(plan,symbol==5?L", 6 hits":L", 12 hits"));
    return name;
}
void releaseMenu(bool resume) {
    if(active.exchange(false)) {
        if(resume && inBattle() && !scene && nativeWindow()==selectedWindow &&
            interactive(selectedWindow) && read<uint16_t>(address(0xf3f73c))==1)
            write<uint16_t>(address(0xf3f73c),savedClock);
        captureSpeech(false); drainController();
    }
    selectedWindow=0; symbolCount=0; row=0;
}
void announce(bool intro=false) {
    if(row>=symbolCount) return;
    TextPlan p;prepareTextPlan(p,WakkaChoiceProducer);p.headline=planConcat(p,planLiteral(p,intro?L"Match reels. ":L""),label(p,symbols[row]));commitTextPlan(p);
}
bool validMenu() {
    return active && current() && inBattle() && !scene && gameFocused() && !utilityConsoleActive() &&
        selectedWindow==window() && interactive(selectedWindow);
}
bool openChoice(uintptr_t win) {
    symbolCount=0;
    for(unsigned symbol=0;symbol<11;++symbol) if(symbol!=4 && position(win,0,symbol)>=0 &&
        position(win,1,symbol)>=0 && position(win,2,symbol)>=0) symbols[symbolCount++]=symbol;
    if(!symbolCount) return false;
    selectedWindow=win; row=0; savedClock=read<uint16_t>(address(0xf3f73c));
    if(!write<uint16_t>(address(0xf3f73c),1)) { selectedWindow=0; symbolCount=0; return false; }
    active=true;
    navAudioCancelOverdriveBeep(); captureSpeech(true); drainController(); announce(true);
    return true;
}
void beep(uintptr_t win) {
    const auto column=read<unsigned>(win+0x64,255),set=read<unsigned>(module+layout->set,255);
    if(column>=3 || read<unsigned>(win+column*0x1c+0x18)!=2) return;
    const auto symbol=selectedSymbol(win,column);
    const auto icon=read<unsigned>(win+column*0x1c+4)>>16;
    if(symbol==255 || (lastIcon==icon && lastColumn==column && lastSet==set)) return;
    lastIcon=icon; lastColumn=column; lastSet=set;
    if(attackReels(win) && symbol!=5 && symbol!=6) { navAudioCancelOverdriveBeep(); return; }
    constexpr float frequencies[]={440,660,880,1100,220,440,880,1320,1540,1760,1980};
    if(symbol<11) navAudioOverdriveBeep(frequencies[symbol],generation());
}
int invoke(uintptr_t fn) {
#ifdef FFX_READER_TEST
    if(fixtureInvoke && fn>=fixtureModule && fn-fixtureModule<0x400000) return fixtureInvoke();
#endif
    return reinterpret_cast<int(__cdecl*)()>(fn)();
}
bool callbackReady(uintptr_t fn) {
#ifdef FFX_READER_TEST
    // Archived PE bytes are mapped non-executable. The fixture substitutes its
    // own native stub; it never executes a game module's code or entry point.
    if(fixtureInvoke && fn>=fixtureModule && fn-fixtureModule<0x400000) return true;
#endif
    return fn && loadedImageBase(fn)!=0;
}
int __cdecl begun(unsigned actor) {
    if(!nativeGameThread()) {
        const auto fn=forwardBegin.load(std::memory_order_acquire);if(!callbackReady(fn)) return 0;
#ifdef FFX_READER_TEST
        if(fixtureInvoke && fn>=fixtureModule && fn-fixtureModule<0x400000) return fixtureInvoke();
#endif
        return reinterpret_cast<int(__cdecl*)(unsigned)>(fn)(actor);
    }
    uintptr_t fn=0;
    {

        if(!module || !layout || loadedModule(module)!=module) return 0;
        releaseMenu(true); epoch=generation(); chosen=dismissed=false;
        lastIcon=lastColumn=lastSet=255; fn=module+layout->begin;
    }
#ifdef FFX_READER_TEST
    if(fixtureInvoke && module==fixtureModule) return fixtureInvoke();
#endif
    return reinterpret_cast<int(__cdecl*)(unsigned)>(fn)(actor);
}
int __cdecl controlled() {
    if(!nativeGameThread()) {
        const auto fn=forwardControl.load(std::memory_order_acquire);return callbackReady(fn)?invoke(fn):0;
    }
    uintptr_t next=0;
    {

        if(!module || !layout || loadedModule(module)!=module || !original) return 0;
        next=original;
        if(nativeGameThread() && (epoch!=generation() || cancelRequested.exchange(false))) resetWakka();
        if(nativeGameThread() && current()) {
            if(active && !validMenu()) releaseMenu(true);
            const auto win=window();
            const bool context=inBattle() && !scene && gameFocused() && !utilityConsoleActive();
            if(active) return 0; // Freeze the native effect sequence while choosing.
            if(context && !modMenuActive() && interactive(win)) {
                if(setting(Setting::OverdriveAssist) && !chosen && !dismissed) {
                    const auto best=automaticMatch(win);
                    if(best!=255) applyMatch(win,best);
                    else if(openChoice(win)) return 0;
                }
                if((!setting(Setting::OverdriveAssist) || dismissed) && setting(Setting::OverdriveCues)) beep(win);
                else navAudioCancelOverdriveBeep();
            }
        }
    }
    // Native callbacks may enter ordinary reader hooks; never hold our lock.
    const auto result=invoke(next);
    {

        if(current()) {
            const auto changed=read<uintptr_t>(table+0x10);
            if(changed!=reinterpret_cast<uintptr_t>(controlled)) {
                // The effect itself replaces its initialization callback with
                // its running callback. Preserve that native transition.
                if(changed==module+layout->control || changed==module+layout->initial) {
                    original=changed;forwardControl.store(changed,std::memory_order_release);
                    write<uintptr_t>(table+0x10,reinterpret_cast<uintptr_t>(controlled));
                }
            }
        }
    }
    return result;
}
void obtained(Event& e) {
    if(!nativeGameThread() || !e.result) return;

    uintptr_t base=0;
    for(const auto& item:layouts) {
        const auto candidate=e.result-item.table;
        if(loadedModule(candidate)==candidate) { base=candidate; break; }
    }
    if(!base) { if(e.rva==wakka_rva::EffectId) recordLog("Wakka module refused: table has no supported exact image base"); return; }
    if(module==base && table==e.result && current() && read<uintptr_t>(table+0x10)==reinterpret_cast<uintptr_t>(controlled)) return;
    if(pendingQualification.serial && pendingQualification.table==e.result && pendingQualification.epoch==generation()) return;
    WakkaIdentity request;request.base=base;request.table=e.result;request.epoch=generation();request.serial=++identitySerial;
#ifdef FFX_READER_TEST
    if(base==fixtureModule && fixturePath) wcsncpy_s(request.testPath.data(),request.testPath.size(),fixturePath,_TRUNCATE);
#endif
    pendingQualification={};
    if(requestWakkaIdentity(request)) pendingQualification=request;
}
void qualified(Event&) {
    if(!nativeGameThread()) return;
    if(pendingQualification.serial && pendingQualification.epoch!=generation()) {
        pendingQualification={};cancelWakkaIdentity();
    }
    WakkaIdentity reply;
    for(unsigned count=0;count<8 && readWakkaIdentity(reply);++count) {
        if(!pendingQualification.serial || reply.serial!=pendingQualification.serial ||
            reply.epoch!=generation() || reply.base!=pendingQualification.base || reply.table!=pendingQualification.table) continue;
        pendingQualification={};
        if(!reply.digest[0] || loadedModule(reply.base)!=reply.base) {
            recordLog("Wakka module refused: unreadable file identity");continue;
        }
        const Layout* match=nullptr;
        for(const auto& item:layouts) if(!std::strcmp(reply.digest.data(),item.digest)) {match=&item;break;}
        if(!match) {recordLog("Wakka module refused: unsupported file identity sha256=%s",reply.digest.data());continue;}
        const auto base=reply.base,tableAddress=reply.table;
        const auto callback=read<uintptr_t>(tableAddress+0x10);
        if(tableAddress!=base+match->table || (callback!=base+match->initial && callback!=base+match->control) ||
            read<uintptr_t>(tableAddress)!=base+match->begin) {
            recordLog("Wakka module refused: native callback entries differ");continue;
        }
        resetWakka();module=base;layout=match;table=tableAddress;original=callback;epoch=generation();
        chosen=dismissed=false;lastIcon=lastColumn=lastSet=255;
        forwardControl.store(original,std::memory_order_release);forwardBegin.store(base+match->begin,std::memory_order_release);
        if(!write<uintptr_t>(table+0x10,reinterpret_cast<uintptr_t>(controlled)) ||
            !write<uintptr_t>(table,reinterpret_cast<uintptr_t>(begun))) {
            resetWakka();recordLog("Wakka module refused: callback table is not writable");continue;
        }
        recordLog("Wakka native reel callback installed table=%p",reinterpret_cast<void*>(table));
    }
}
void effectCreated(Event& e) {
    if(!nativeGameThread() || !inBattle() || (e.result&0xff)!=4) return;
    using Character=uintptr_t(__cdecl*)(unsigned);
    const auto actor=reinterpret_cast<Character>(address(0x394030))(4);
    if(!actor) return;
    // Native MsGetReelKind selects these exact four commands at actor+F5C.
    // Ordinary Wakka attacks also create effect IDs; they are not reel casts.
    const auto command=read<uint16_t>(actor+0xf5c);
    if(command<0x3074 || command>0x3077) return;
    const auto type=(e.result>>8)&0xff;
    Event tableEvent;
    tableEvent.rva=wakka_rva::EffectId;
    if(type==2) {
        tableEvent.result=read<uintptr_t>(actor+0xf8c);
    } else if(type==0x80) tableEvent.result=read<uintptr_t>(address(wakka_rva::GlobalTable));
    else return;
    recordLog("Wakka effect start handle=%x table=%p owner=%lu",static_cast<unsigned>(e.result),reinterpret_cast<void*>(tableEvent.result),GetCurrentThreadId());
    // Both native start sites create the packed ID immediately before calling
    // table[0]. Discover the consumed table on the owner thread, including an
    // asynchronously loaded overlay whose getter ran on the loader thread.
    obtained(tableEvent);
}
}
bool wakkaMenuActive() { return active; }
void wakkaMenuKey(unsigned vk) {

    if(!nativeGameThread()) return;
    if(!validMenu()) { releaseMenu(true); return; }
    if(vk==VK_ESCAPE || vk==VK_BACK || vk==VK_F8) {
        dismissed=true; releaseMenu(true); output(L"Manual reels",true,WakkaChoiceProducer); return;
    }
    if(vk==VK_UP) row=(row+symbolCount-1)%symbolCount;
    else if(vk==VK_DOWN) row=(row+1)%symbolCount;
    else if(vk==VK_HOME || vk==VK_LEFT) row=0;
    else if(vk==VK_END || vk==VK_RIGHT) row=static_cast<unsigned>(symbolCount-1);
    else if(vk==VK_RETURN || vk==VK_SPACE) {
        TextPlan p;prepareTextPlan(p,WakkaChoiceProducer);p.headline=label(p,symbols[row]);
        if(!applyMatch(selectedWindow,symbols[row])) { dismissed=true; releaseMenu(true); return; }
        releaseMenu(true);commitTextPlan(p);return;
    } else if(vk=='I' || vk=='N' || vk=='O') {
        output(L"Up and Down choose a match. Enter or Space applies it. On a controller, use A. Escape or B returns to manual reels.",true,WakkaChoiceProducer); return;
    } else return;
    announce();
}
void resetWakka() {
    if(!nativeGameThread()) { cancelRequested=true; return; }

    pendingQualification={};cancelWakkaIdentity();
    releaseMenu(true);
    if(module && loadedModule(module)==module && table && read<uintptr_t>(table+0x10)==reinterpret_cast<uintptr_t>(controlled)) write<uintptr_t>(table+0x10,original);
    if(module && layout && loadedModule(module)==module && table && read<uintptr_t>(table)==reinterpret_cast<uintptr_t>(begun)) write<uintptr_t>(table,module+layout->begin);
    forwardControl.store(0,std::memory_order_release);forwardBegin.store(0,std::memory_order_release);
    module=table=original=0; layout=nullptr; epoch=0; chosen=dismissed=false;
    lastIcon=lastColumn=lastSet=255; navAudioCancelOverdriveBeep();
    cancelRequested=false;
}
void installWakka() {
    if(!startWakkaIdentity()) throw std::runtime_error("Wakka identity worker unavailable");
    hook(0x225930,nullptr,qualified);
    hook(0x5da370,nullptr,obtained);hook(wakka_rva::EffectId,nullptr,effectCreated);
}
#ifdef FFX_READER_TEST
void testWakkaModule(uintptr_t base,const wchar_t* path,int(*fn)()) { resetWakka(); fixtureModule=base; fixturePath=path; fixtureInvoke=fn; }
bool testWakkaIdentityPending() {return pendingQualification.serial!=0;}
void testWakkaPollIdentity() {Event e;qualified(e);}
int testWakkaControl() { return controlled(); }
int testWakkaBegin(unsigned actor) { return begun(actor); }
uintptr_t testWakkaLoadedModule(uintptr_t candidate) { return loadedModule(candidate); }
#endif
}
