#pragma once
#include "resource_reset.hpp"

namespace ffx {
inline unsigned nativeMenuLanguage() {
    if(!nativeGameThread() || nativeResourceLoading()) return 1;
    using Getter=unsigned(__cdecl*)();
    const auto get=reinterpret_cast<Getter>(address(0x241290));
    return get?get():1;
}

inline unsigned menuLanguageIndex(unsigned locale) {
    // FFX UI language IDs: Japanese through Italian (0–5), Chinese (9), Korean (10).
    return locale<=5?locale:locale==9?6u:locale==10?7u:1u;
}

struct MenuListText {
    const wchar_t* empty;
    const wchar_t* positionPrefix;
    const wchar_t* positionSeparator;
    const wchar_t* positionSuffix;
};

inline const MenuListText& menuListText(unsigned locale) {
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
    return labels[menuLanguageIndex(locale)];
}

struct MenuAvailabilityText {
    const wchar_t* unavailable;
    const wchar_t* option;
    const wchar_t* description;
    const wchar_t* on;
    const wchar_t* off;
};

inline const MenuAvailabilityText& menuAvailabilityText(unsigned locale) {
    static constexpr MenuAvailabilityText labels[] = {
        {L"\u4f7f\u7528\u3067\u304d\u307e\u305b\u3093", L"\u4f7f\u7528\u4e0d\u53ef\u306e\u8aad\u307f\u4e0a\u3052", L"\u30a2\u30a4\u30c6\u30e0\u3084\u30a2\u30d3\u30ea\u30c6\u30a3\u3001\u30d0\u30c8\u30eb\u306e\u30e1\u30cb\u30e5\u30fc\u3067\u64cd\u4f5c\u304c\u62d2\u5426\u3055\u308c\u305f\u3068\u304d\u306b\u3001\u4f7f\u7528\u3067\u304d\u306a\u3044\u3053\u3068\u3092\u8aad\u307f\u4e0a\u3052\u307e\u3059\u3002", L"\u30aa\u30f3", L"\u30aa\u30d5"},
        {L"Unavailable", L"Unavailable cue", L"Say Unavailable when the game rejects an action in the Items, Abilities, or battle menus.", L"On", L"Off"},
        {L"Indisponible", L"Annonce d\u2019indisponibilit\u00e9", L"Annonce une action indisponible lorsque le jeu la refuse dans les menus Objets, Comp\u00e9tences ou combat.", L"Activ\u00e9e", L"D\u00e9sactiv\u00e9e"},
        {L"No disponible", L"Aviso de acci\u00f3n no disponible", L"Anuncia que una acci\u00f3n no est\u00e1 disponible cuando el juego la rechaza en los men\u00fas de objetos, habilidades o combate.", L"Activado", L"Desactivado"},
        {L"Nicht verf\u00fcgbar", L"Hinweis auf nicht verf\u00fcgbare Aktionen", L"Meldet, wenn das Spiel eine Aktion im Gegenstands-, F\u00e4higkeiten- oder Kampfmen\u00fc ablehnt.", L"An", L"Aus"},
        {L"Non disponibile", L"Avviso di azione non disponibile", L"Segnala quando il gioco rifiuta un\u2019azione nei menu di oggetti, abilit\u00e0 o battaglia.", L"Attivo", L"Disattivato"},
        {L"\u65e0\u6cd5\u4f7f\u7528", L"\u65e0\u6cd5\u4f7f\u7528\u63d0\u793a", L"\u6e38\u620f\u62d2\u7edd\u9053\u5177\u3001\u80fd\u529b\u6216\u6218\u6597\u83dc\u5355\u4e2d\u7684\u64cd\u4f5c\u65f6\uff0c\u8bfb\u51fa\u65e0\u6cd5\u4f7f\u7528\u3002", L"\u5f00\u542f", L"\u5173\u95ed"},
        {L"\uc0ac\uc6a9\ud560 \uc218 \uc5c6\uc74c", L"\uc0ac\uc6a9 \ubd88\uac00 \uc54c\ub9bc", L"\uc544\uc774\ud15c, \uc5b4\ube4c\ub9ac\ud2f0 \ub610\ub294 \uc804\ud22c \uba54\ub274\uc5d0\uc11c \uac8c\uc784\uc774 \ub3d9\uc791\uc744 \uac70\ubd80\ud558\uba74 \uc0ac\uc6a9\ud560 \uc218 \uc5c6\ub2e4\uace0 \uc77d\uc2b5\ub2c8\ub2e4.", L"\ucf1c\uc9d0", L"\uaebc\uc9d0"},
    };
    return labels[menuLanguageIndex(locale)];
}
}
