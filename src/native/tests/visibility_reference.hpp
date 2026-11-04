// Previous mod implementation, retained only as an independent regression oracle.
// This is the user's mod code, not extracted/decompiled game implementation.
// Source baseline SHA256: 97e906d5c0b6e93c5a9cde5301550d1b25bfb4e176086dc4499304dd5f3e65b0
// Production must never include this file. Keep route visibility/crossing/height
// parity explicit when changing the accelerated smoother.
#pragma once
#include "../navigation.hpp"
#include "../navigation_cancel.hpp"
#include <algorithm>
#include <cmath>
namespace ffx::visibility_reference {
inline bool contains(const Triangle& t, Point p, double tolerance) {
    if (!t.valid) return false;
    const double ax = t.points[2].x - t.points[0].x, az = t.points[2].z - t.points[0].z;
    const double bx = t.points[1].x - t.points[0].x, bz = t.points[1].z - t.points[0].z;
    const double cx = p.x - t.points[0].x, cz = p.z - t.points[0].z;
    const double aa = ax * ax + az * az, ab = ax * bx + az * bz, ac = ax * cx + az * cz;
    const double bb = bx * bx + bz * bz, bc = bx * cx + bz * cz, determinant = aa * bb - ab * ab;
    if (std::abs(determinant) < .0001) return false;
    const double u = (bb * ac - ab * bc) / determinant, v = (aa * bc - ab * ac) / determinant;
    return u >= -tolerance && v >= -tolerance && u + v <= 1 + tolerance;
}
inline bool walkable(const Triangle& t) { return t.valid && (t.collision == 0 || t.collision == 2); }
inline double floorHeight(const Triangle& t, Point p) {
    const auto& a = t.points[0]; const auto& b = t.points[1]; const auto& c = t.points[2];
    const double determinant = (b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    if (std::abs(determinant)<.0001) return t.center.y;
    const double u = ((b.z-c.z)*(p.x-c.x)+(c.x-b.x)*(p.z-c.z))/determinant;
    const double v = ((c.z-a.z)*(p.x-c.x)+(a.x-c.x)*(p.z-c.z))/determinant;
    return u*a.y+v*b.y+(1-u-v)*c.y;
}

inline bool lineOfSight(const Mesh& mesh, Point a, Point b) {
    if(!routeSegmentAllowed(mesh,a,b)) return false;
    // Cover the complete segment with clipped triangle intervals. Sampling
    // every five units can skip a narrow wall or gap between two sample points.
    std::vector<std::pair<double,double>> intervals;
    for(const auto& t:mesh.triangles) {
        nav::checkComputation();if(!walkable(t)) continue;
        const auto p=t.points[0],q=t.points[1],r=t.points[2];
        const double determinant=(q.z-r.z)*(p.x-r.x)+(r.x-q.x)*(p.z-r.z);
        if(std::abs(determinant)<.0001) continue;
        auto coordinates=[&](Point point) {
            const double u=((q.z-r.z)*(point.x-r.x)+(r.x-q.x)*(point.z-r.z))/determinant;
            const double v=((r.z-p.z)*(point.x-r.x)+(p.x-r.x)*(point.z-r.z))/determinant;
            return std::array<double,3>{u,v,1-u-v};
        };
        const auto first=coordinates(a),last=coordinates(b);
        double low=0,high=1;
        auto clip=[&](double start,double finish) {
            const double change=finish-start;
            if(std::abs(change)<.0000001) { if(start<-.000001) high=-1; }
            else if(change>0) low=std::max(low,-start/change);
            else high=std::min(high,-start/change);
        };
        for(unsigned i=0;i<3;++i) clip(first[i],last[i]);
        if(!mesh.fixedHeight) {
            const double startHeight=floorHeight(t,a)-a.y,endHeight=floorHeight(t,b)-b.y;
            clip(8-startHeight,8-endHeight); clip(8+startHeight,8+endHeight);
        }
        if(low<=high && high>=0 && low<=1) intervals.push_back({std::max(0.0,low),std::min(1.0,high)});
    }
    std::sort(intervals.begin(),intervals.end(),[](const auto& a,const auto& b) {nav::checkComputation();return a<b;});
    double covered=0;
    for(const auto& interval:intervals) {
        if(interval.first>covered+.000001) return false;
        covered=std::max(covered,interval.second);
        if(covered>=1-.000001) return true;
    }
    return false;
}

inline std::vector<Point> smooth(const Mesh& mesh, const std::vector<Point>& points) {
    if(!mesh.events.ready) return {};
    for(size_t i=1;i<points.size();++i) if(!routeSegmentAllowed(mesh,points[i-1],points[i])) return {};
    if (points.size() <= 2) return points;
    std::vector<Point> out{points[0]};
    for (size_t i = 0; i < points.size() - 1;) {
        nav::checkComputation();
        size_t next = i + 1;
        for (size_t j = points.size() - 1; j > i + 1; --j) if (lineOfSight(mesh, points[i], points[j])) { next = j; break; }
        out.push_back(points[next]); i = next;
    }
    return out;
}

inline std::vector<Point> routePoints(const Mesh& mesh,Point start,Point end,int endTriangle) {
    if(!mesh.events.ready) return {};
    if(mesh.openPlane) {
        if(!std::isfinite(start.x) || !std::isfinite(start.y) || !std::isfinite(start.z) ||
            !std::isfinite(end.x) || !std::isfinite(end.y) || !std::isfinite(end.z)) return {};
        end.y=mesh.fixedHeight?start.y:mesh.planeY; return {start,end};
    }
    const auto route=path(mesh,findTriangle(mesh,start),endTriangle,true,&start);
    if(route.empty()) return {};
    std::vector<Point> points{start};
    for(size_t i=0;i+1<route.size();++i) {
        nav::checkComputation();
        const auto& a=mesh.triangles[route[i]]; const auto& b=mesh.triangles[route[i+1]];
        Point portal; if(!floorPortal(a,b,portal)) return {};
        points.push_back(portal);
    }
    // Event targets just outside the floor end at the nearest boundary.
    const auto& triangle=mesh.triangles[route.back()];
    if(!contains(triangle,end,0)) {
        double best=1e18; Point projected=end;
        for(unsigned edge=0;edge<3;++edge) {
            const auto a=triangle.points[edge],b=triangle.points[(edge+1)%3];
            const double dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz; if(length<.0001) continue;
            const double fraction=std::clamp(((end.x-a.x)*dx+(end.z-a.z)*dz)/length,0.0,1.0);
            const Point candidate{a.x+fraction*dx,a.y+fraction*(b.y-a.y),a.z+fraction*dz};
            const double distance=distanceXZ(candidate,end);
            if(distance<best) { best=distance; projected=candidate; }
        }
        end=projected;
    } else end.y=floorHeight(triangle,end);
    points.push_back(end);
    // Native swimming X/Z follows this component independently of Y. A cached
    // projection must not lose its route when native buoyancy changes depth.
    if(mesh.fixedHeight) for(auto& point:points) point.y=start.y;
    return visibility_reference::smooth(mesh,points);
}
}
