#pragma once
#include "navigation.hpp"
#include <string>
#include <cmath>
#include <algorithm>
#include <cwchar>
namespace ffx {
inline std::wstring routeDistanceText(double steps,bool partial) {
    wchar_t text[40]{};
    if(partial) swprintf_s(text,L"%.1f",steps);
    else swprintf_s(text,L"%.0f",std::max(1.0,std::round(steps)));
    return text;
}
inline std::wstring routeSegmentText(double steps,const std::wstring& direction,bool partial) {
    return routeDistanceText(steps,partial)+L" "+direction;
}
inline std::wstring routeDirectionsText(const std::vector<Point>& points,double units,bool partial) {
    std::vector<std::pair<double,std::wstring>> merged;
    Point previous{};
    for(size_t i=1;i<points.size();++i) {
        const double count=distanceXZ(points[i-1],points[i])/units;
        if(count<=.005) continue;
        const Point vector{points[i].x-points[i-1].x,0,points[i].z-points[i-1].z};
        const auto heading=direction(vector);
        const double cross=vector.x*previous.z-vector.z*previous.x,dot=vector.x*previous.x+vector.z*previous.z;
        // Rounded compass labels do not prove a straight walk. Preserve real
        // bends instead of summing their lengths into one misleading instruction.
        if(!merged.empty() && merged.back().second==heading && dot>0 && cross*cross<=dot*dot*1e-12) merged.back().first+=count;
        else merged.push_back({count,heading});
        previous=vector;
    }
    std::vector<std::wstring> parts;
    for(const auto& segment:merged) parts.push_back(routeSegmentText(segment.first,segment.second,partial));
    return join(parts);
}
}
