#include "navigation.hpp"
#include "navigation_cancel.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace ffx {
namespace {
bool contains(const Triangle& t, Point p, double tolerance) {
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
bool walkable(const Triangle& t) { return t.valid && (t.collision == 0 || t.collision == 2); }
double floorHeight(const Triangle& t, Point p) {
    const auto& a = t.points[0]; const auto& b = t.points[1]; const auto& c = t.points[2];
    const double determinant = (b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    if (std::abs(determinant)<.0001) return t.center.y;
    const double u = ((b.z-c.z)*(p.x-c.x)+(c.x-b.x)*(p.z-c.z))/determinant;
    const double v = ((c.z-a.z)*(p.x-c.x)+(a.x-c.x)*(p.z-c.z))/determinant;
    return u*a.y+v*b.y+(1-u-v)*c.y;
}
bool lineOfSight(const Mesh& mesh, Point a, Point b) {
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
}
double distanceXZ(Point a, Point b) { return std::hypot(a.x - b.x, a.z - b.z); }
bool routeSegmentAllowed(const Mesh& mesh,Point from,Point to) {
    if(!mesh.events.ready || mesh.events.count>mesh.events.rows.size()) return false;
    for(unsigned i=0;i<mesh.events.count;++i) {
        const auto& e=mesh.events.rows[i];const double x=e.b.x-e.a.x,z=e.b.z-e.a.z,length=std::hypot(x,z);
        if(length<.001) return false;
        const auto side=[&](Point p) {return (x*(p.z-e.a.z)-z*(p.x-e.a.x))/length;};
        const double landing=side(e.landing),a=side(from)*(landing>=0?1:-1),b=side(to)*(landing>=0?1:-1);
        if(a<-.000001 || b>=-.000001 || a-b<.000001) continue;
        const double t=a/(a-b),px=from.x+t*(to.x-from.x),pz=from.z+t*(to.z-from.z);
        const double along=((px-e.a.x)*x+(pz-e.a.z)*z)/length;
        if(along<-.25 || along>length+.25) continue;
        const double edgeY=e.a.y+std::clamp(along/length,0.0,1.0)*(e.b.y-e.a.y);
        const double y=from.y+t*(to.y-from.y);
        if(e.height>0 && std::abs(y-edgeY)>=e.height) continue;
        return false;
    }
    return true;
}
bool routeEdgeAllowed(const Mesh& mesh,int from,int to,Point portal,const Point* origin) {
    return routeSegmentAllowed(mesh,origin?*origin:mesh.triangles[from].center,portal) &&
        routeSegmentAllowed(mesh,portal,mesh.triangles[to].center);
}
int findTriangle(const Mesh& mesh, Point p) {
    nav::checkComputation();
    if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return -1;
    if(mesh.openPlane) return 0;
    if(mesh.playerTriangle>=0 && static_cast<size_t>(mesh.playerTriangle)<mesh.triangles.size() &&
        distanceXZ(mesh.player,p)<.001 && (mesh.fixedHeight || std::abs(mesh.player.y-p.y)<.001) &&
        walkable(mesh.triangles[mesh.playerTriangle]) && contains(mesh.triangles[mesh.playerTriangle],p,.000001))
        return mesh.playerTriangle;
    double height = 1e18; int contained = -1;
    for (unsigned i = 0; i < mesh.triangles.size(); ++i) {
        nav::checkComputation();if(!walkable(mesh.triangles[i]) || !contains(mesh.triangles[i],p,.000001)) continue;
        const auto difference = std::abs(floorHeight(mesh.triangles[i],p)-p.y);
        if (difference<height) { height=difference; contained=static_cast<int>(i); }
    }
    if (contained>=0 && (mesh.fixedHeight || height<25)) return contained;
    double best = 33.39*33.39; int result = -1;
    for (unsigned i = 0; i < mesh.triangles.size(); ++i) {
        nav::checkComputation();if(!walkable(mesh.triangles[i])) continue;
        const auto& triangle=mesh.triangles[i];
        for(unsigned edge=0;edge<3;++edge) {
            const Point a=triangle.points[edge],b=triangle.points[(edge+1)%3];
            const double dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz;
            if(length<.0001) continue;
            const double fraction=std::clamp(((p.x-a.x)*dx+(p.z-a.z)*dz)/length,0.0,1.0);
            const Point closest{a.x+fraction*dx,a.y+fraction*(b.y-a.y),a.z+fraction*dz};
            if(!mesh.fixedHeight && std::abs(closest.y-p.y)>25) continue;
            const double d=distanceXZ(closest,p);
            if(d*d<best) { best=d*d; result=static_cast<int>(i); }
        }
    }
    return result;
}
bool nearWalkable(const Mesh& mesh, Point p, double threshold) {
    for (const auto& t : mesh.triangles) {
        nav::checkComputation();if(!walkable(t)) continue;
        if(!mesh.fixedHeight && std::abs(floorHeight(t,p)-p.y)>25) continue;
        double minX = t.points[0].x, maxX = minX, minZ = t.points[0].z, maxZ = minZ;
        for (const auto& v : t.points) { minX = std::min(minX, v.x); maxX = std::max(maxX, v.x); minZ = std::min(minZ, v.z); maxZ = std::max(maxZ, v.z); }
        if (p.x < minX - 3 || p.x > maxX + 3 || p.z < minZ - 3 || p.z > maxZ + 3) continue;
        if (contains(t, p, .02)) return true;
        for (unsigned i = 0; i < 3; ++i) {
            const auto a = t.points[i], b = t.points[(i + 1) % 3];
            const double dx = b.x - a.x, dz = b.z - a.z, length = dx * dx + dz * dz;
            if (length < .0001) continue;
            const double fraction = std::clamp(((p.x - a.x) * dx + (p.z - a.z) * dz) / length, 0.0, 1.0);
            const double ex = p.x - (a.x + dx * fraction), ez = p.z - (a.z + dz * fraction);
            if (ex * ex + ez * ez <= threshold) return true;
        }
    }
    return false;
}
bool floorPortal(const Triangle& a,const Triangle& b,Point& midpoint) {
    std::array<unsigned,3> shared{}; unsigned count=0;
    for(unsigned j=0;j<3;++j) for(unsigned k=0;k<3;++k) {
        const auto x=a.points[j],y=b.points[k];
        // The native neighbour table links physical edges. Distinct vertex
        // indices may hold identical coordinates at an exported seam.
        if(distanceXZ(x,y)<.000001 && std::abs(x.y-y.y)<.000001) {
            bool duplicate=false;
            for(unsigned i=0;i<count;++i) {
                const auto previous=a.points[shared[i]];
                if(distanceXZ(previous,x)<.000001 && std::abs(previous.y-x.y)<.000001) duplicate=true;
            }
            if(!duplicate) shared[count++]=j;
            break;
        }
    }
    // Native swimming adjacency includes vertical connector triangles. Their
    // projected duplicate vertices still form one physical edge, not three.
    if(count!=2) return false;
    const auto x=a.points[shared[0]],y=a.points[shared[1]];
    if(distanceXZ(x,y)<.000001) return false;
    midpoint={(x.x+y.x)/2,(x.y+y.y)/2,(x.z+y.z)/2}; return true;
}
std::vector<int> path(const Mesh& mesh, int start, int end,bool requirePortal,const Point* origin) {
    nav::checkComputation();
    const int count = static_cast<int>(mesh.triangles.size());
    if (!mesh.events.ready || start < 0 || end < 0 || start >= count || end >= count) return {};
    if(!walkable(mesh.triangles[start]) || !walkable(mesh.triangles[end])) return {};
    if (start == end) return {start};
    std::vector<bool> closed(count, false);
    std::vector<double> g(count, std::numeric_limits<double>::infinity()), f = g;
    std::vector<int> from(count, -1);
    using Entry=std::pair<double,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> open;
    g[start] = 0; f[start] = distanceXZ(mesh.triangles[start].center, mesh.triangles[end].center);
    open.push({f[start],start});
    while(!open.empty()) {
        nav::checkComputation();
        int current=open.top().second; const double score=open.top().first; open.pop();
        if(closed[current] || score>f[current]) continue;
        if (current == end) {
            std::vector<int> result{current};
            while (from[current] >= 0) { current = from[current]; result.push_back(current); }
            std::reverse(result.begin(), result.end()); return result;
        }
        closed[current] = true;
        for (int next : mesh.triangles[current].neighbours) {
            if (next < 0 || next >= count || closed[next] || !walkable(mesh.triangles[next])) continue;
            Point portal;
            if(requirePortal && !floorPortal(mesh.triangles[current],mesh.triangles[next],portal)) continue;
            if(mesh.events.count && (!floorPortal(mesh.triangles[current],mesh.triangles[next],portal) || !routeEdgeAllowed(mesh,current,next,portal,current==start?origin:nullptr))) continue;
            const double cost = g[current] + distanceXZ(mesh.triangles[current].center, mesh.triangles[next].center);
            if (cost < g[next]) {
                from[next] = current; g[next] = cost;
                f[next] = cost + distanceXZ(mesh.triangles[next].center, mesh.triangles[end].center);
                open.push({f[next],next});
            }
        }
    }
    return {};
}
std::vector<Point> smooth(const Mesh& mesh, const std::vector<Point>& points) {
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
std::vector<Point> routePoints(const Mesh& mesh,Point start,Point end) {
    return routePoints(mesh,start,end,findTriangle(mesh,end));
}
std::vector<Point> routePoints(const Mesh& mesh,Point start,Point end,int endTriangle) {
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
    return smooth(mesh,points);
}
unsigned nearbyWalls(const Mesh& mesh,Point position,Point forward,Point right,double range) {
    unsigned walls=0;
    const Point rays[]={forward,{-right.x,0,-right.z},right,{-forward.x,0,-forward.z}};
    for(const auto& triangle:mesh.triangles) {
        nav::checkComputation();if(!walkable(triangle)) continue;
        double minX=triangle.points[0].x,maxX=minX,minZ=triangle.points[0].z,maxZ=minZ;
        for(const auto& point:triangle.points) {
            minX=std::min(minX,point.x); maxX=std::max(maxX,point.x);
            minZ=std::min(minZ,point.z); maxZ=std::max(maxZ,point.z);
        }
        if(position.x<minX-range || position.x>maxX+range || position.z<minZ-range || position.z>maxZ+range) continue;
        for(unsigned edge=0;edge<3;++edge) {
            const int neighbour=triangle.neighbours[edge];
            if(neighbour>=0 && neighbour<static_cast<int>(mesh.triangles.size()) && walkable(mesh.triangles[neighbour])) continue;
            const auto a=triangle.points[edge],b=triangle.points[(edge+1)%3];
            const double dx=b.x-a.x,dz=b.z-a.z;
            for(unsigned direction=0;direction<4;++direction) {
                const auto ray=rays[direction];
                const double determinant=ray.x*dz-ray.z*dx;
                if(std::abs(determinant)<.000001) continue;
                const double ax=a.x-position.x,az=a.z-position.z;
                const double distance=(ax*dz-az*dx)/determinant;
                const double fraction=(ax*ray.z-az*ray.x)/determinant;
                if(distance>=0 && distance<=range && fraction>=0 && fraction<=1 && (mesh.fixedHeight || std::abs(a.y+fraction*(b.y-a.y)-position.y)<8))
                    walls|=1u<<direction;
            }
        }
    }
    return walls;
}
}
