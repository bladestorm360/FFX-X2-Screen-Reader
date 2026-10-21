// Handles displayed free-search coordinate snapshots and default-Off six-region movement/Confirm assist, not the missing image destination list. See Docs/features/index.md and Docs/dylan_reports_20261018.md.
#include "airship_search.hpp"
#include "airship_destinations.hpp"
#include "airship_map.hpp"
#include "events.hpp"
#include "nonblocking_snapshots.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "auto_walk.hpp"
#include "input_device.hpp"
#include "game_bindings.hpp"
#include <atomic>
#include <array>
#include <cmath>
#include <cwchar>
#include <algorithm>
namespace ffx {
namespace {
constexpr unsigned CodeLength=24785,CodeHash=0xde956a03;
struct Identity { unsigned epoch=0;uintptr_t token=0;std::array<uint8_t,CodeLength> code{}; };
struct Frame {
    unsigned epoch=0;uintptr_t token=0;uint64_t stamp=0;
    float x=0,y=0,z=0,forwardX=0,forwardZ=0;
    bool search=false,controlled=false,focused=false;
    std::array<unsigned,4> movement{};unsigned confirmScan=0;int confirmButton=-1;
};
struct Verified { unsigned epoch=0;uintptr_t token=0; };
struct Input { unsigned epoch=0,request=0,mask=0;uintptr_t token=0;uint64_t stamp=0;bool confirm=false; };
async::TryCaptureQueue<Identity,4> identities;
async::AtomicSnapshot<Frame> frames;
async::AtomicSnapshot<Verified> verified;
async::AtomicSnapshot<Input> input;
std::atomic<unsigned> request{1};
async::TryCaptureQueue<unsigned,32> readRequests;
std::atomic<int> destination{-1};
std::atomic<bool> startRequested{false};
HANDLE wake=nullptr;
unsigned sentEpoch=0;uintptr_t sentToken=0; // native owner only
unsigned consumedConfirm=0; // native input owner only
constexpr float targets[6][2]={{14.3147459f,-59.843399f},{72.0362701f,-36.3792343f},
    {30.5800018f,-74.6152496f},{35.0868149f,-57.518692f},{41.4966354f,-58.689209f},{14.2042418f,-42.8997765f}};
bool current(Frame& frame,bool allowMenu=true) {
    Verified id;
    if(!frames.tryRead(frame) || !verified.tryRead(id) || id.epoch!=generation() || id.epoch!=frame.epoch ||
        !id.token || id.token!=frame.token || !frame.search || !frame.focused || !frame.controlled ||
        GetTickCount64()-frame.stamp>250 || (!allowMenu && modMenuActive())) return false;
    const auto fieldController=read<uintptr_t>(address(0xf26ae8));
    if(!fieldController || read<uintptr_t>(fieldController+0x48)!=frame.token) return false;
    const auto global=read<unsigned>(frame.token+0x20);
    return !scene && !inBattle() && global==0x150f8 && airshipFreeSearchActive();
}
void cancel() { destination=-1;request.fetch_add(1); }
void captured(Event& e) {
    // Common00A3 already produced these values on the native script owner.
    if(e.args[0]!=2 || e.parentRva!=0x456cc0 || !nativeGameThread()) return;
    const auto fieldController=read<uintptr_t>(address(0xf26ae8));
    const auto chunk=read<uintptr_t>(fieldController+0x48);
    if(!chunk || read<unsigned>(chunk)!=CodeLength || read<uint16_t>(chunk+0x34)!=41 ||
        read<unsigned>(chunk+0x20)!=0x150f8 || read<unsigned>(chunk+0x30)!=0xe6c) return;
    const unsigned nameOffset=read<unsigned>(chunk+0xc);
    char name[8]{};
    if(nameOffset>0x100000-8 || !copy(chunk+nameOffset,name,8) || std::memcmp(name,"hiku2100",8)) return;
    const auto global=chunk+0x150f8;
    if(e.args[1]!=global+0x180 || e.args[2]!=global+0x184 || e.args[3]!=global+0x188) return;
    if(sentEpoch!=e.epoch || sentToken!=chunk) {
        Identity id;id.epoch=e.epoch;id.token=chunk;
        if(copy(chunk+0xe6c,id.code.data(),id.code.size()) && identities.tryPush(id)) {sentEpoch=e.epoch;sentToken=chunk;}
    }
    Frame frame;frame.epoch=e.epoch;frame.token=chunk;frame.stamp=GetTickCount64();
    frame.x=read<float>(e.args[1]);frame.y=read<float>(e.args[2]);frame.z=read<float>(e.args[3]);
    frame.search=read<int>(global+0x150)==1;
    frame.controlled=(read<uint8_t>(fieldController)&2)!=0 && read<uintptr_t>(address(0xf00740))!=0;
    frame.focused=gameFocused();
    const auto camera=read<uintptr_t>(address(0x1f11430));
    if(camera) {frame.forwardX=read<float>(camera+0x40)-read<float>(camera+0x65c);frame.forwardZ=read<float>(camera+0x48)-read<float>(camera+0x664);}
    constexpr unsigned actions[]={0,2,1,3}; // native masks Up, Left, Down, Right
    for(unsigned i=0;i<4;++i) frame.movement[i]=configuredActionKeyboardScan(actions[i]);
    const unsigned confirmAction=read<uint8_t>(address(0x859554))?5:4;
    frame.confirmScan=configuredActionKeyboardScan(confirmAction);frame.confirmButton=configuredActionControllerButton(confirmAction);
    frames.publish(frame);if(wake) SetEvent(wake);
}
void process() {
    Identity id;
    while(identities.tryPop(id)) {
        unsigned hash=2166136261u;for(auto byte:id.code) hash=(hash^byte)*16777619u;
        if(id.epoch==generation()) verified.publish(hash==CodeHash?Verified{id.epoch,id.token}:Verified{});
    }
    Frame f;Verified v;
    const bool have=frames.tryRead(f) && verified.tryRead(v) && f.epoch==generation() && v.epoch==f.epoch && v.token==f.token &&
        f.search && f.controlled && f.focused && GetTickCount64()-f.stamp<=250 && std::isfinite(f.x) && std::isfinite(f.y) && std::isfinite(f.z);
    unsigned readEpoch=0;
    while(readRequests.tryPop(readEpoch)) if(have && readEpoch==f.epoch) {
        // The script renders absolute X/Z, absolute Y plus minus only for Y>0,
        // and truncates four fraction digits. Match those displayed digits.
        auto shown=[](float number) { return std::floor(std::abs(double(number))*10000)/10000; };
        wchar_t text[160]{};swprintf_s(text,L"X %.4f, Y %ls%.4f, Z %.4f",shown(f.x),f.y>0?L"minus ":L"",shown(f.y),shown(f.z));
        outputAt(text,true,"mod-airship-coordinates",f.epoch);
    }
    static unsigned previousRequest=0;static double best=0;static uint64_t improved=0;
    const auto serial=request.load();const int target=destination.load();
    Input command;command.request=serial;command.epoch=f.epoch;command.token=f.token;command.stamp=f.stamp;
    if(target<0 || target>=6 || !have || !setting(Setting::HiddenAirshipDestinations)) {if(target>=0) cancel();input.publish(command);return;}
    if(serial!=previousRequest) {
        previousRequest=serial;best=1e9;improved=GetTickCount64();
        outputAt(L"Moving search cursor to "+std::wstring(hiddenAirshipNames[target])+L". Movement or Cancel stops the search.",true,"mod-airship-search",f.epoch);
    }
    const double dx=targets[target][0]-f.x,dz=targets[target][1]-f.z,distance=std::hypot(dx,dz);
    if(distance<best-.05) {best=distance;improved=GetTickCount64();}
    if(GetTickCount64()-improved>15000) {cancel();outputAt(L"Airship search stopped: no cursor progress",true,"mod-airship-search",f.epoch);input.publish(command);return;}
    if(distance<.45) command.confirm=true;
    else {
        const double length=std::hypot(f.forwardX,f.forwardZ);
        if(!std::isfinite(length) || length<.001) {cancel();input.publish(command);return;}
        const double front=(dx*f.forwardX+dz*f.forwardZ)/length,side=(dx*f.forwardZ-dz*f.forwardX)/length;
        const int octant=(static_cast<int>(std::round(std::atan2(side,front)*4/3.141592653589793))+8)&7;
        constexpr unsigned octants[]={1,9,8,12,4,6,2,3};command.mask=octants[octant];
    }
    if(serial==request.load()) input.publish(command);
}
DWORD WINAPI worker(void*) {for(;;) {WaitForSingleObject(wake,20);process();}}
void airshipKeyboard(uint8_t* keys) {
    if(destination.load()<0) return;
    Frame frame;Input command;
    if(!setting(Setting::HiddenAirshipDestinations) || nativeInputUsesController() || !current(frame,false) ||
        !input.tryRead(command) || command.request!=request.load() || command.epoch!=generation() || command.token!=frame.token || GetTickCount64()-command.stamp>250) return;
    for(auto scan:frame.movement) if(scan && scan<256 && (keys[scan]&0x80)) {cancel();return;}
    if((keys[0x01]|keys[0x1c])&0x80 || (frame.confirmScan<256 && (keys[frame.confirmScan]&0x80))) {cancel();return;}
    if(command.confirm && consumedConfirm!=command.request) {
        if(!frame.confirmScan || frame.confirmScan>=256) {cancel();return;}
        keys[frame.confirmScan]|=0x80;consumedConfirm=command.request;destination=-1;
    } else for(unsigned i=0;i<4;++i) if(command.mask&(1u<<i)) {if(frame.movement[i]>0 && frame.movement[i]<256) keys[frame.movement[i]]|=0x80;else cancel();}
}
void airshipController(pad::State& state) {
    if(destination.load()<0) return;
    Frame frame;Input command;
    if(!setting(Setting::HiddenAirshipDestinations) || !nativeInputUsesController() || !current(frame,false) ||
        !input.tryRead(command) || command.request!=request.load() || command.epoch!=generation() || command.token!=frame.token || GetTickCount64()-command.stamp>250) return;
    if(state.held || std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 || std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 || state.lt>=30 || state.rt>=30) {cancel();return;}
    if(command.confirm && consumedConfirm!=command.request) {
        if(frame.confirmButton<0) {cancel();return;}
        if(frame.confirmButton==pad::LT) state.lt=255;else if(frame.confirmButton==pad::RT) state.rt=255;else state.held|=pad::bit(frame.confirmButton);
        consumedConfirm=command.request;destination=-1;
    } else {state.lx=static_cast<int16_t>((command.mask&8?18000:0)-(command.mask&2?18000:0));state.ly=static_cast<int16_t>((command.mask&1?18000:0)-(command.mask&4?18000:0));}
}
}
bool hiddenAirshipMenuAvailable() {Frame f;return setting(Setting::HiddenAirshipDestinations) && current(f);}
bool beginHiddenAirshipSearch(unsigned target) {
    Frame f;if(target>=6 || !hiddenAirshipMenuAvailable() || !current(f,false)) return false;
    stopAutoWalk();request.fetch_add(1);destination=static_cast<int>(target);if(wake) SetEvent(wake);return true;
}
bool readAirshipCoordinates() {Frame f;if(!current(f)) return false;readRequests.tryPush(f.epoch);if(wake) SetEvent(wake);return true;}
void resetAirshipSearch() {resetAirshipDestinations();cancel();sentEpoch=0;sentToken=0;}
void installAirshipSearch() {
    installAirshipDestinations();
#ifndef FFX_READER_TEST
    if(!startRequested.exchange(true)) {
        HMODULE pinned=nullptr;
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&installAirshipSearch),&pinned)) {
            wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
            if(wake) {const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);if(thread) CloseHandle(thread);else {CloseHandle(wake);wake=nullptr;}}
        }
    }
#endif
    hook(0x456cc0,nullptr,nullptr); // verified Common00A3 parent boundary
    hook(0x46a280,nullptr,captured);
    setNavigationInputAssist(airshipKeyboard,airshipController);
    hotkey('M',[](bool) {if(!readAirshipCoordinates()) announceCurrentMap();});
    hotkeyChordWhen('M',true,[](bool) {openHiddenAirshipMenu();},hiddenAirshipMenuAvailable);
}
#ifdef FFX_READER_TEST
void drainAirshipSearchForTest() {process();}
#endif
}
