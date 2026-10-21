#include "runtime.hpp"
#include "profile.hpp"
#include "settings.hpp"
#include "text_encoding.hpp"
#include "direction_glyph.hpp"
#include <atomic>
#include <algorithm>

namespace ffx {
#ifdef FFX_READER_TEST
extern uintptr_t testBase;
// The upper fixture range contains synthetic windows/text, not game RVAs.
uintptr_t address(unsigned rva) { return testBase + (rva >= 0x2100000 ? rva : profileRva(rva)); }
#else
uintptr_t address(unsigned rva) {
    // First resolved by bootstrap/profile validation before native hooks run.
    static const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    return base + profileRva(rva);
}
#endif

bool copy(uintptr_t source, void* destination, size_t length) {
    if (source < 0x10000 || length > 0x10000) return false;
    __try { memcpy(destination, reinterpret_cast<void*>(source), length); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

namespace {
ButtonGlyphReader glyphReader=nullptr;
TextEncodingDetector encodingDetector=nullptr;
std::atomic<int> detectedEncoding{-1}; // -1 unknown, 0 stock, 1 Polish
int encodingForString() {
    const int preference=setting(Setting::PolishText); // one snapshot for this decode call
    if(preference==1) return 1;
    int cached=detectedEncoding.load();
    if(cached>=0) return cached;
    const auto detector=encodingDetector;
    if(!detector) return 0;
    bool known=false;
    const bool polish=detector(known);
    if(known) {
        const int result=polish?1:0;
        detectedEncoding.store(result);
        return result;
    }
    return 0;
}
}
int textEncodingSnapshot() { return encodingForString(); }
void setTextEncodingDetector(TextEncodingDetector detector) {
    encodingDetector=detector;
    detectedEncoding=-1;
}
void setButtonGlyphReader(ButtonGlyphReader reader) { glyphReader=reader; }
std::wstring buttonGlyph(uint8_t glyph) {
    if(glyphReader) { const auto text=glyphReader(glyph); if(!text.empty()) return text; }
    static const wchar_t* names[]={L"Triangle",L"Circle",L"Cross",L"Square",L"L1",L"L2",L"R1",L"R2",L"Start",L"Select"};
    if(glyph>=0x30 && glyph<0x3a) return names[glyph-0x30];
    if(glyph>=0x40 && glyph<0x50) {
        std::wstring text;
        const wchar_t* directions[]={L"Up",L"Right",L"Down",L"Left"};
        for(unsigned i=0;i<4;++i) if(glyph&(1u<<i)) { if(!text.empty()) text+=directionGlyphSeparator(glyph); text+=directions[i]; }
        return text.empty()?L"directional pad":text;
    }
    if(glyph>=0x20 && glyph<=0x22) return L"left stick";
    return L"controller button";
}
std::vector<std::wstring> decode(uintptr_t source, size_t length, TextMode mode) {
    static constexpr wchar_t charset[] = L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~\u00b7\u3010\u3011\u266a\u2665 \u201c\u201d\u2014 \u00a1\u2191\u2193\u2190\u2192\u00a8\u00ab\u00ba \u00bb\u00bf\u00c0\u00c1\u00c2\u00c4\u00c7\u00c8\u00c9\u00ca\u00cb\u00cc\u00cd\u00ce\u00cf\u00d1\u00d2\u00d3\u00d4\u00d6\u00d9\u00da\u00db\u00dc\u00df\u00e0\u00e1\u00e2\u00e4\u00e7\u00e8\u00e9\u00ea\u00eb\u00ec\u00ed\u00ee\u00ef\u00f1\u00f2\u00f3\u00f4\u00f6\u00f9\u00fa\u00fb\u00fc,\u0192\u201e\u2026'\u2019\u2022-~\u2122 \u203a\u00a7\u00a9\u00aa\u00ae\u00b1\u00b2\u00b3\u00bc\u00bd\u00be\u00d7\u00f7\u2039\u2026 \u01ce\u2605\u2606\u25a0\u221e";
    static constexpr struct { int index; wchar_t value; } polishGlyphs[] = {
        {115,L'\u0104'},{116,L'\u015A'},{119,L'\u0106'},{121,L'\u0118'},
        {123,L'\u0141'},{128,L'\u0143'},{133,L'\u017B'},{134,L'\u0179'},
        {138,L'\u0105'},{142,L'\u0107'},{144,L'\u0119'},{150,L'\u0142'},
        {151,L'\u0144'},{154,L'\u015B'},{157,L'\u017A'},{159,L'\u017C'}
    };
    if (!length || length > 0x10000) return {};
    const int textEncoding=encodingForString();
    // Read bytes individually so a terminator next to an unmapped page is safe.
    std::vector<std::wstring> parts;
    std::wstring current;
    auto finish = [&] {
        if(mode==TextMode::Literal) { if(!current.empty()) parts.push_back(current); current.clear(); return; }
        const auto begin = current.find_first_not_of(L" \r\n\t");
        if (begin != std::wstring::npos)
            parts.push_back(current.substr(begin, current.find_last_not_of(L" \r\n\t") - begin + 1));
        current.clear();
    };
    for (size_t i = 0; i < length;) {
        uint8_t b;
        if (!copy(source + i, &b, 1)) break;
        if (!b) { finish(); if (mode == TextMode::Terminated || mode==TextMode::Literal) break; ++i; continue; }
        if (b == 3 || b == 10) { finish(); i += b == 10 && mode != TextMode::LegacyRecords ? 2 : 1; continue; }
        if (mode == TextMode::LegacyRecords) {
            if (b == 9 || b == 16) { i += 2; continue; }
            if (b < 0x2b) { ++i; continue; }
        }
        if(b==11) {
            uint8_t glyph=0; if(i+1>=length || !copy(source+i+1,&glyph,1)) break;
            current+=buttonGlyph(glyph); i+=2; continue;
        }
        if (b == 4) {
            uint8_t next;
            if (!copy(source + i + 1, &next, 1)) break;
            i += next < 0x30 ? 3 : 2;
            continue;
        }
        if ((b >= 6 && b <= 14) || b == 16 || (b >= 18 && b <= 42)) { i += 2; continue; }
        if (b < 0x2b) { ++i; continue; }
        int index = 0;
        if (b < 0x30) {
            index = (b - 0x2b) * 0xd0;
            if (++i >= length || !copy(source + i, &b, 1)) break;
        }
        index += b - 0x30;
        if (index >= 0 && index < _countof(charset) - 1) {
            wchar_t glyph=charset[index];
            if(textEncoding==1) for(const auto& replacement:polishGlyphs)
                if(replacement.index==index) { glyph=replacement.value; break; }
            current += glyph;
        }
        ++i;
    }
    finish();
    return parts;
}

std::wstring playerName(int character) {
    if (character < 0 || character >= 20) return {};
    // The field renderer uses FUN_00785070, including renamed characters and
    // aeons. The battle character array can be absent in a field menu.
    using Getter = uintptr_t(__cdecl*)(unsigned);
    const auto parts = decode(reinterpret_cast<Getter>(address(0x385070))(character), 40);
    return parts.empty() ? std::wstring{} : parts.front();
}
}
