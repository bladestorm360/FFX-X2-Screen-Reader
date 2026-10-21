#include "navigation.hpp"
#include "navigation_cancel.hpp"
#include "cross_water_data.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
namespace ffx {
namespace {
std::vector<Point> crossWaterRoute(const Mesh&,Point,const InteractionZone&,bool);
Point onSegment(Point a,Point b,Point p) {
    const double x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
    const double t=length>1e-8?std::clamp(((p.x-a.x)*x+(p.z-a.z)*z)/length,0.0,1.0):0;
    return {a.x+t*x,a.y+t*(b.y-a.y),a.z+t*z};
}
Point onShape(const InteractionShape& zone,Point p) {
    if(zone.shape==2) return onSegment(zone.center,zone.end,p);
    if(zone.shape!=3) return zone.center;
    const double c=std::cos(zone.angle),s=std::sin(zone.angle);
    const double x=p.x-zone.center.x,z=p.z-zone.center.z;
    const double margin=zone.crossing?.25:0;
    const double width=std::max(0.0,zone.width-margin),depth=std::max(0.0,zone.depth-margin);
    const double u=std::clamp(x*c-z*s,-width,width),v=std::clamp(x*s+z*c,-depth,depth);
    return {zone.center.x+u*c+v*s,zone.center.y,zone.center.z-u*s+v*c};
}
bool lineEndpoint(const InteractionZone& zone,Point to,bool horizontal=false) {
    const auto closest=onShape(zone,to);
    return distanceXZ(closest,to)<=1.01 &&
        (horizontal || zone.height<=0 || std::abs(to.y-closest.y)<zone.height);
}
bool routeCrossesLine(const InteractionZone& zone,const std::vector<Point>& route,bool horizontal=false) {
    const double x=zone.end.x-zone.center.x,z=zone.end.z-zone.center.z,length=x*x+z*z;
    if(length<1e-8) return false;
    for(size_t i=1;i<route.size();++i) {
        const auto from=route[i-1],to=route[i];
        const double a=x*(from.z-zone.center.z)-z*(from.x-zone.center.x);
        const double b=x*(to.z-zone.center.z)-z*(to.x-zone.center.x);
        if(!(a*b<0 || (std::abs(a)<1e-8 && std::abs(b)>.1))) continue;
        const double t=a/(a-b);
        const Point crossing{from.x+t*(to.x-from.x),from.y+t*(to.y-from.y),from.z+t*(to.z-from.z)};
        const double along=((crossing.x-zone.center.x)*x+(crossing.z-zone.center.z)*z)/length;
        if(along<0 || along>1) continue;
        const double height=zone.center.y+along*(zone.end.y-zone.center.y);
        if(horizontal || zone.height<=0 || std::abs(crossing.y-height)<zone.height) return true;
    }
    return false;
}
Point onFloor(const Triangle& t,Point p) {
    const auto a=t.points[0],b=t.points[1],c=t.points[2];
    const double det=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    if(std::abs(det)>1e-8) {
        const double u=((b.z-c.z)*(p.x-c.x)+(c.x-b.x)*(p.z-c.z))/det;
        const double v=((c.z-a.z)*(p.x-c.x)+(a.x-c.x)*(p.z-c.z))/det;
        if(u>=0 && v>=0 && u+v<=1) return {p.x,u*a.y+v*b.y+(1-u-v)*c.y,p.z};
    }
    Point best=t.points[0]; double d=std::numeric_limits<double>::infinity();
    for(unsigned i=0;i<3;++i) {
        const auto candidate=onSegment(t.points[i],t.points[(i+1)%3],p);
        const auto value=distanceXZ(candidate,p); if(value<d) { d=value; best=candidate; }
    }
    return best;
}
bool insideShape(const InteractionShape& zone,Point p,bool target=true,bool horizontal=false) {
    if(!zone.valid) return false;
    const auto closest=onShape(zone,p);
    if(!horizontal && zone.height>0 && std::abs(p.y-closest.y)>=zone.height) return false;
    if(zone.shape==1 && (zone.height==0 || zone.range<=0)) return false;
    const double planar=distanceXZ(closest,p);
    const double distance=zone.shape==1 && zone.height<0 && !horizontal?std::hypot(planar,p.y-closest.y):planar;
    return zone.range>0?distance<std::max(0.0,zone.range-(target?.1:zone.shape==1?0:-.00001)):distance<.001;
}
double talkScore(const InteractionShape& zone,Point p,bool horizontal=false) {
    // 866920 scores circles by distance squared. 866C60 and 8665B0
    // score rectangular/line controls by their total talk reach squared.
    if(zone.shape!=1) return zone.range*zone.range;
    const double x=p.x-zone.center.x,z=p.z-zone.center.z,y=zone.height<0 && !horizontal?p.y-zone.center.y:0;
    return x*x+y*y+z*z;
}
std::vector<Point> interactionAnchors(const InteractionZone& zone,Point player) {
    if(zone.contactRadius>0) {
        const double radius=zone.contactRadius+.25;
        std::vector<Point> anchors;
        const std::vector<Point> sides=distanceXZ(zone.approach,{})>.1?std::vector<Point>{zone.approach}:
            std::vector<Point>{{1,0,0},{-1,0,0},{0,0,1},{0,0,-1}};
        for(const auto side:sides) anchors.push_back({zone.center.x+side.x*radius,zone.center.y,zone.center.z+side.z*radius});
        return anchors;
    }
    const auto closest=onShape(zone,player);
    const double d=distanceXZ(player,closest),radius=std::max(0.0,zone.range-.25);
    Point desired=closest;
    if(d>1e-8 && radius>0) { desired.x+=(player.x-closest.x)*std::min(1.0,radius/d); desired.z+=(player.z-closest.z)*std::min(1.0,radius/d); }
    if(zone.crossing && zone.shape==2 && d>1e-8) {
        desired.x+=(closest.x-player.x)/d; desired.z+=(closest.z-player.z)/d;
    } else if(zone.crossing && zone.shape==2) {
        const double x=zone.end.x-zone.center.x,z=zone.end.z-zone.center.z,length=std::hypot(x,z);
        if(length>1e-8) { desired.x+=z/length; desired.z-=x/length; }
    }
    std::vector<Point> anchors{desired,closest,zone.center,zone.end};
    if(zone.shape==3) {
        const double c=std::cos(zone.angle),s=std::sin(zone.angle);
        const double margin=zone.crossing?.25:0;
        const double width=std::max(0.0,zone.width-margin),depth=std::max(0.0,zone.depth-margin);
        for(double u:{-width,0.0,width}) for(double v:{-depth,0.0,depth})
            anchors.push_back({zone.center.x+u*c+v*s,zone.center.y,zone.center.z-u*s+v*c});
    }
    if(zone.crossing && zone.shape==2) {
        const double x=zone.end.x-zone.center.x,z=zone.end.z-zone.center.z,length=std::hypot(x,z);
        if(length>1e-8) for(double fraction:{0.0,.5,1.0}) for(double sign:{-1.0,1.0}) {
            // Use the line normal even when the player is far along its
            // extension. A radial approach to an endpoint may never cross.
            // A winding route can approach from either side independently of
            // the player's original side. Validate the final route crossing.
            anchors.push_back({zone.center.x+fraction*x+sign*z/length,zone.center.y,
                zone.center.z+fraction*z-sign*x/length});
        }
    }
    if(zone.talk && !zone.competing.empty()) {
        // Approach overlapping controls from a side where the native
        // dispatcher selects this worker, including narrow score boundaries.
        double reach=radius;
        for(const auto& other:zone.competing) if(other.shape!=1) reach=std::min(reach,std::max(0.0,other.range-.5));
        for(double r:{reach,radius*.75,radius*.5,radius*.25}) for(unsigned i=0;i<32;++i) {
            const double a=i*6.283185307179586/32;
            anchors.push_back({zone.center.x+r*std::cos(a),zone.center.y,zone.center.z+r*std::sin(a)});
        }
    }
    return anchors;
}

bool clearsPedestal(const std::vector<Point>& points,const InteractionZone& zone) {
    if(points.empty()) return false;
    for(size_t i=1;i<points.size();++i) {
        const auto a=points[i-1],b=points[i];
        const auto closest=onSegment(a,b,zone.center);
        if(distanceXZ(closest,zone.center)>=zone.contactRadius+.05) continue;
        // Permit an outward first segment when already touching the actor.
        const double x=a.x-zone.center.x,z=a.z-zone.center.z;
        if(i==1 && x*(b.x-a.x)+z*(b.z-a.z)>=0 &&
            distanceXZ(b,zone.center)>=distanceXZ(a,zone.center)) continue;
        return false;
    }
    return true;
}
std::vector<Point> contactRoute(const Mesh& mesh,Point player,Point goal,const InteractionZone& zone) {
    const auto direct=routePoints(mesh,player,goal);
    if(clearsPedestal(direct,zone)) return direct;
    // The floor mesh omits actor collisions. Try both arcs around the live
    // target cylinder, validating each leg against the actual connected floor.
    constexpr unsigned count=16;
    constexpr double turn=6.283185307179586,countAngle=turn/count;
    const double radius=(zone.contactRadius+1.5)/std::cos(countAngle*.5);
    const auto index=[&](Point p) {
        return (static_cast<int>(std::round(std::atan2(p.z-zone.center.z,p.x-zone.center.x)/countAngle))+count)%count;
    };
    const int first=index(player),last=index(goal);
    std::vector<Point> best; double bestLength=std::numeric_limits<double>::infinity();
    for(int side:{-1,1}) {
        std::vector<Point> candidate{player}; bool valid=true;
        int at=first;
        for(unsigned n=0;n<count;++n) {
            nav::checkComputation();
            const double angle=at*countAngle;
            const Point ring{zone.center.x+radius*std::cos(angle),zone.center.y,zone.center.z+radius*std::sin(angle)};
            const auto leg=routePoints(mesh,candidate.back(),ring);
            if(!clearsPedestal(leg,zone)) { valid=false; break; }
            candidate.insert(candidate.end(),leg.begin()+1,leg.end());
            if(at==last) break;
            at=(at+side+count)%count;
        }
        if(!valid) continue;
        const auto final=routePoints(mesh,candidate.back(),goal);
        if(!clearsPedestal(final,zone)) continue;
        candidate.insert(candidate.end(),final.begin()+1,final.end());
        if(!clearsPedestal(candidate,zone)) continue;
        double length=0; for(size_t i=1;i<candidate.size();++i) length+=distanceXZ(candidate[i-1],candidate[i]);
        if(length<bestLength) { bestLength=length; best=std::move(candidate); }
    }
    return best;
}
std::vector<Point> avoidingRoute(const Mesh& mesh,Point player,Point goal,const InteractionZone& zone,int endTriangle=-1) {
    if(zone.obstacleRadius>0) {
        auto obstacle=zone; obstacle.center=zone.obstacle; obstacle.contactRadius=zone.obstacleRadius;
        return contactRoute(mesh,player,goal,obstacle);
    }
    return zone.contactRadius>0?contactRoute(mesh,player,goal,zone):
        endTriangle>=0?routePoints(mesh,player,goal,endTriangle):routePoints(mesh,player,goal);
}

}
template<class Zone> static bool interactionEndpoint(const Zone& zone,Point p,bool horizontal) {
    if(zone.contactRadius>0) {
        if(!zone.valid || (!horizontal && (p.y<zone.contactMinY || p.y>zone.contactMaxY))) return false;
        const double x=p.x-zone.center.x,z=p.z-zone.center.z;
        const double distance=std::hypot(x,z);
        // Stage outside collision even when a previous manual touch left its
        // contact flag set. The assisted inward walk must create a fresh edge.
        if(distance<zone.contactRadius+.1 || distance>zone.contactRadius+.8) return false;
        if(distanceXZ(zone.approach,{})>.1) {
            const double along=x*zone.approach.x+z*zone.approach.z;
            return along>0 && std::abs(x*zone.approach.z-z*zone.approach.x)<=.65;
        }
        return std::min(std::abs(x),std::abs(z))<=.65;
    }
    if(!insideShape(zone,p,true,horizontal)) return false;
    if(!zone.talk) return true;
    if(zone.viewAngle<=0) return false;
    const double facing=std::atan2(zone.center.z-p.z,zone.center.x-p.x);
    const auto score=talkScore(zone,p,horizontal);
    for(const auto& other:zone.competing) {
        if(!insideShape(other,p,false,horizontal)) continue;
        const double angle=std::atan2(other.center.z-p.z,other.center.x-p.x);
        if(std::abs(std::remainder(angle-facing,6.283185307179586))>=other.viewAngle) continue;
        const auto competitor=talkScore(other,p,horizontal);
        if(competitor<=score+.01) return false;
    }
    return true;
}
bool inInteractionZone(const InteractionZone& zone,Point p) { return interactionEndpoint(zone,p,false); }
bool inInteractionZone(const NativeInteractionZone& zone,Point p) { return zone.competing.count<=zone.competing.rows.size() && interactionEndpoint(zone,p,false); }
std::vector<Point> interactionRoute(const Mesh& mesh,Point player,const InteractionZone& zone,bool horizontalApproach) {
    if(!zone.valid || !mesh.events.ready) return {};
    if(mesh.crossWater) return crossWaterRoute(mesh,player,zone,horizontalApproach);
    const bool horizontal=mesh.fixedHeight && horizontalApproach;
    // Swimming controls X/Z on the native component; buoyancy owns Y. These
    // approach routes do not assert native activation or change player depth.
    if(mesh.openPlane) {
        if(interactionEndpoint(zone,player,horizontal) && !(zone.crossing && zone.shape==2)) return {player,player};
        std::vector<Point> goals;
        for(auto candidate:interactionAnchors(zone,player)) {
            candidate.y=mesh.fixedHeight?player.y:mesh.planeY;
            if(zone.crossing && zone.shape==2?!lineEndpoint(zone,candidate,horizontal):!interactionEndpoint(zone,candidate,horizontal)) continue;
            goals.push_back(candidate);
        }
        std::stable_sort(goals.begin(),goals.end(),[&](Point a,Point b) { return distanceXZ(player,a)<distanceXZ(player,b); });
        for(auto goal:goals) {
            nav::checkComputation();
            const auto route=avoidingRoute(mesh,player,goal,zone);
            if(!route.empty() && (!(zone.crossing && zone.shape==2) || routeCrossesLine(zone,route,horizontal))) return route;
        }
        return {};
    }
    const int start=findTriangle(mesh,player); if(start<0) return {};
    if(interactionEndpoint(zone,player,horizontal) && !(zone.crossing && zone.shape==2)) return {player,player};
    std::vector<double> cost(mesh.triangles.size(),std::numeric_limits<double>::infinity()); cost[start]=0;
    using Entry=std::pair<double,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending; pending.push({0,start});
    const auto anchors=interactionAnchors(zone,player);
    struct Goal { double cost; Point position; int triangle; };
    std::vector<Goal> goals;
    while(!pending.empty()) {
        nav::checkComputation();
        const auto [value,at]=pending.top(); pending.pop(); if(value!=cost[at]) continue;
        const auto& triangle=mesh.triangles[at];
        for(const auto anchor:anchors) {
            auto p=onFloor(triangle,anchor); if(mesh.fixedHeight) p.y=player.y;
            if(zone.crossing && zone.shape==2?!lineEndpoint(zone,p,horizontal):!interactionEndpoint(zone,p,horizontal)) continue;
            const double candidate=value+(at==start?distanceXZ(player,p):distanceXZ(triangle.center,p));
            if(std::isfinite(candidate)) {
                goals.push_back({candidate,p,at});
            }
        }
        for(int next:triangle.neighbours) {
            if(next<0 || static_cast<size_t>(next)>=mesh.triangles.size()) continue;
            const auto& t=mesh.triangles[next]; if(!t.valid || (t.collision!=0 && t.collision!=2)) continue;
            Point portal; if(!floorPortal(triangle,t,portal)) continue;
            if(!routeEdgeAllowed(mesh,at,next,portal,at==start?&player:nullptr)) continue;
            const double candidate=value+distanceXZ(triangle.center,t.center);
            if(candidate<cost[next]) { cost[next]=candidate; pending.push({candidate,next}); }
        }
    }
    std::stable_sort(goals.begin(),goals.end(),[](const Goal& a,const Goal& b) { nav::checkComputation();return a.cost<b.cost; });
    // Keep the connected floor chosen above. A fresh global lookup can pick
    // a disconnected overlapping polygon at precisely the same endpoint.
    for(const auto& goal:goals) {
        nav::checkComputation();
        const auto route=avoidingRoute(mesh,player,goal.position,zone,goal.triangle);
        if(!route.empty() && (zone.crossing && zone.shape==2?routeCrossesLine(zone,route,horizontal):interactionEndpoint(zone,route.back(),horizontal))) return route;
    }
    return {};
}
namespace {
bool waterContains(const nav::WaterCrossing& crossing,int triangle) {
    return triangle>=0 && std::binary_search(crossing.triangles,crossing.triangles+crossing.count,static_cast<uint16_t>(triangle));
}
bool sameExit(const nav::WaterCrossing& crossing,const InteractionZone& zone) {
    const auto same=[](Point a,Point b) {return distanceXZ(a,b)<.02 && std::abs(a.y-b.y)<.02;};
    return zone.shape==2 && ((same(zone.center,crossing.exitA) && same(zone.end,crossing.exitB)) ||
        (same(zone.end,crossing.exitA) && same(zone.center,crossing.exitB)));
}
bool crossesShore(const nav::WaterCrossing& crossing,const std::vector<Point>& route,bool entering) {
    const auto a=crossing.a,b=crossing.b;
    const double x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
    if(length<.001) return false;
    const auto side=[&](Point p) {return x*(p.z-a.z)-z*(p.x-a.x);};
    const double waterSide=side(crossing.water);
    for(size_t i=1;i<route.size();++i) {
        const double first=side(route[i-1]),last=side(route[i]);
        if(first*last>=0 || (entering?last:first)*waterSide<=0) continue;
        const double t=first/(first-last);
        const Point p{route[i-1].x+t*(route[i].x-route[i-1].x),0,route[i-1].z+t*(route[i].z-route[i-1].z)};
        const double along=((p.x-a.x)*x+(p.z-a.z)*z)/length;
        if(along>=0 && along<=1) return true;
    }
    return false;
}
Mesh waterView(const Mesh& land,const nav::WaterCrossing& crossing,Point origin,int triangle) {
    auto water=land;water.fixedHeight=true;water.swimmingFloor=true;water.planeY=origin.y;
    water.player=origin;water.playerTriangle=triangle;
    for(unsigned i=0;i<water.triangles.size();++i) {
        nav::checkComputation();auto& t=water.triangles[i];t.valid=t.valid && waterContains(crossing,static_cast<int>(i));
        for(auto& p:t.points) p.y=origin.y;t.center.y=origin.y;
    }
    return water;
}
std::vector<Point> crossWaterRoute(const Mesh& mesh,Point player,const InteractionZone& zone,bool horizontalApproach) {
    // Baked identities/regions select only original shoreline scripts. Every
    // leg still searches current native adjacency/collision on owned copies.
    Mesh land;land.player=mesh.player;land.playerTriangle=mesh.playerTriangle;
    if(mesh.fixedHeight) {
        if(mesh.terrain.size()!=mesh.triangles.size()) return {};
        land.triangles=mesh.terrain;
    } else land.triangles=mesh.triangles;
    int source=-1,destination=-1;
    const auto& crossings=mesh.crossWater->crossings;
    for(unsigned i=0;i<crossings.size();++i) {
        if(mesh.fixedHeight && waterContains(crossings[i],mesh.playerTriangle)) source=static_cast<int>(i);
        if(sameExit(crossings[i],zone)) destination=static_cast<int>(i);
    }
    if(mesh.fixedHeight && source<0) {
        auto local=mesh;local.crossWater=nullptr;local.terrain.clear();
        return interactionRoute(local,player,zone,horizontalApproach);
    }
    if(source>=0 && (source==destination || destination<0)) {
        auto water=waterView(land,crossings[source],player,mesh.playerTriangle);
        auto local=interactionRoute(water,player,zone,horizontalApproach || source==destination);
        if(!local.empty()) return local;
    }
    if(source<0 && destination<0) return interactionRoute(land,player,zone,horizontalApproach);
    std::vector<Point> route{player};Point onLand=player;
    if(source>=0) {
        if(!(mesh.waterCrossings&(1u<<source))) return {};
        const auto& crossing=crossings[source];
        auto water=waterView(land,crossing,player,mesh.playerTriangle);
        auto approach=crossing.water;approach.y=player.y;
        auto leg=routePoints(water,player,approach,crossing.waterTriangle);
        auto across=routePoints(water,approach,crossing.land,crossing.landTriangle);
        if(leg.empty() || across.empty() || !crossesShore(crossing,across,false)) return {};
        across.back().y=crossing.land.y;leg.insert(leg.end(),across.begin()+1,across.end());
        route=std::move(leg);onLand=crossing.land;
        land.player=onLand;land.playerTriangle=crossing.landTriangle;
    }
    if(destination>=0) {
        if(!(mesh.waterCrossings&(1u<<destination))) return {};
        const auto& crossing=crossings[destination];
        auto landing=crossing.water;
        if(crossing.waterTriangle<0 || static_cast<size_t>(crossing.waterTriangle)>=land.triangles.size()) return {};
        landing=onFloor(land.triangles[crossing.waterTriangle],landing);
        auto leg=routePoints(land,onLand,crossing.land,crossing.landTriangle);
        auto across=routePoints(land,crossing.land,landing,crossing.waterTriangle);
        if(leg.empty() || across.empty() || !crossesShore(crossing,across,true)) return {};
        across.back()=crossing.water;leg.insert(leg.end(),across.begin()+1,across.end());
        route.insert(route.end(),leg.begin()+1,leg.end());
        auto water=waterView(land,crossing,crossing.water,crossing.waterTriangle);
        leg=interactionRoute(water,crossing.water,zone,true);
        if(leg.empty()) return {};
        route.insert(route.end(),leg.begin()+1,leg.end());
    } else {
        const auto leg=interactionRoute(land,onLand,zone,horizontalApproach);
        if(leg.empty()) return {};
        route.insert(route.end(),leg.begin()+1,leg.end());
    }
    // Never smooth across a shoreline: its crossing and change of movement
    // mode must remain in the route. Native scripts alone perform that change.
    return route;
}
}
bool destinationUnreachable(const Mesh& mesh,Point player,const Entity& target) {
    if(!mesh.events.ready) return false; // Pending event ownership is not proof of a hidden target.
    if(target.swimDepth) return target.inactive;
    const int start=findTriangle(mesh,player);
    if((mesh.triangles.empty() && !mesh.openPlane) || start<0) return false;
    const auto route=target.zone.valid?interactionRoute(mesh,player,target.zone,target.swimApproach):routePoints(mesh,player,target.position);
    if(!route.empty()) return false;
    // A failed route is not proof of unavailable access. In particular, a
    // boundary crossing may have no mapped floor on its far side, and malformed
    // portals/unknown target geometry must not silently remove destinations.
    if(target.category==L"Exit") {
        if(!target.zone.valid || mesh.openPlane) return false;
        const auto gate=[](const Triangle& triangle) {
            return triangle.valid && triangle.collisionGroup>=48 && triangle.collisionGroup<=63 &&
                triangle.collision!=0 && triangle.collision!=2;
        };
        const auto& terrain=mesh.terrain.empty()?mesh.triangles:mesh.terrain;
        if(std::none_of(terrain.begin(),terrain.end(),[&](const Triangle& t) {nav::checkComputation();return gate(t);})) return false;
        auto ungated=mesh;
        for(auto& triangle:ungated.triangles) {nav::checkComputation();if(gate(triangle)) triangle.collision=0;}
        for(auto& triangle:ungated.terrain) {nav::checkComputation();if(gate(triangle)) triangle.collision=0;}
        // This counterfactual mesh is used only to prove a native collision
        // gate removed an otherwise valid path. It never supplies walking/audio.
        return findTriangle(ungated,player)==start && !interactionRoute(ungated,player,target.zone,target.swimApproach).empty();
    }
    if(mesh.openPlane) return false;
    if(!target.zone.valid) {
        const int end=findTriangle(mesh,target.position);
        return end>=0 && path(mesh,start,end).empty();
    }
    // For other entities, prove an actual interaction endpoint on a separate
    // or blocked floor. If no endpoint can be established, retain the target.
    const auto anchors=interactionAnchors(target.zone,player);
    std::vector<bool> connected(mesh.triangles.size(),false); connected[start]=true;
    std::vector<int> pending{start};
    while(!pending.empty()) {
        nav::checkComputation();
        const auto at=pending.back(); pending.pop_back();
        for(int next:mesh.triangles[at].neighbours) if(next>=0 && static_cast<size_t>(next)<connected.size() && !connected[next]) {
            const auto& t=mesh.triangles[next];
            if(t.valid && (t.collision==0 || t.collision==2)) { connected[next]=true; pending.push_back(next); }
        }
    }
    bool endpoint=false;
    for(unsigned i=0;i<mesh.triangles.size();++i) {
        nav::checkComputation();
        const auto& triangle=mesh.triangles[i]; if(!triangle.valid) continue;
        for(const auto anchor:anchors) {
            auto candidate=onFloor(triangle,anchor); if(mesh.fixedHeight) candidate.y=player.y;
            if(target.zone.crossing && target.zone.shape==2?!lineEndpoint(target.zone,candidate,mesh.fixedHeight && target.swimApproach):!interactionEndpoint(target.zone,candidate,mesh.fixedHeight && target.swimApproach)) continue;
            endpoint=true;
            if(connected[i]) return false;
        }
    }
    return endpoint;
}
}
