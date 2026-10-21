#include "game_bindings.hpp"
#include "runtime.hpp"
#include "controller_model.hpp"
namespace ffx {
unsigned configuredKeyboardScan(unsigned id) {
    // Config IDs name both device classes. Native keyboard629F00 subtracts
    // 28; its device Update then converts that local ID to a DI scan code.
    return id>=28 && id<128?read<uint8_t>(address(0x830e90)+id-28):0;
}
int configuredControllerButton(unsigned id) {
    // Native controller629E50 subtracts11, not keyboard's28.
    constexpr int controls[]={pad::West,pad::South,pad::East,pad::North,pad::LB,pad::RB,
        pad::LT,pad::RT,pad::View,pad::Menu,pad::L3,pad::R3,pad::Up,pad::Right,pad::Down,pad::Left};
    return id>=11 && id<=26?controls[id-11]:-1;
}
unsigned configuredActionKeyboardScan(unsigned action) {
    const auto scans=configuredActionKeyboardScans(action);
    return scans[0]?scans[0]:scans[1];
}
std::array<unsigned,2> configuredActionKeyboardScans(unsigned action) {
    if(action>=14) return {};
    const auto config=read<uintptr_t>(address(0x1efb504)); if(!config) return {};
    return {configuredKeyboardScan(read<unsigned>(config+0x60+action*4,128)),
        configuredKeyboardScan(read<unsigned>(config+0x60+(action+49)*4,128))};
}
int configuredActionControllerButton(unsigned action) {
    if(action>=14) return -1;
    const auto config=read<uintptr_t>(address(0x1efb504));
    return config?configuredControllerButton(read<unsigned>(config+0x60+(action+14)*4,128)):-1;
}
}
