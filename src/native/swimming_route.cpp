// Native swim physics caps Y at the interpolated ChMap floor minus body clearance.
// Reach a connected patch deep enough for the real cdsp0000 Y>250 transition.
#include "swimming_route.hpp"
#include <algorithm>
#include <cmath>
namespace ffx {
std::vector<Point> swimmingRoute(const Mesh& mesh,Point origin,const Entity& target) {
    if(target.inactive || !target.swimReady) return {};
    if(!mesh.swimmingFloor || mesh.floorY.size()!=mesh.triangles.size()) {
        // Native collision-free swimming has no regional floor constraint.
        return mesh.openPlane?std::vector<Point>{origin,{origin.x,251,origin.z}}:std::vector<Point>{};
    }
    struct Candidate { Point point; double distance; };
    std::vector<Candidate> candidates;
    const double depth=252+std::max(0.0,target.swimClearance);
    for(unsigned i=0;i<mesh.triangles.size();++i) {
        const auto& triangle=mesh.triangles[i];
        if(!triangle.valid || (triangle.collision!=0 && triangle.collision!=2)) continue;
        std::vector<Point> polygon;
        for(unsigned j=0;j<3;++j) {
            auto a=triangle.points[j],b=triangle.points[(j+1)%3];
            a.y=mesh.floorY[i][j];b.y=mesh.floorY[i][(j+1)%3];
            if(!std::isfinite(a.y) || !std::isfinite(b.y)) {polygon.clear();break;}
            if(a.y>=depth) polygon.push_back(a);
            if((a.y>=depth)!=(b.y>=depth)) {
                const double t=(depth-a.y)/(b.y-a.y);
                polygon.push_back({a.x+t*(b.x-a.x),depth,a.z+t*(b.z-a.z)});
            }
        }
        if(polygon.size()<3) continue;
        Point center{}; for(const auto& p:polygon) {center.x+=p.x/polygon.size();center.z+=p.z/polygon.size();}
        Point nearest=center;double best=distanceXZ(origin,center); bool inside=true;
        double sign=0;
        for(size_t j=0;j<polygon.size();++j) {
            const auto a=polygon[j],b=polygon[(j+1)%polygon.size()];
            const double dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz;
            const double cross=dx*(origin.z-a.z)-dz*(origin.x-a.x);
            if(std::abs(cross)>1e-8) {if(sign && sign*cross<0) inside=false;else sign=cross;}
            const double t=length>1e-8?std::clamp(((origin.x-a.x)*dx+(origin.z-a.z)*dz)/length,0.0,1.0):0;
            Point p{a.x+t*dx,origin.y,a.z+t*dz}; const double d=distanceXZ(origin,p);
            if(d<best) {best=d;nearest=p;}
        }
        if(inside) nearest=origin;
        else { // Stop just inside the sufficient-depth polygon, not on a rounding edge.
            const double d=distanceXZ(nearest,center),t=d>0?std::min(1.0,.25/d):0;
            nearest.x+=(center.x-nearest.x)*t;nearest.z+=(center.z-nearest.z)*t;
        }
        nearest.y=origin.y;candidates.push_back({nearest,distanceXZ(origin,nearest)});
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){return a.distance<b.distance;});
    auto planar=mesh;planar.planeY=origin.y;
    // Preserve the native current triangle through vertical motion. If X/Z
    // moved outside it, use83DE10's least original floor Y below the actor.
    planar.player=origin;
    double nearestFloor=1e30;int current=-1;
    for(unsigned i=0;i<mesh.triangles.size();++i) {
        const auto& t=mesh.triangles[i];if(!t.valid || (t.collision!=0 && t.collision!=2)) continue;
        const auto a=t.points[0],b=t.points[1],c=t.points[2];
        const double det=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);if(std::abs(det)<1e-8) continue;
        const double u=((b.z-c.z)*(origin.x-c.x)+(c.x-b.x)*(origin.z-c.z))/det;
        const double v=((c.z-a.z)*(origin.x-c.x)+(a.x-c.x)*(origin.z-c.z))/det;
        if(u<0 || v<0 || u+v>1) continue;
        const double floor=u*mesh.floorY[i][0]+v*mesh.floorY[i][1]+(1-u-v)*mesh.floorY[i][2];
        if(static_cast<int>(i)==mesh.playerTriangle && floor>=origin.y-.01) {current=i;break;}
        if(floor>=origin.y && floor<nearestFloor) {current=i;nearestFloor=floor;}
    }
    planar.playerTriangle=current;
    // Collision may have been cached at Surface height before the player dived.
    for(auto& t:planar.triangles) {for(auto& p:t.points) p.y=origin.y;t.center.y=origin.y;}
    for(const auto& candidate:candidates) {
        auto route=routePoints(planar,origin,candidate.point);
        if(route.empty()) continue;
        const auto end=route.back();route.push_back({end.x,251,end.z});return route;
    }
    return {};
}
}
