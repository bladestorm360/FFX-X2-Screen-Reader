#pragma once

namespace ffx {
struct MenuListText {
    const wchar_t* empty;
    const wchar_t* positionPrefix;
    const wchar_t* positionSeparator;
    const wchar_t* positionSuffix;
};

inline const MenuListText& menuListText(unsigned locale) {
    // FFX menu-language IDs: Japanese through Italian (0–5), Chinese (9), Korean (10).
    static constexpr MenuListText labels[] = {
        {L"\u7a7a\u304d", L"", L"\u756a\u76ee\uff08\u5168", L"\u4ef6\uff09"},
        {L"Empty", L"", L" of ", L""},
        {L"Vide", L"", L" sur ", L""},
        {L"Vac\u00edo", L"", L" de ", L""},
        {L"Leer", L"", L" von ", L""},
        {L"Vuoto", L"", L" di ", L""},
        {L"\u7a7a", L"\u7b2c", L"\u9879\uff0c\u5171", L"\u9879"},
        {L"\ube44\uc5b4 \uc788\uc74c", L"", L"\ubc88\uc9f8 (\uc804\uccb4 ", L"\uac1c)"},
    };
    const auto row = locale <= 5 ? locale : locale == 9 ? 6u : locale == 10 ? 7u : 1u;
    return labels[row];
}
}
