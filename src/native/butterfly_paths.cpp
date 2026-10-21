#include "butterfly_paths.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace ffx {
namespace {
constexpr double epsilon=1e-8;
constexpr unsigned ringCount=32;
constexpr double turn=6.2831853071795864769;
bool finite(Point p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
double floatRoundoff(Point a,Point b) {
    // Native mesh/actor coordinates are floats. Cover interpolation rounding
    // at their actual magnitude; this is not a world-distance tolerance.
    const double scale=std::max({1.0,std::abs(a.x),std::abs(a.y),std::abs(a.z),
        std::abs(b.x),std::abs(b.y),std::abs(b.z)});
    return 8*std::numeric_limits<float>::epsilon()*scale;
}
bool validZone(const Entity& e) {
    const auto& z=e.zone;
    return !e.inactive && z.valid && z.shape==1 && finite(z.center) &&
        std::isfinite(z.range) && z.range>0 && std::isfinite(z.contactMinY) &&
        std::isfinite(z.contactMaxY) && z.contactMinY<=z.contactMaxY;
}
bool touches(Point a,Point b,const InteractionZone& z) {
    // Clip the finite parameter interval by vertical overlap BEFORE minimizing
    // horizontal distance. Tangency is conservatively treated as hazardous.
    double low=0,high=1;
    const double rounding=floatRoundoff(a,b)+floatRoundoff(z.center,{z.range,0,0});
    const double minY=z.contactMinY-rounding,maxY=z.contactMaxY+rounding;
    const double dy=b.y-a.y;
    if(dy==0) {
        if(a.y<minY || a.y>maxY) return false;
    } else {
        const double u=(minY-a.y)/dy,v=(maxY-a.y)/dy;
        low=std::max(low,std::min(u,v)); high=std::min(high,std::max(u,v));
        if(low>high) return false;
    }
    const double dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz;
    const double t=length>0?std::clamp(((z.center.x-a.x)*dx+(z.center.z-a.z)*dz)/length,low,high):low;
    const double x=a.x+t*dx-z.center.x,zDistance=a.z+t*dz-z.center.z;
    const double distanceSquared=x*x+zDistance*zDistance,radius=z.range+rounding;
    return !std::isfinite(distanceSquared) || distanceSquared<=radius*radius;
}
bool clear(Point a,Point b,const std::vector<Entity>& red) {
    if(!finite(a) || !finite(b)) return false;
    for(const auto& e:red) if(!e.inactive && (!validZone(e) || touches(a,b,e.zone))) return false;
    return true;
}
bool walkable(const Triangle& t) {
    const auto a=t.points[0],b=t.points[1],c=t.points[2];
    const double determinant=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    return t.valid && (t.collision==0 || t.collision==2) && finite(a) && finite(b) && finite(c) &&
        std::isfinite(determinant) && std::abs(determinant)>=epsilon;
}
bool floorPoint(const Triangle& t,Point p,Point& out) {
    const auto a=t.points[0],b=t.points[1],c=t.points[2];
    if(!finite(a) || !finite(b) || !finite(c) || !finite(p)) return false;
    const double det=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    if(!std::isfinite(det) || std::abs(det)<epsilon) return false;
    const double u=((b.z-c.z)*(p.x-c.x)+(c.x-b.x)*(p.z-c.z))/det;
    const double v=((c.z-a.z)*(p.x-c.x)+(a.x-c.x)*(p.z-c.z))/det;
    // No expanded triangle and no off-mesh projection across a gap.
    if(u<0 || v<0 || u+v>1) return false;
    out={p.x,u*a.y+v*b.y+(1-u-v)*c.y,p.z};
    return finite(out);
}
Point mix(Point a,Point b,double t) { return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t}; }
Point closest(const Triangle& t,Point p) {
    Point out;
    if(floorPoint(t,p,out)) return out;
    double best=std::numeric_limits<double>::infinity(); out=t.points[0];
    for(unsigned i=0;i<3;++i) {
        const auto a=t.points[i],b=t.points[(i+1)%3];
        const double dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz;
        const auto q=mix(a,b,length>0?std::clamp(((p.x-a.x)*dx+(p.z-a.z)*dz)/length,0.0,1.0):0);
        const double d=distanceXZ(p,q);
        if(d<best) { best=d; out=q; }
    }
    return out;
}
bool same(Point a,Point b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
struct Node { Point p; int triangle; bool goal; std::vector<size_t> bridges; };
}
bool butterflyInTarget(const Entity& blue,Point p) {
    if(!validZone(blue) || !finite(p)) return false;
    const auto& z=blue.zone;
    return p.y>=z.contactMinY && p.y<=z.contactMaxY && distanceXZ(p,z.center)<z.range;
}
bool butterflyRouteClear(const std::vector<Point>& route,const std::vector<Entity>& red) {
    if(route.empty() || !clear(route.front(),route.front(),red)) return false;
    for(size_t i=1;i<route.size();++i) if(!clear(route[i-1],route[i],red)) return false;
    return true;
}
std::vector<Point> butterflyRoute(const Mesh& mesh,Point origin,const Entity& blue,const std::vector<Entity>& red) {
    if(!validZone(blue) || !clear(origin,origin,red) || mesh.openPlane || mesh.triangles.empty()) return {};
    const int start=findTriangle(mesh,origin);
    Point floor;
    if(start<0 || !walkable(mesh.triangles[start]) || !floorPoint(mesh.triangles[start],origin,floor) ||
        std::abs(floor.y-origin.y)>floatRoundoff(floor,origin)) return {};
    std::vector<Node> nodes;
    std::vector<std::vector<size_t>> groups(mesh.triangles.size());
    const auto add=[&](int triangle,Point p) -> size_t {
        if(!clear(p,p,red)) return std::numeric_limits<size_t>::max();
        const size_t n=nodes.size(); nodes.push_back({p,triangle,butterflyInTarget(blue,p),{}});
        groups[triangle].push_back(n); return n;
    };
    const auto source=add(start,origin);
    // Every edge remains inside one convex floor triangle. Native neighbours
    // alone connect triangles, through identically positioned shared portals.
    for(size_t i=0;i<mesh.triangles.size();++i) {
        const auto& t=mesh.triangles[i]; if(!walkable(t)) continue;
        Point p;
        if(floorPoint(t,t.center,p)) add(static_cast<int>(i),p);
        add(static_cast<int>(i),closest(t,blue.zone.center));
        // Interior blue anchors allow an arrival away from a red overlap.
        for(unsigned n=0;n<ringCount;++n) {
            const double a=turn*n/ringCount,r=blue.zone.range*.5;
            const Point q{blue.zone.center.x+r*std::cos(a),blue.zone.center.y,blue.zone.center.z+r*std::sin(a)};
            if(floorPoint(t,q,p)) add(static_cast<int>(i),p);
        }
        for(const auto& e:red) if(!e.inactive) {
            // Circumscribed ring supplies detour candidates; every resulting
            // finite chord is still tested, including against other hazards.
            const double radius=(e.zone.range+.25)/std::cos(turn/(2*ringCount));
            for(unsigned n=0;n<ringCount;++n) {
                const double a=turn*n/ringCount;
                const Point q{e.zone.center.x+radius*std::cos(a),e.zone.center.y,e.zone.center.z+radius*std::sin(a)};
                if(floorPoint(t,q,p)) add(static_cast<int>(i),p);
            }
        }
    }
    const size_t missing=std::numeric_limits<size_t>::max();
    for(size_t i=0;i<mesh.triangles.size();++i) {
        const auto& a=mesh.triangles[i]; if(!walkable(a)) continue;
        for(int next:a.neighbours) {
            if(next<0 || static_cast<size_t>(next)>=mesh.triangles.size() || static_cast<size_t>(next)==i) continue;
            const auto& b=mesh.triangles[next]; if(!walkable(b)) continue;
            const bool reverse=std::find(std::begin(b.neighbours),std::end(b.neighbours),static_cast<int>(i))!=std::end(b.neighbours);
            if(reverse && static_cast<size_t>(next)<i) continue;
            std::vector<Point> shared;
            for(unsigned j=0;j<3;++j) for(unsigned k=0;k<3;++k)
                if(a.vertices[j]==b.vertices[k] && same(a.points[j],b.points[k])) shared.push_back(a.points[j]);
            if(shared.size()!=2 || same(shared[0],shared[1])) continue;
            for(double fraction:{.125,.25,.5,.75,.875}) {
                const auto p=mix(shared[0],shared[1],fraction);
                const auto from=add(static_cast<int>(i),p),to=add(next,p);
                if(from!=missing && to!=missing) {
                    nodes[from].bridges.push_back(to);
                    if(reverse) nodes[to].bridges.push_back(from);
                }
            }
        }
    }
    std::vector<double> cost(nodes.size(),std::numeric_limits<double>::infinity());
    std::vector<size_t> previous(nodes.size(),missing);
    using Entry=std::pair<double,size_t>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
    cost[source]=0; pending.push({0,source});
    while(!pending.empty()) {
        const auto [value,at]=pending.top(); pending.pop(); if(value!=cost[at]) continue;
        if(nodes[at].goal) {
            std::vector<Point> route;
            for(auto n=at;n!=missing;n=previous[n]) route.push_back(nodes[n].p);
            std::reverse(route.begin(),route.end());
            if(route.size()==1) route.push_back(route.front());
            return butterflyRouteClear(route,red) && butterflyInTarget(blue,route.back())?route:std::vector<Point>{};
        }
        const auto visit=[&](size_t next) {
            if(next==at || !clear(nodes[at].p,nodes[next].p,red)) return;
            const double candidate=value+distanceXZ(nodes[at].p,nodes[next].p);
            if(candidate<cost[next]) { cost[next]=candidate; previous[next]=at; pending.push({candidate,next}); }
        };
        for(auto next:groups[nodes[at].triangle]) visit(next);
        for(auto next:nodes[at].bridges) visit(next);
    }
    return {};
}
}
