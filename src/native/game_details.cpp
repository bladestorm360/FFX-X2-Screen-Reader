#include "events.hpp"
#include "settings.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
namespace ffx {
namespace {
using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
std::array<uint16_t,4> abilities(uintptr_t record) {
    std::array<uint16_t,4> result{0xff,0xff,0xff,0xff};
    const auto slots=read<uint8_t>(record+0xb);
    if(slots>4) return result;
    // 8D8A70 renders all four named records, then the slot icons. Its
    // 798EC0 normalization makes the visible slot count at least this count.
    for(unsigned i=0;i<4;++i) result[i]=read<uint16_t>(record+0xe+i*2,0xff);
    return result;
}
bool named(unsigned id) { return id && id!=0xff && id!=0xffff; }
}
std::wstring recordDescription(unsigned getter,unsigned id) {
    // Command dispatcher returns dummy name-only records for plain commands,
    // characters and equipment. Those records have no description field.
    if(getter==0x39a4c0) {
        const auto type=id>>12;
        if(type!=2 && type!=3 && type!=4 && type!=6 && type!=8) return {};
    }
    using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    uintptr_t base=0;
    const auto record=reinterpret_cast<Getter>(address(getter))(id,&base);
    using Language=unsigned(__cdecl*)();
    if(!record || !base) return {};
    const auto offset=read<uint16_t>(record+8+(reinterpret_cast<Language>(address(0x3852b0))()&1)*4);
    return join(decode(base+offset,1024),L" ");
}
std::wstring textDescription(uintptr_t source) {
    const auto parts=decode(source,1024);
    if(parts.size()>2 && parts[1]==L"-") return join(std::vector<std::wstring>(parts.begin()+2,parts.end()),L" ");
    return {};
}
std::wstring menuDescription(int space,unsigned id) {
    using Getter=uintptr_t(__cdecl*)(int,unsigned,unsigned);
    using Language=unsigned(__cdecl*)();
    const auto language=reinterpret_cast<Language>(address(0x3852b0))();
    // 4D19F0 -> 4DDF80 displays this independent help field for mode rows.
    // The name getter has no compound description in these command records.
    if(space==1 && id>=0x1033 && id<=0x104f)
        return join(decode(reinterpret_cast<Getter>(address(0x38fc00))(space,id,language),1024,TextMode::Terminated),L" ");
    return textDescription(reinterpret_cast<Getter>(address(0x38fd40))(space,id,language));
}
std::wstring equipmentName(uintptr_t record) {
    if(!record) return {};
    using Name=uintptr_t(__cdecl*)(unsigned,unsigned,int,uintptr_t);
    using Language=int(__cdecl*)();
    return first(reinterpret_cast<Name>(address(0x3a0c70))(read<uint16_t>(record),
        read<uint8_t>(record+4),reinterpret_cast<Language>(address(0x3852b0))(),0));
}
std::wstring equipmentDetails(uintptr_t record) {
    if(!record) return {};
    const auto slots=read<uint8_t>(record+0xb);
    if(slots>4) return {};
    std::vector<std::wstring> parts;
    unsigned filled=0;
    for(auto id:abilities(record)) {
        if(named(id)) { ++filled; parts.push_back(recordName(0x39a4c0,id)); }
    }
    const unsigned empty=std::max<unsigned>(slots,filled)-filled;
    if(empty) parts.push_back(std::to_wstring(empty)+(empty==1?L" empty slot":L" empty slots"));
    return join(parts);
}
std::wstring equipmentDescription(uintptr_t record) {
    return equipmentDetails(record);
}
std::wstring characterDescription(unsigned id) {
    if(id>=20) return {};
    using Saved=uintptr_t(__cdecl*)(unsigned);
    const auto record=reinterpret_cast<Saved>(address(0x3853f0))(id);
    if(!record) return {};
    return L"HP "+std::to_wstring(read<unsigned>(record+0x1c))+L"/"+std::to_wstring(read<unsigned>(record+0x24))+
        L", MP "+std::to_wstring(read<unsigned>(record+0x20))+L"/"+std::to_wstring(read<unsigned>(record+0x28))+
        L", Sphere Levels "+std::to_wstring(read<uint8_t>(record+0x3b));
}
std::wstring hpReadout(int current,int maximum) {
    auto text=L"HP "+std::to_wstring(current);
    if(maximum<=0) return text;
    if(setting(Setting::ShowMaximumHp)) return text+L"/"+std::to_wstring(maximum);
    const auto percent=std::clamp<int64_t>((static_cast<int64_t>(current)*100+maximum/2)/maximum,0,100);
    return text+L" ("+std::to_wstring(percent)+L"%)";
}
std::wstring battleMpReadout(int current,int maximum) {
    auto text=L"MP "+std::to_wstring(current);
    if(setting(Setting::ShowMaximumMp) && maximum>0) text+=L"/"+std::to_wstring(maximum);
    return text;
}
std::wstring battleSelectionVitals(unsigned command) {
    using Character=uintptr_t(__cdecl*)(unsigned);
    const auto vitals=[](unsigned id) -> std::wstring {
        if(id>=18) return {};
        const auto actor=reinterpret_cast<Character>(address(0x394030))(id);
        // 492C20 draws current combat HP/MP through 39ADE0/39AE60. Reserve
        // actors are initialized (+10), but intentionally not active (+DC8).
        if(!actor || !read<uint8_t>(actor+0x10)) return {};
        // Native max-HP getter39AE00 returns effective actor+594 for HP color
        // rendering. Original38D330 updates it when capacity changes, retaining
        // the independent base maximum at+59C; never reconstruct the maximum.
        return hpReadout(read<int>(actor+0x5d0),read<int>(actor+0x594))+
            L", MP "+std::to_wstring(read<int>(actor+0x5d4));
    };
    if(command!=0x30ff) return vitals(command&255);
    // The native special Summon row displays the three Magus actors separately.
    std::vector<std::wstring> parts;
    for(unsigned id=15;id<18;++id) {
        const auto text=vitals(id);
        if(!text.empty()) parts.push_back(battleName(id)+L", "+text);
    }
    return join(parts,L". ");
}
std::wstring shopEquipmentDescription(uintptr_t record) {
    const auto owner=read<uint8_t>(address(0x146ab60),255),type=read<uint8_t>(record+5,255);
    if(!record || owner>=7 || read<uint8_t>(record+4,255)>=7 || type>1) return {};
    using Character=uintptr_t(__cdecl*)(unsigned);
    const auto saved=reinterpret_cast<Character>(address(0x3853f0))(owner);
    if(!saved) return {};
    const auto index=read<uint8_t>(saved+0x2d+type,255);
    const auto current=index==255?0:reinterpret_cast<Gear>(address(0x3abbf0))(index,nullptr);
    // 8D85F0 displays current and selected equipment panels. 8D8A70/8F4F10
    // display ability names only; catalog effects and calculated changes are
    // not drawn. Keep equal names on distinct panels, with their context.
    const auto status=characterDescription(owner);
    return playerName(owner)+L". "+status.substr(0,status.find(L", Sphere Levels"))+L". Current equipment: "+(current?equipmentName(current)+L", "+equipmentDetails(current):L"None")+
        L". Selected equipment: "+equipmentDetails(record);
}
}
