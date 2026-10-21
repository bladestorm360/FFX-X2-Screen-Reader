#include "input_device.hpp"
// Consent is translated through the real native configuration and field remap.
// Device callbacks consume only this published snapshot, never script state.
#include "pedestal.hpp"
#include "pedestal_rvas.hpp"
#include "controller.hpp"
#include "game_bindings.hpp"
#include <atomic>
#include <array>
namespace ffx {
namespace {
std::array<std::atomic<uint64_t>,4> keys{};
std::array<std::atomic<uint64_t>,4> movementKeys{};
std::atomic<uint64_t> buttons{0};
std::atomic<unsigned> epoch{0};
constexpr unsigned nativeMasks[]={0x1000,0x4000,0x8000,0x2000,0x20,0x40,0x10,0x80,
    0x08000004,0x10000001,0x02000008,0x04000002,0x800,0x100};
unsigned scanFor(uintptr_t config,unsigned action) {
    const unsigned id=read<unsigned>(config+0x60+action*4,128);
    return configuredKeyboardScan(id);
}
}
unsigned pedestalLogicalInput(unsigned raw) {
    using Remap=uint16_t(__cdecl*)(unsigned,unsigned,uint16_t);
    return reinterpret_cast<Remap>(address(pedestal_rva::Remap))(0,0,static_cast<uint16_t>(raw));
}
unsigned pedestalInputMask(unsigned logical) {
    unsigned result=0;
    for(unsigned i=0;i<16;++i) if(pedestalLogicalInput(1u<<i)&logical) result|=1u<<i;
    return result;
}
unsigned pedestalConfirmMask() { return pedestalInputMask(0x20); }
std::wstring pedestalConsentName(const PedestalPlan& plan) {
    const bool single=!plan.moves.empty() && plan.moves.front().mode==PushMode::Single;
    const auto mask=single?pedestalConfirmMask():0x40;
    const bool pad=nativeInputUsesController();
    std::vector<std::wstring> names;
    for(unsigned i=0;i<_countof(nativeMasks);++i) if(nativeMasks[i]&mask) {
        for(unsigned action:{pad?i+14:i,pad?i+14:i+49}) {
            const auto name=nativeBindingName(action,pad);
            if(!name.empty()) names.push_back(name);
            if(pad) break;
        }
    }
    return names.empty()?single?L"Confirm":L"the configured push control":join(names,L" or ");
}
bool publishPedestalConsent(unsigned mask) {
    epoch=0; buttons=0; for(auto& key:keys) key=0; for(auto& key:movementKeys) key=0;
    if(!mask) return false;
    const auto config=read<uintptr_t>(address(0x1efb504)); if(!config) return false;
    std::array<uint64_t,4> scans{};
    uint64_t allowed=0,movement=0;
    std::array<bool,256> movementScans{};
    for(unsigned i=0;i<4;++i) {
        for(unsigned action:{i,i+49}) movementScans[scanFor(config,action)]=true;
        const auto button=configuredControllerButton(read<unsigned>(config+0x60+(i+14)*4,128));
        if(button>=0) movement|=pad::bit(button);
    }
    for(unsigned i=0;i<_countof(nativeMasks);++i) {
        if((nativeMasks[i]&0xffff)&mask) for(unsigned action:{i,i+49}) {
            const auto scan=scanFor(config,action);
            if(scan && !movementScans[scan]) scans[scan/64]|=uint64_t{1}<<(scan%64);
        }
        if(nativeMasks[i]&mask) {
            const auto button=configuredControllerButton(read<unsigned>(config+0x60+(i+14)*4,128));
            if(button>=0) allowed|=pad::bit(button);
        }
    }
    // Directional/command-mode buttons never become consent for walking.
    allowed&=~(movement|pad::bit(pad::Up)|pad::bit(pad::Down)|pad::bit(pad::Left)|pad::bit(pad::Right)|
        pad::bit(pad::View)|pad::bit(pad::Menu)|pad::bit(pad::Guide));
    for(unsigned i=0;i<keys.size();++i) keys[i]=scans[i];
    for(unsigned scan=1;scan<256;++scan) if(movementScans[scan]) movementKeys[scan/64]|=uint64_t{1}<<(scan%64);
    bool configured=allowed!=0;
    for(const auto scansWord:scans) configured=configured || scansWord!=0;
    if(!configured) return false;
    buttons=allowed; epoch=generation(); return true;
}
bool pedestalConsentKey(unsigned scan) {
    return scan<256 && epoch==generation() && (keys[scan/64]&(uint64_t{1}<<(scan%64)))!=0;
}
bool pedestalConsentVk(unsigned vk) {
    const auto scan=MapVirtualKeyW(vk,MAPVK_VK_TO_VSC_EX);
    return pedestalConsentKey((scan&0xff)|((scan&0xff00)?0x80:0));
}
uint64_t pedestalConsentButtons() { return epoch==generation()?buttons.load():0; }
bool pedestalMovementKey(unsigned scan) {
    return scan<256 && epoch==generation() && (movementKeys[scan/64]&(uint64_t{1}<<(scan%64)))!=0;
}
}
