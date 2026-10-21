#include "copied_controls.hpp"
#include "input_device.hpp"
#include "events.hpp"
#include "process_storage.hpp"
#include <atomic>
namespace ffx {
namespace {
using NameTables=std::array<std::array<std::wstring,128>,2>;
ProcessStorage<NameTables> nameStorage;
NameTables& names() noexcept {return nameStorage.get();}
std::atomic<bool> namesReady{false};
}
void prepareCopiedControlNames() {
#ifndef FFX_READER_TEST
    if(namesReady.load(std::memory_order_acquire)) return;
#endif
    nameStorage.prepare();
    // Isolated reader scenarios replace synthetic name tables between cases;
    // their explicit drain reinitializes this fixture cache. Native module
    // tables are immutable and initialized once before hook publication.
    // Verified 405AF0/405B10 are pure immutable module pointer-table getters.
    // They have no engine objects, allocation, state mutation or thread owner.
    using Name=uintptr_t(__cdecl*)(unsigned);
    for(unsigned pad=0;pad<2;++pad) for(unsigned id=0;id<128;++id)
        names()[pad][id]=clean(ascii(reinterpret_cast<Name>(address(pad?0x5b10:0x5af0))(id),64));
    namesReady.store(true,std::memory_order_release);
}
OwnedTextContext copiedControlContext(const CopiedControls& p) {
    OwnedTextContext c; c.polish=p.polish;
    if(!namesReady.load(std::memory_order_acquire)) return c;
    const unsigned dodge=p.bindings[p.controller?18:4];
    if(dodge<128) c.primaryDodge=names()[p.controller?1:0][dodge];
    constexpr uint16_t masks[]={0x1000,0x4000,0x8000,0x2000,0x20,0x40,0x10,0x80,4,1,8,2,0x800,0x100};
    auto name=[&](uint16_t mask) -> std::wstring {
        for(unsigned i=0;i<14;++i) if(mask==masks[i]) {
            if(p.controller) { const auto id=p.bindings[i+14]; return id<128?names()[1][id]:std::wstring{}; }
            for(unsigned row:{i,i+49}) {
                const auto id=p.bindings[row];
                if(id>=28 && id<128 && !names()[0][id].empty()) return names()[0][id];
            }
        }
        return {};
    };
    constexpr uint16_t face[]={0x10,0x20,0x40,0x80,4,8,1,2,0x800,0x100};
    for(unsigned i=0;i<10;++i) { auto value=name(face[i]); c.glyphs[0x30+i]=value.empty()?L"unassigned control":value; }
    if(p.swap) std::swap(c.glyphs[0x31],c.glyphs[0x32]);
    // Native6E6860 maps the extra keyboard tokens20/21/22 to actions38/36/37
    // (including the CTB scroll keys). Controller art uses its stick symbols.
    if(!p.controller) for(unsigned token=0;token<3;++token) {
        constexpr unsigned actions[]={38,36,37};const auto id=p.bindings[actions[token]];
        c.glyphs[0x20+token]=id>=28 && id<128?names()[0][id]:L"unassigned control";
    }
    c.glyphs[0x40]=L"directional pad";
    constexpr uint16_t direction[]={0x1000,0x2000,0x4000,0x8000};
    for(unsigned glyph=0x41;glyph<0x50;++glyph) for(unsigned bit=0;bit<4;++bit) if(glyph&(1u<<bit)) {
        if(!c.glyphs[glyph].empty()) c.glyphs[glyph]+=directionGlyphSeparator(static_cast<uint8_t>(glyph));
        auto value=name(direction[bit]); c.glyphs[glyph]+=value.empty()?L"unassigned control":value;
    }
    return c;
}
void prepareCopiedControlSnapshot(CopiedControls& p) {
    p.controller=nativeInputUsesController();p.swap=read<unsigned>(address(0x859554))==1;
    p.polish=textEncodingSnapshot()==1;p.bindings.fill(128);
    const auto config=read<uintptr_t>(address(0x1efb504));
    if(config) copy(config+0x60,p.bindings.data(),sizeof(p.bindings));
}
}
