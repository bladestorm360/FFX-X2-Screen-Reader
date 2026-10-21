#include "exit_labels.hpp"
#include "room_areas.hpp"
#include <array>
#include <algorithm>
#include <tuple>
#include "exit_destinations_data.hpp"
namespace ffx {
std::wstring navigationExitName(const AtelScript& script,unsigned worker,int room,const std::wstring& authored) {
    return navigationExitName(script,worker,room,authored,[](int index) {return nativeAreaName(index);});
}
std::wstring navigationExitName(const AtelScript& script,unsigned worker,int room,const std::wstring& authored,const std::function<std::wstring(int)>& lookup) {
    const auto it=std::lower_bound(std::begin(exitDestinations),std::end(exitDestinations),
        std::tuple<unsigned,std::string,unsigned>{script.checksum,script.name,worker},[](const ExitDestination& entry,const auto& key) {
            return std::tuple<unsigned,std::string,unsigned>{entry.checksum,entry.script,entry.worker}<key;
        });
    const bool known=it!=std::end(exitDestinations) && it->checksum==script.checksum && script.name==it->script && it->worker==worker;
    // This trigger starts the original Winno challenge, including its
    // childhood flashback. Name the visible minigame role, not room191's
    // generic Zanarkand area. The title is retained from its native tutorial.
    if(known && script.name=="swin0200" && script.checksum==0x273a7d2b && worker==34)
        return L"Exit 1 to Jecht Shot challenge";
    std::vector<std::wstring> names;
    if(known) for(const auto index:it->areas) {
        if(!index) break;
        const auto name=lookup(index);
        if(name.empty()) { names.clear(); break; }
        names.push_back(name);
    }
    else if(room>=0) {
        const auto name=lookup(areaIndex(room));
        if(!name.empty()) names.push_back(name);
    }
    // Different subareas of one native location share its own prefix. The
    // game's area selector is collision/ATEL state, not the arrival-point ID.
    // Never guess a specific floor from the warp's entrance number.
    std::wstring destination;
    if(names.size()>1) {
        const auto separator=names[0].find(L" - ");
        if(separator!=std::wstring::npos) {
            const auto prefix=names[0].substr(0,separator);
            if(std::all_of(names.begin(),names.end(),[&](const auto& name) { return name.find(prefix+L" - ")==0; })) destination=prefix;
        }
    }
    if(destination.empty()) for(const auto& name:names) { if(!destination.empty()) destination+=L" or "; destination+=name; }
    const bool number=!known || it->total>1 || destination.empty();
    auto label=L"Exit"+(number?L" "+std::to_wstring(known?it->number:worker+1):std::wstring{});
    if(!known && authored.size()>5 && authored.compare(0,5,L"Exit ")==0 &&
        std::all_of(authored.begin()+5,authored.end(),[](wchar_t c) { return c>=L'0' && c<=L'9'; })) label=authored;
    if(!destination.empty()) return label+L" to "+destination;
    if(authored.find(L"Exit to ")==0) return label+authored.substr(4);
    return label;
}
}
