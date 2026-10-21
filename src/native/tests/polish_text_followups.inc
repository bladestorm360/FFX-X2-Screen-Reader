// Include inside a native fixture translation unit.
#include "../text_encoding.hpp"
#include <stdexcept>
namespace ffx {
namespace {
int polishFixtureCalls=0;
bool polishFixtureKnown=false,polishFixtureValue=false;
bool polishFixtureDetector(bool& known) { ++polishFixtureCalls; known=polishFixtureKnown; return polishFixtureValue; }
void polishTextFixtures() {
    const auto decodeBytes=[](const uint8_t* bytes,size_t count) { return decode(reinterpret_cast<uintptr_t>(bytes),count); };
    static const uint8_t prefix[]={0x2b};
    static const uint8_t controls[]={11,0x31,3,11,0x32,0};
    const int prior=setting(Setting::PolishText);
    setSetting(Setting::PolishText,0);
    polishFixtureCalls=0; polishFixtureKnown=false; polishFixtureValue=true; setTextEncodingDetector(polishFixtureDetector);
    if(!decodeBytes(prefix,sizeof(prefix)).empty()) throw std::runtime_error("truncated glyph prefix");
    if(polishFixtureCalls!=1) throw std::runtime_error("Polish probe first attempt");
    polishFixtureKnown=true;
    decodeBytes(prefix,sizeof(prefix));
    if(polishFixtureCalls!=2) throw std::runtime_error("unknown Polish probe did not retry");
    setSetting(Setting::PolishText,1); polishFixtureCalls=0;
    decodeBytes(prefix,sizeof(prefix));
    if(polishFixtureCalls!=0) throw std::runtime_error("forced Polish called detector");
    setSetting(Setting::PolishText,0); polishFixtureKnown=true; polishFixtureValue=false; setTextEncodingDetector(polishFixtureDetector);
    const uint8_t stockWord[]={0x2b,0x30+119,0};
    auto stock=decodeBytes(stockWord,sizeof(stockWord));
    if(stock.empty() || stock[0].size()!=1 || stock[0][0]==L'\u0106') throw std::runtime_error("stock capital C distinction");
    const uint8_t stockAccent[]={0x30+115,0};
    auto accented=decodeBytes(stockAccent,sizeof(stockAccent));
    if(accented.empty() || accented[0]!=L"\u00c0") throw std::runtime_error("stock accented glyph");
    polishFixtureValue=true; setTextEncodingDetector(polishFixtureDetector);
    auto polish=decodeBytes(stockWord,sizeof(stockWord));
    if(polish.empty() || polish[0].size()!=1 || polish[0][0]!=L'\u0106') throw std::runtime_error("Polish capital C glyph");
    static constexpr struct { int index; wchar_t value; } expected[]={
        {115,L'\u0104'},{116,L'\u015a'},{119,L'\u0106'},{121,L'\u0118'},{123,L'\u0141'},{128,L'\u0143'},{133,L'\u017b'},{134,L'\u0179'},
        {138,L'\u0105'},{142,L'\u0107'},{144,L'\u0119'},{150,L'\u0142'},{151,L'\u0144'},{154,L'\u015b'},{157,L'\u017a'},{159,L'\u017c'} };
    for(const auto& g:expected) {
        const uint8_t encoded[]={static_cast<uint8_t>(0x30+g.index),0};
        auto decoded=decodeBytes(encoded,sizeof(encoded));
        if(decoded.empty() || decoded[0]!=std::wstring(1,g.value)) throw std::runtime_error("Polish glyph table");
    }
    auto ctl=decodeBytes(controls,sizeof(controls));
    if(ctl.empty() || ctl[0].find(buttonGlyph(0x31))==std::wstring::npos) throw std::runtime_error("Polish controls changed");
    auto metadata=settingInfo(Setting::PolishText);
    if(metadata.initial!=0 || metadata.minimum!=0 || metadata.maximum!=1 || metadata.key[0]!=L'p') throw std::runtime_error("Polish setting persistence metadata");
    setTextEncodingDetector(nullptr); setSetting(Setting::PolishText,prior);
}
}
}
