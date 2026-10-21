#include "events.hpp"
#include "text_encoding.hpp"
#include "game_visual_text.hpp"
#include <algorithm>
#include <cwctype>

namespace ffx {
namespace {
bool exactRawName(unsigned index,const uint8_t* expected,size_t size) {
    using Name=uintptr_t(__cdecl*)(unsigned,unsigned);
    const auto encoded=reinterpret_cast<Name>(address(0x46c3c0))(index,0);
    if(encoded<0x10000) return false;
    for(size_t i=0;i<size;++i) { uint8_t byte=0; if(!copy(encoded+i,&byte,1) || byte!=expected[i]) return false; }
    return size && expected[size-1]==0;
}
bool detectPolishText(bool& known) {
    known=false;
    if(!nativeGameThread()) return false;
    using Table=uintptr_t(__cdecl*)(unsigned);
    const auto table=reinterpret_cast<Table>(address(0x4b7f10))(0);
    if(table<0x10000) return false;
    const auto bound=read<uint16_t>(table)>>2;
    if(bound<=44 || bound>0x4000) return false;
    static constexpr uint8_t stock0[]={93,126,125,116,0};
    static constexpr uint8_t polish0[]={93,120,114,0};
    static constexpr uint8_t stock44[]={81,116,130,112,120,115,58,71,58,99,129,120,112,123,130,0};
    static constexpr uint8_t polish44[]={81,116,130,112,120,115,70,58,90,123,112,130,137,131,126,129,58,95,129,201,113,0};
    const bool s0=exactRawName(0,stock0,sizeof(stock0)), p0=exactRawName(0,polish0,sizeof(polish0));
    const bool s44=exactRawName(44,stock44,sizeof(stock44)), p44=exactRawName(44,polish44,sizeof(polish44));
    if(s0 && s44) { known=true; return false; }
    if(p0 && p44) { known=true; return true; }
    return false;
}
}
const wchar_t* battleGraphicLabel(BattleGraphic label) {
    using Getter=unsigned(__cdecl*)();
    const auto locale=reinterpret_cast<Getter>(address(0x241290))();
    // 4AC710 selects the alternate English atlas for the three Asian UI locales.
    const bool english=(locale==0 || locale==9 || locale==10) &&
        reinterpret_cast<Getter>(address(0x385290))()!=0;
    static constexpr const wchar_t* labels[][3]={
        {L"MISS",L"GUARD",L"OVER KILL"},
        {L"MISS",L"IMMUNE",L"OVERKILL"},
        {L"RAT\u00c9",L"GARDE",L"OVERKILL"},
        {L"Fallo",L"Guardia",L"Exterminio"},
        {L"DANEBEN",L"IMMUN",L"VERNICHTEND"},
        {L"MANCATO",L"Guardia",L"Ultracidio"}
    };
    if((locale>5 && locale!=9 && locale!=10) || static_cast<unsigned>(label)>=3) return L"";
    const unsigned row=english?1:locale<=5?locale:0;
    return labels[row][static_cast<unsigned>(label)];
}
std::wstring battleGraphicText(BattleGraphic label) {return battleGraphicLabel(label);}
void installTextEncoding() { setTextEncodingDetector(detectPolishText); }
std::wstring first(uintptr_t source, size_t length) {
    const auto parts = decode(source, length);
    return parts.empty() ? std::wstring{} : parts.front();
}
std::wstring join(const std::vector<std::wstring>& parts, const wchar_t* separator) {
    std::wstring out;
    for (const auto& part : parts) if (!part.empty()) {
        if (!out.empty()) out += separator;
        out += part;
    }
    return out;
}
std::wstring ascii(uintptr_t source, size_t length) {
    std::string bytes;
    for (size_t i = 0; i < length; ++i) {
        const auto b = read<uint8_t>(source + i);
        if (!b) break;
        bytes += static_cast<char>(b);
    }
    if (bytes.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    const auto codepage = size ? CP_UTF8 : CP_ACP;
    const int count = size ? size : MultiByteToWideChar(codepage, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    std::wstring out(count, 0);
    MultiByteToWideChar(codepage, 0, bytes.data(), static_cast<int>(bytes.size()), &out[0], count);
    return out;
}
std::wstring clean(std::wstring value) {
    auto hasType = [](wchar_t c, WORD wanted) {
        WORD type=0;
        return GetStringTypeW(CT_CTYPE1,&c,1,&type) && (type&wanted)!=0;
    };
    while (value.size() > 1 && !hasType(value.front(),C1_ALPHA|C1_DIGIT) && value.front() != L' ') value.erase(value.begin());
    if (value.size() > 1 && hasType(value[0],C1_DIGIT) && hasType(value[1],C1_ALPHA)) value.erase(value.begin());
    auto start = value.find_first_not_of(L" \r\n\t"), end = value.find_last_not_of(L" \r\n\t");
    return start == std::wstring::npos ? std::wstring{} : value.substr(start, end - start + 1);
}
std::wstring menuText(int space, unsigned id) {
    using Language = unsigned(__cdecl*)();
    using Getter = uintptr_t(__cdecl*)(int, unsigned, unsigned);
    const auto lang = reinterpret_cast<Language>(address(0x3852b0))();
    return first(reinterpret_cast<Getter>(address(0x38fd40))(space, id, lang));
}
std::wstring recordName(unsigned getter, unsigned id) {
    using Getter = uintptr_t(__cdecl*)(unsigned, uintptr_t*);
    uintptr_t base = 0;
    const auto record = reinterpret_cast<Getter>(address(getter))(id, &base);
    using Language=unsigned(__cdecl*)();
    return record && base ? first(base + read<uint16_t>(record+(reinterpret_cast<Language>(address(0x3852b0))()&1)*4)) : std::wstring{};
}
std::wstring nativeText(unsigned getter, unsigned id) {
    using Getter = uintptr_t(__cdecl*)(unsigned);
    return first(reinterpret_cast<Getter>(address(getter))(id));
}
uint8_t equippedItem(unsigned character,bool armor) {
    if(character>=18) return 255;
    using Getter=uint8_t(__cdecl*)(unsigned);
    return reinterpret_cast<Getter>(address(armor?0x4a97d0:0x4a9c20))(character);
}
std::wstring equippedItemName(unsigned character,bool armor) {
    const auto id=equippedItem(character,armor);
    if(id==255) {
        const auto none=menuText(11,0x17);
        return none.empty()?L"Nothing equipped":none;
    }
    const auto text=id<200?nativeText(0x3abe10,id):std::wstring{};
    if(!text.empty()) return text;
    recordLog("equipment name unavailable character=%u armor=%d index=%u",character,armor,id);
    return L"Equipment name unavailable";
}
unsigned quantity(unsigned id) {
    using Getter = unsigned(__cdecl*)(unsigned);
    return reinterpret_cast<Getter>(address(0x390500))(id);
}
bool inBattle() { return read<uint8_t>(address(0xd2a8e0)) != 0; }
std::wstring battleName(unsigned id) {
    if (id >= 31) return {};
    const auto array = read<uintptr_t>(address(0xd334cc));
    auto name = array ? first(array + id * 0xf90 + 0x540, 40) : std::wstring{};
    return !name.empty() ? name : playerName(id);
}
}
