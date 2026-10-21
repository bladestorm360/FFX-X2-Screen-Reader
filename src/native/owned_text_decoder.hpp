#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "direction_glyph.hpp"

namespace ffx {
enum class OwnedTextMode { Terminated, Records, LegacyRecords, Literal };

struct OwnedTextContext {
    bool polish = false;
    std::wstring primaryDodge;
    std::array<std::wstring, 256> glyphs{};
};

inline std::vector<std::wstring> decodeOwnedBytes(
    const std::uint8_t* bytes, std::size_t length, OwnedTextMode mode,
    const OwnedTextContext& context) {
    static constexpr wchar_t charset[] = L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~\u00b7\u3010\u3011\u266a\u2665 \u201c\u201d\u2014 \u00a1\u2191\u2193\u2190\u2192\u00a8\u00ab\u00ba \u00bb\u00bf\u00c0\u00c1\u00c2\u00c4\u00c7\u00c8\u00c9\u00ca\u00cb\u00cc\u00cd\u00ce\u00cf\u00d1\u00d2\u00d3\u00d4\u00d6\u00d9\u00da\u00db\u00dc\u00df\u00e0\u00e1\u00e2\u00e4\u00e7\u00e8\u00e9\u00ea\u00eb\u00ec\u00ed\u00ee\u00ef\u00f1\u00f2\u00f3\u00f4\u00f6\u00f9\u00fa\u00fb\u00fc,\u0192\u201e\u2026'\u2019\u2022-~\u2122 \u203a\u00a7\u00a9\u00aa\u00ae\u00b1\u00b2\u00b3\u00bc\u00bd\u00be\u00d7\u00f7\u2039\u2026 \u01ce\u2605\u2606\u25a0\u221e";
    struct PolishGlyph { int index; wchar_t value; };
    static constexpr PolishGlyph polishGlyphs[] = {
        {115,L'\u0104'},{116,L'\u015A'},{119,L'\u0106'},{121,L'\u0118'},
        {123,L'\u0141'},{128,L'\u0143'},{133,L'\u017B'},{134,L'\u0179'},
        {138,L'\u0105'},{142,L'\u0107'},{144,L'\u0119'},{150,L'\u0142'},
        {151,L'\u0144'},{154,L'\u015B'},{157,L'\u017A'},{159,L'\u017C'}
    };
    if (!bytes || length == 0 || length > 0x10000) return {};
    std::vector<std::wstring> parts;
    std::wstring current;
    auto finish = [&] {
        if (mode == OwnedTextMode::Literal) {
            if (!current.empty()) parts.push_back(current);
            current.clear();
            return;
        }
        const auto begin = current.find_first_not_of(L" \r\n\t");
        if (begin != std::wstring::npos)
            parts.push_back(current.substr(begin, current.find_last_not_of(L" \r\n\t") - begin + 1));
        current.clear();
    };
    auto fallbackButton = [](std::uint8_t glyph) -> std::wstring {
        static const wchar_t* names[] = {L"Triangle",L"Circle",L"Cross",L"Square",L"L1",L"L2",L"R1",L"R2",L"Start",L"Select"};
        if (glyph >= 0x30 && glyph < 0x3a) return names[glyph - 0x30];
        if (glyph >= 0x40 && glyph < 0x50) {
            std::wstring text;
            const wchar_t* directions[] = {L"Up",L"Right",L"Down",L"Left"};
            for (unsigned i = 0; i < 4; ++i) if (glyph & (1u << i)) {
                if (!text.empty()) text += directionGlyphSeparator(glyph);
                text += directions[i];
            }
            return text.empty() ? L"directional pad" : text;
        }
        if (glyph >= 0x20 && glyph <= 0x22) return L"left stick";
        return L"controller button";
    };
    for (std::size_t i = 0; i < length;) {
        const std::uint8_t b = bytes[i];
        if (b == 0) {
            finish();
            if (mode == OwnedTextMode::Terminated || mode == OwnedTextMode::Literal) break;
            ++i;
            continue;
        }
        if (b == 3 || b == 10) {
            finish();
            const std::size_t skip = b == 10 && mode != OwnedTextMode::LegacyRecords ? 2 : 1;
            i += std::min(skip, length - i);
            continue;
        }
        if (mode == OwnedTextMode::LegacyRecords) {
            if (b == 9 || b == 16) { i += std::min<std::size_t>(2, length - i); continue; }
            if (b < 0x2b) { ++i; continue; }
        }
        if (b == 11) {
            if (length - i < 2) break;
            const auto glyph = bytes[i + 1];
            current += context.glyphs[glyph].empty() ? fallbackButton(glyph) : context.glyphs[glyph];
            i += 2;
            continue;
        }
        if (b == 4) {
            if (length - i < 2) break;
            const auto next = bytes[i + 1];
            const std::size_t skip = next < 0x30 ? 3 : 2;
            i += std::min(skip, length - i);
            continue;
        }
        if ((b >= 6 && b <= 14) || b == 16 || (b >= 18 && b <= 42)) {
            i += std::min<std::size_t>(2, length - i);
            continue;
        }
        if (b < 0x2b) { ++i; continue; }
        int index = 0;
        std::uint8_t character = b;
        if (b < 0x30) {
            index = (b - 0x2b) * 0xd0;
            if (i + 1 >= length) break;
            character = bytes[++i];
        }
        index += character - 0x30;
        if (index >= 0 && index < static_cast<int>(sizeof(charset) / sizeof(charset[0])) - 1) {
            wchar_t decoded = charset[index];
            if (context.polish) for (const auto& replacement : polishGlyphs)
                if (replacement.index == index) { decoded = replacement.value; break; }
            current += decoded;
        }
        ++i;
    }
    finish();
    return parts;
}
} // namespace ffx
