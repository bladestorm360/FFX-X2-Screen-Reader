#include "input_device.hpp"
#include "events.hpp"
#include "controller.hpp"
#include "direction_glyph.hpp"
namespace ffx {
std::wstring nativeBindingName(unsigned action,bool controller) {
    if(action>80) return {};
    const auto config=read<uintptr_t>(address(0x1efb504));
    if(!config) return {};
    const auto value=read<unsigned>(config+0x60+action*4,128);
    if(value>=128) return {};
    using Name=uintptr_t(__cdecl*)(unsigned);
    return clean(ascii(reinterpret_cast<Name>(address(controller?0x5b10:0x5af0))(value),64));
}
std::wstring nativeButtonMaskName(uint16_t mask) {
    // Native 631770 registers config rows in this order. These are logical
    // input masks, not font tokens: 10 is Square, 80 is Triangle, 20 Confirm.
    constexpr uint16_t masks[]={0x1000,0x4000,0x8000,0x2000,0x20,0x40,0x10,0x80,
        4,1,8,2,0x800,0x100};
    const bool pad=nativeInputUsesController();
    for(unsigned i=0;i<_countof(masks);++i) if(mask==masks[i]) {
        if(pad) return nativeBindingName(i+14,true);
        const auto config=read<uintptr_t>(address(0x1efb504));
        if(!config) return {};
        for(unsigned action:{i,i+49}) {
            const auto key=read<unsigned>(config+0x60+action*4,128);
            if(key<28 || key>=128) continue;
            const auto name=nativeBindingName(action,false);
            if(!name.empty()) return name;
        }
        return {};
    }
    return {};
}
namespace {
std::wstring nativeGlyph(uint8_t glyph) {
    // 8F8EE0 selects the atlas token; 6E6860 maps that token to an action.
    // 631770 supplies that action's logical mask. Tokens are not row indices.
    using Swap=int(__cdecl*)();
    if((glyph==0x31 || glyph==0x32) && reinterpret_cast<Swap>(address(0x4acee0))()==1) glyph=glyph==0x31?0x32:0x31;
    if(glyph>=0x20 && glyph<=0x22 && !nativeInputUsesController()) {
        constexpr unsigned actions[]={38,36,37};
        const auto name=nativeBindingName(actions[glyph-0x20],false);
        return name.empty()?L"unassigned control":name;
    }
    if(glyph>=0x30 && glyph<0x3a) {
        constexpr uint16_t masks[]={0x10,0x20,0x40,0x80,4,8,1,2,0x800,0x100};
        const auto name=nativeButtonMaskName(masks[glyph-0x30]);
        return name.empty()?L"unassigned control":name;
    }
    if(glyph==0x40) return L"directional pad";
    if(glyph>0x40 && glyph<0x50) {
        std::vector<std::wstring> directions;
        constexpr uint16_t masks[]={0x1000,0x2000,0x4000,0x8000};
        for(unsigned i=0;i<4;++i) if(glyph&(1u<<i)) {
            const auto name=nativeButtonMaskName(masks[i]);
            directions.push_back(name.empty()?L"unassigned control":name);
        }
        return join(directions,directionGlyphSeparator(glyph));
    }
    return {};
}
}
void installButtonGlyphs() { setButtonGlyphReader(nativeGlyph); }
}
