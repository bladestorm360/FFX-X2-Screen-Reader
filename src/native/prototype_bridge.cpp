#include "profile.hpp"
#include "mod_menu.hpp"
// Read-only bridges for user-run, offline-authored Frida prototypes. Function
// resolution stays in the verified native profile; no Frida producer ships.
extern "C" uintptr_t __cdecl FfxNative_ProfileAddress(unsigned rva) {
    if(!FfxNative_Features() || ffx::nativeProfile==ffx::NativeProfile::MovieCaptions) return 0;
    switch(rva) {
    case 0x425010: case 0x426770: case 0x42b500: case 0x43d3b0:
    case 0x4713a0: case 0x46c500: case 0x46c240: case 0x46a7c0: case 0x46fed0:
    case 0x5093b0: case 0x4aa0b0: case 0xf26ae8: case 0x1fc44e4:
    case 0x1fc44e0: case 0xf00740: return ffx::address(rva);
    default: return 0;
    }
}
extern "C" int __cdecl FfxNative_AccessibilityMenuActive() { return ffx::modMenuActive(); }
