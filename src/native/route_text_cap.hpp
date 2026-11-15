#pragma once
#include <string>
#include <vector>
namespace ffx::nav {
// Auto walk reads its route before walking; a long winding route read fourteen parts in the
// Djose trial. Keep the first SpokenRouteLegs legs (with any "bear left/right" between them)
// and count the rest as turns. The walk and its route cues still follow every leg.
constexpr unsigned SpokenRouteLegs=3;
inline std::wstring capRouteText(const std::wstring& directions,unsigned legs=SpokenRouteLegs) {
    std::vector<std::wstring> parts;
    for(size_t start=0;start<=directions.size();) {
        const auto end=directions.find(L", ",start);
        parts.push_back(directions.substr(start,end==std::wstring::npos?std::wstring::npos:end-start));
        if(end==std::wstring::npos) break;start=end+2;
    }
    const auto bend=[](const std::wstring& part) {return part.rfind(L"bear ",0)==0;};
    unsigned total=0;for(const auto& part:parts) if(!bend(part)) ++total;
    if(directions.empty() || total<=legs) return directions;
    std::wstring text;unsigned spoken=0;
    for(const auto& part:parts) {
        if(spoken==legs) break;
        if(bend(part) && !spoken) continue;
        if(!text.empty()) text+=L", ";
        text+=part;if(!bend(part)) ++spoken;
    }
    const auto rest=total-legs;
    return text+L", then "+std::to_wstring(rest)+(rest==1?L" more turn":L" more turns");
}
}
