#include "movement_keys.hpp"
#include "events.hpp"
#include "game_bindings.hpp"
#include <cwctype>
namespace ffx {
std::array<unsigned,4> movementKeys() {
    // Documented default FFX movement is the arrow keys, unlike FFXII WASD.
    std::array<unsigned,4> result{0xc8,0xcb,0xd0,0xcd};
    const auto config=read<uintptr_t>(address(0x1efb504));
    if(!config) return result;
    using Resolve=void(__cdecl*)(unsigned,char*,unsigned);
    for(const auto& binding:keyboard) {
        char caption[256]{};
        reinterpret_cast<Resolve>(address(0x2414a0))(binding.label,caption,sizeof(caption));
        auto label=ascii(reinterpret_cast<uintptr_t>(caption),sizeof(caption));
        std::wstring name; for(auto c:label) if(iswalpha(c)) name+=static_cast<wchar_t>(towlower(c));
        int direction=-1;
        if(name==L"up" || name==L"moveup" || name==L"forward" || name==L"moveforward") direction=0;
        if(name==L"left" || name==L"moveleft") direction=1;
        if(name==L"down" || name==L"movedown" || name==L"backward" || name==L"movebackward") direction=2;
        if(name==L"right" || name==L"moveright") direction=3;
        if(direction<0) continue;
        const auto primary=configuredKeyboardScan(read<unsigned>(config+0x60+binding.first*4,128));
        const auto alternate=configuredKeyboardScan(read<unsigned>(config+0x60+binding.second*4,128));
        if(primary || alternate) result[direction]=primary?primary:alternate;
    }
    return result;
}
}
