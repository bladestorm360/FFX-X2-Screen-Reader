// Image-menu labels verified in all eight MetaMenu.vbf language variants.
// Keep game logos as displayed; localize only text that actually changes.
// The caller supplies the current UI-thread LanguagePath on every presentation,
// so returning from native settings cannot leave an old-language label cached.
// See Docs/features/distribution/launcher_languages_20261023.md for asset hashes.
namespace FfxLauncher {
ref class MenuLabels abstract sealed {
public:
    static String^ Name(String^ action,String^ path) {
        if(path==nullptr) path=L"";
        path=path->ToLowerInvariant();
        if(path==L"en") path=L"us";
        if(path==L"es") path=L"sp";
        if(action==L"OnFFXClicked") return L"Final Fantasy X";
        if(action==L"OnSideStoryClicked") return path==L"jp"?
            L"Final Fantasy X \u6c38\u9060\u306e\u30ca\u30ae\u7bc0":L"Final Fantasy X Eternal Calm";
        if(action==L"OnFFX2Clicked") return L"Final Fantasy X-2";
        if(action==L"OnLastMissionClicked") return L"Final Fantasy X-2 Last Mission";
        if(action==L"OnCreditClicked") {
            if(path==L"de") return L"Abspann & Extras";
            if(path==L"fr") return L"Cr\u00e9dits & contenu bonus";
            if(path==L"it") return L"Riconoscimenti & Extra";
            if(path==L"sp") return L"Cr\u00e9ditos y contenido extra";
            return L"Credits & Bonus Audio";
        }
        if(action==L"OnCredit2014Clicked") {
            if(path==L"de") return L"Weitere Mitwirkende";
            if(path==L"fr") return L"Cr\u00e9dits suppl\u00e9mentaires";
            if(path==L"it") return L"Riconoscimenti aggiuntivi";
            if(path==L"sp") return L"Cr\u00e9ditos adicionales";
            return L"Additional Credits";
        }
        if(action==L"OnConfigClicked") {
            if(path==L"ch") return L"\u8a2d\u7f6e";
            if(path==L"kr") return L"\uc124\uc815";
            if(path==L"de") return L"Einstellung";
            if(path==L"fr") return L"Param\u00e8tres";
            if(path==L"it") return L"Impostazioni";
            if(path==L"sp") return L"Configuraci\u00f3n";
            return L"Setting"; // Japanese artwork also displays English SETTING.
        }
        return L""; // Unknown actions retain native behavior without invented text.
    }
};
}
