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
    // A run is one spoken turn: its walked length, labelled by its overall heading.
    struct Run {Point start,end,first;double count;};
    std::vector<Run> runs;
    // cos(15 degrees): well under half an octant, and below the 18 degree bends a
    // listener should still hear as a separate turn.
    constexpr double mergeCosine=.96592582628906829;
    for(size_t i=1;i<points.size();++i) {
        const double count=distanceXZ(points[i-1],points[i])/units;
        if(count<=.005) continue;
        const Point vector{points[i].x-points[i-1].x,0,points[i].z-points[i-1].z};
        // Rounded compass labels do not prove a straight walk. Preserve real
        // bends instead of summing their lengths into one misleading instruction.
        // Each leg is compared with the run's overall heading, not the previous
        // leg, so a gradual curve cannot drift into one long straight.
        if(!runs.empty()) {
            auto& run=runs.back();
            const Point chord{run.end.x-run.start.x,0,run.end.z-run.start.z};
            const double dot=vector.x*chord.x+vector.z*chord.z,size=std::hypot(vector.x,vector.z)*std::hypot(chord.x,chord.z);
            if(size>0 && dot>=size*mergeCosine) {run.end=points[i];run.count+=count;continue;}
        }
        runs.push_back({points[i-1],points[i],vector,count});
    }
    std::vector<std::wstring> parts;std::wstring previous;Point chord{};
    for(const auto& run:runs) {
        const Point next{run.end.x-run.start.x,0,run.end.z-run.start.z};
        auto label=direction(next);
        // Two turns that round to the same word would sound like one straight
        // line said twice. Name the bend between them instead. The run's first
        // leg is the one that broke the previous run, so its side is reliable.
        // Positive cross products turn toward camera-left (see cameraDirectionLabel).
        if(!parts.empty() && label==previous) {
            const double cross=chord.x*run.first.z-chord.z*run.first.x;
            parts.push_back(cross>0?L"bear left":L"bear right");
        }
        parts.push_back(routeSegmentText(run.count,label,partial));previous=std::move(label);chord=next;
    }
    return join(parts);
}
}
