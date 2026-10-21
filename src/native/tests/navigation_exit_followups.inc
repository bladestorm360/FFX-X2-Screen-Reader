namespace {
Mesh exitStrip(unsigned count,double length=10) {
    Mesh mesh;
    for(unsigned i=0;i<count;++i) {
        const double left=i*length,right=(i+1)*length;
        auto a=triangle({left,0,0},{right,0,0},{left,0,10});
        auto b=triangle({right,0,0},{right,0,10},{left,0,10});
        const int vertex=static_cast<int>(i)*2,index=static_cast<int>(i)*2;
        a.vertices[0]=vertex; a.vertices[1]=vertex+2; a.vertices[2]=vertex+1;
        b.vertices[0]=vertex+2; b.vertices[1]=vertex+3; b.vertices[2]=vertex+1;
        a.neighbours[0]=index+1; b.neighbours[0]=index;
        if(i) a.neighbours[1]=index-1;
        if(i+1<count) b.neighbours[1]=index+2;
        mesh.triangles.push_back(a); mesh.triangles.push_back(b);
    }
    return mesh;
}
void exitValidationFixtures() {
    auto mesh=exitStrip(1,8);
    Entity exit; exit.category=L"Exit";
    exit.zone.valid=true; exit.zone.shape=2; exit.zone.crossing=true;
    exit.zone.center={8,0,0}; exit.zone.end={8,0,10}; exit.zone.height=2;
    exit.position=exit.zone.center;
    for(double x:{1.0,4.0,7.5,8.0}) {
        check(!destinationUnreachable(mesh,{x,0,5},exit),
            "enabled boundary exit stays listed while the player walks across the map");
    }
    check(!destinationUnreachable(Mesh{},{1,0,5},exit),"missing collision data cannot remove an enabled exit");
    check(!destinationUnreachable(mesh,{100,50,100},exit),"unknown player floor cannot remove an enabled exit");
    exit.zone.center={6,0,0}; exit.zone.end={6,0,10};
    for(double x:{1.0,4.0,7.5}) {
        const auto route=interactionRoute(mesh,{x,0,5},exit.zone);
        check(!route.empty() && (x-6)*(route.back().x-6)<0,
            "interior line exit routes across the trigger from either side");
    }
    auto extension=exitStrip(1,200);
    exit.zone.center={100,0,0}; exit.zone.end={100,0,5};
    auto extensionRoute=interactionRoute(extension,{1,0,9},exit.zone);
    check(!extensionRoute.empty() && extensionRoute.back().x>100 && extensionRoute.back().z<=5,
        "far endpoint-extension approach uses a perpendicular finite-line crossing");
    auto slope=mesh;
    for(auto& t:slope.triangles) { for(auto& p:t.points) p.y=p.z; t.center.y=t.center.z; }
    exit.zone.center={6,0,0}; exit.zone.end={6,10,10}; exit.zone.height=.5;
    check(!interactionRoute(slope,{1,5,5},exit.zone).empty(),"sloped line height uses interpolated trigger and floor geometry");
    exit.zone.center.y+=3; exit.zone.end.y+=3;
    check(interactionRoute(slope,{1,5,5},exit.zone).empty(),"crossing another height cannot be reported as a valid route");
    Mesh around;
    const double xs[]={0,9.5,10.5,20},zs[]={-10,-2,10};
    for(unsigned row=0;row<2;++row) for(unsigned col=0;col<3;++col) {
        if(row==1 && col==1) continue;
        auto a=triangle({xs[col],0,zs[row]},{xs[col+1],0,zs[row]},{xs[col],0,zs[row+1]});
        auto b=triangle({xs[col+1],0,zs[row]},{xs[col+1],0,zs[row+1]},{xs[col],0,zs[row+1]});
        a.vertices[0]=row*4+col; a.vertices[1]=row*4+col+1; a.vertices[2]=(row+1)*4+col;
        b.vertices[0]=row*4+col+1; b.vertices[1]=(row+1)*4+col+1; b.vertices[2]=(row+1)*4+col;
        around.triangles.push_back(a); around.triangles.push_back(b);
    }
    for(unsigned i=0;i<around.triangles.size();++i) for(unsigned j=i+1;j<around.triangles.size();++j) {
        auto& a=around.triangles[i]; auto& b=around.triangles[j]; unsigned shared=0;
        for(int x:a.vertices) for(int y:b.vertices) if(x==y) ++shared;
        if(shared==2) {
            for(auto& n:a.neighbours) if(n<0) { n=j; break; }
            for(auto& n:b.neighbours) if(n<0) { n=i; break; }
        }
    }
    // Connected route goes below z=-2, around the finite trigger's endpoint.
    exit.zone.center={10,0,0}; exit.zone.end={10,0,10}; exit.zone.height=2;
    check(interactionRoute(around,{1,0,5},exit.zone).empty(),
        "being on opposite sides of an infinite line does not prove the actual route crosses its finite trigger");
    auto bridge=exitStrip(3);
    exit.zone={}; exit.zone.valid=true; exit.zone.shape=3; exit.zone.crossing=true;
    exit.zone.center={28,0,5}; exit.zone.width=1; exit.zone.depth=2; exit.zone.height=2;
    exit.position=exit.zone.center;
    check(!destinationUnreachable(bridge,{1,0,5},exit),"open native bridge keeps its exit reachable");
    for(unsigned i:{2u,3u}) { bridge.triangles[i].collision=1; bridge.triangles[i].collisionGroup=53; }
    check(interactionRoute(bridge,{1,0,5},exit.zone).empty(),"native bridge closure removes the live route");
    check(destinationUnreachable(bridge,{1,0,5},exit),"filter proves a native mutable gate removed exit access");
    check(!interactionRoute(bridge,{25,0,5},exit.zone).empty() && !destinationUnreachable(bridge,{25,0,5},exit),
        "closed gate does not hide an exit on the player's side");
    for(unsigned i:{2u,3u}) bridge.triangles[i].collision=0;
    check(!destinationUnreachable(bridge,{1,0,5},exit),"event reopening restores exit access immediately");
    for(unsigned i:{2u,3u}) { bridge.triangles[i].collision=1; bridge.triangles[i].collisionGroup=2; }
    check(!destinationUnreachable(bridge,{1,0,5},exit),"static blocked walls are not evidence of an event removing exit access");
    bridge.triangles[0].neighbours[0]=-1; bridge.triangles[0].neighbours[1]=-1;
    check(!destinationUnreachable(bridge,{1,0,1},exit),"static disconnected exit geometry cannot silently remove an enabled exit");
    auto malformed=exitStrip(2);
    malformed.triangles.back().vertices[0]=100; malformed.triangles.back().vertices[1]=101;
    malformed.triangles.back().vertices[2]=102;
    auto seam=malformed;
    check(!routePoints(seam,{1,0,1},{18,0,8}).empty(),
        "native adjacent polygons with duplicate coordinate vertices remain traversable at an exported seam");
    for(auto& p:malformed.triangles.back().points) p.y+=1;
    malformed.triangles.back().center.y+=1;
    Entity object; object.position={18,0,8};
    check(!path(malformed,0,3).empty() && routePoints(malformed,{1,0,1},object.position).empty(),
        "fixture separates connected adjacency from malformed shared-portal geometry");
    check(!destinationUnreachable(malformed,{1,0,1},object),"route-construction failure cannot hide a connected object");
    object.position={1000,0,1000};
    check(!destinationUnreachable(malformed,{1,0,1},object),"unknown target floor cannot silently hide an object");
    Mesh overlap;
    overlap.triangles={triangle({10,0,0},{20,0,0},{10,0,10}),
        triangle({0,0,0},{10,0,0},{10,0,10}),triangle({10,0,0},{20,0,0},{10,0,10})};
    const int ids[][3]={{10,11,12},{0,1,2},{1,3,2}};
    for(unsigned i=0;i<3;++i) for(unsigned j=0;j<3;++j) overlap.triangles[i].vertices[j]=ids[i][j];
    overlap.triangles[1].neighbours[0]=2; overlap.triangles[2].neighbours[0]=1;
    InteractionZone zone; zone.valid=true; zone.shape=1; zone.center={12,0,3}; zone.height=2; zone.range=.5;
    auto route=interactionRoute(overlap,{9,0,1},zone);
    check(!route.empty() && inInteractionZone(zone,route.back()),
        "connected interaction endpoint retains its chosen polygon when a disconnected floor overlaps");
    Mesh ambiguous=overlap;
    ambiguous.triangles[0]=ambiguous.triangles[1];
    ambiguous.triangles[0].collision=1; ambiguous.triangles[0].collisionGroup=53;
    ambiguous.triangles[1].neighbours[0]=-1;
    exit.zone=zone;
    check(!destinationUnreachable(ambiguous,{9,0,1},exit),
        "relaxed collision proof cannot replace the player's floor with a disconnected overlapping polygon");
    auto distant=exitStrip(1000,1000);
    exit.zone={}; exit.zone.valid=true; exit.zone.shape=3; exit.zone.crossing=true;
    exit.zone.center={999998,0,5}; exit.zone.width=1; exit.zone.depth=2; exit.zone.height=2;
    exit.position=exit.zone.center;
    for(Point player:std::vector<Point>{{1,0,5},{250001,0,5},{999990,0,5}}) {
        route=interactionRoute(distant,player,exit.zone);
        check(!route.empty() && inInteractionZone(exit.zone,route.back()) && !destinationUnreachable(distant,player,exit),
            "full-map exits remain reachable at arbitrary player-to-target distance");
    }
    object.position={999998,0,5};
    check(!routePoints(distant,{1,0,5},object.position).empty() && !destinationUnreachable(distant,{1,0,5},object),
        "other entities have no path distance or traversal-count limit");
    auto inn=exitStrip(5);
    // An unusable adjacency shortcut must not prevent the valid floor route.
    inn.triangles[0].neighbours[2]=8;
    exit.zone.center={48,0,5}; exit.zone.width=1; exit.zone.depth=2;
    for(Point player:std::vector<Point>{{1,0,5},{21,0,5},{45,0,5}}) {
        route=interactionRoute(inn,player,exit.zone);
        check(!route.empty() && inInteractionZone(exit.zone,route.back()),
            "inn approach remains routeable from distant and nearby positions when a shorter adjacency has no physical portal");
    }
    auto levels=exitStrip(2);
    levels.triangles.insert(levels.triangles.begin(),triangle({0,1,0},{10,1,0},{0,1,10}));
    for(unsigned i=1;i<levels.triangles.size();++i) for(auto& next:levels.triangles[i].neighbours) if(next>=0) ++next;
    levels.player={1,.8,1}; levels.playerTriangle=1;
    check(findTriangle(levels,levels.player)==1,"native player floor wins over a nearer disconnected overlapping polygon");
    route=routePoints(levels,levels.player,{18,0,8});
    check(!route.empty(),"native player floor supplies a full route before approaching the destination");
    levels.playerTriangle=100;
    check(findTriangle(levels,levels.player)==0,"invalid native floor falls back to geometric lookup");
    Mesh winding;
    for(unsigned row=0;row<3;++row) for(unsigned col=0;col<2;++col) {
        if(row==1 && col==1) continue;
        const double x=col*10.0,z=row*10.0;
        auto a=triangle({x,0,z},{x+10,0,z},{x,0,z+10});
        auto b=triangle({x+10,0,z},{x+10,0,z+10},{x,0,z+10});
        const int v=static_cast<int>(row*3+col);
        a.vertices[0]=v; a.vertices[1]=v+1; a.vertices[2]=v+3;
        b.vertices[0]=v+1; b.vertices[1]=v+4; b.vertices[2]=v+3;
        winding.triangles.push_back(a); winding.triangles.push_back(b);
    }
    for(unsigned i=0;i<winding.triangles.size();++i) for(unsigned j=i+1;j<winding.triangles.size();++j) {
        auto& a=winding.triangles[i]; auto& b=winding.triangles[j]; unsigned shared=0;
        for(int x:a.vertices) for(int y:b.vertices) if(x==y) ++shared;
        if(shared==2) {
            for(auto& next:a.neighbours) if(next<0) { next=j; break; }
            for(auto& next:b.neighbours) if(next<0) { next=i; break; }
        }
    }
    exit.zone={}; exit.zone.valid=true; exit.zone.shape=2; exit.zone.crossing=true;
    exit.zone.center={10,0,1}; exit.zone.end={10,0,9}; exit.zone.height=2;
    for(Point player:std::vector<Point>{{15,0,25},{5,0,5}}) {
        route=interactionRoute(winding,player,exit.zone);
        check(!route.empty() && route.back().x>10,
            "distant winding inn route crosses the finite trigger even when its endpoint shares the player's original side");
        bool crossed=false;
        for(size_t i=1;i<route.size();++i) if((route[i-1].x-10)*(route[i].x-10)<0) {
            const double t=(10-route[i-1].x)/(route[i].x-route[i-1].x);
            const double z=route[i-1].z+t*(route[i].z-route[i-1].z);
            if(z>=1 && z<=9) crossed=true;
        }
        check(crossed,"winding route proves a real segment crosses the finite exit");
    }
}
}
