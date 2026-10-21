void zoneFixtures() {
    Mesh counter;
    counter.triangles={triangle({0,0,0},{8,0,0},{0,0,10}),triangle({8,0,0},{8,0,10},{0,0,10}),
        triangle({10,0,0},{20,0,0},{10,0,10}),triangle({20,0,0},{20,0,10},{10,0,10})};
    counter.triangles[0].neighbours[0]=1; counter.triangles[1].neighbours[0]=0;
    counter.triangles[2].neighbours[0]=3; counter.triangles[3].neighbours[0]=2;
    const unsigned vertices[][3]={{0,1,2},{1,3,2},{4,5,6},{5,7,6}};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<3;++j) counter.triangles[i].vertices[j]=vertices[i][j];
    const Point player{1,0,5}; InteractionZone zone;
    zone.valid=true; zone.shape=1; zone.center={12,0,5}; zone.range=5; zone.height=2;
    check(routePoints(counter,player,zone.center).empty(),"counter blocks a route to the actor center");
    auto route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x<=8 && inInteractionZone(zone,route.back()),"reachable front-of-counter interaction zone replaces inaccessible actor center");
    Entity target; target.position=zone.center; target.zone=zone;
    check(!destinationUnreachable(counter,player,target),"unreachable filter keeps a shop with a reachable counter zone");
    target.zone={}; check(destinationUnreachable(counter,player,target),"filter excludes a disconnected destination");
    check(!destinationUnreachable(Mesh{},player,target),"unknown floor cannot silently hide a destination");
    zone.range=2; check(interactionRoute(counter,player,zone).empty(),"no route is invented when a zone misses every connected floor");
    zone.range=5; zone.center.y=100; check(interactionRoute(counter,player,zone).empty(),"talk height excludes a separate floor");
    zone.center.y=0; zone.height=-1;
    check(!inInteractionZone(zone,{12,6,5}),"negative circular height uses native three-dimensional range");
    zone.height=0; check(!inInteractionZone(zone,{12,0,5}),"zero circular talk height is not an accessible zone");
    zone.shape=3; zone.width=2; zone.depth=4; zone.angle=1.5707963267948966; zone.range=0; zone.height=2;
    route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x<=8 && inInteractionZone(zone,route.back()),"rotated native rectangle reaches its accessible edge");
    zone.center={6,0,0}; zone.end={6,0,10}; zone.shape=2; zone.height=2; zone.crossing=true;
    route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x>6 && route.back().x<=8,"line exit destination crosses the native trigger rather than stopping on it");
    zone.shape=3; zone.center={6,0,5}; zone.width=1; zone.depth=2; zone.angle=0;
    route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x>5,"rectangle crossing target is inside the native boundary");
    Mesh room; room.triangles={triangle({0,0,0},{100,0,0},{0,0,100}),triangle({100,0,0},{100,0,100},{0,0,100})};
    room.triangles[0].neighbours[0]=1; room.triangles[1].neighbours[0]=0;
    const unsigned ids[][3]={{0,1,2},{1,3,2}};
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<3;++j) room.triangles[i].vertices[j]=ids[i][j];
    InteractionZone pedestal; pedestal.valid=true; pedestal.shape=1; pedestal.center={50,0,40};
    pedestal.range=20; pedestal.height=5; pedestal.talk=true; pedestal.worker=7;
    InteractionShape recess; recess.valid=true; recess.shape=3; recess.center={53,0,40};
    recess.width=20; recess.depth=5; recess.range=10; recess.height=5; recess.talk=true; recess.worker=44;
    pedestal.competing={recess};
    check(!inInteractionZone(pedestal,{31,0,40}),"inside pedestal range is insufficient while the rectangular recess wins");
    auto push=interactionRoute(room,{20,0,40},pedestal);
    check(!push.empty() && inInteractionZone(pedestal,push.back()) && distanceXZ(push.back(),pedestal.center)<10,
        "pedestal route enters the native selection region instead of stopping at its larger talk radius");
    InteractionZone slot; static_cast<InteractionShape&>(slot)=recess;
    slot.competing={static_cast<const InteractionShape&>(pedestal)};
    auto insert=interactionRoute(room,{20,0,40},slot);
    check(!insert.empty() && inInteractionZone(slot,insert.back()) && !inInteractionZone(pedestal,insert.back()),
        "pedestal and recess have distinct actionable route endpoints");
    check(!inInteractionZone(pedestal,{50,0,30}),"equally scored overlapping controls cannot claim arrival");
    pedestal.competing[0].viewAngle=.05;
    check(inInteractionZone(pedestal,{50,0,30}),"native facing cone excludes a competing control outside the facing direction");
    pedestal.competing[0].center=pedestal.center; pedestal.competing[0].range=100;
    pedestal.competing[0].width=100; pedestal.competing[0].depth=100;
    pedestal.competing[0].viewAngle=3.141592653589793; pedestal.shape=3; pedestal.range=100;
    check(interactionRoute(room,{20,0,40},pedestal).empty(),"indistinguishable native controls cannot produce a false arrival");
    InteractionZone contact; contact.valid=true; contact.shape=1; contact.center={50,0,40};
    contact.contactRadius=7; contact.approach={0,0,1};
    contact.competing={recess};
    auto approach=interactionRoute(room,{20,0,40},contact);
    check(!approach.empty() && std::abs(approach.back().x-50)<.65 && approach.back().z>47,
        "Besaid push route approaches the rear collision boundary rather than its talk circle");
    check(!inInteractionZone(contact,{50,0,59}) && !inInteractionZone(contact,{50,0,33}),
        "talk range and the opposite pedestal face cannot report push arrival");
    check(inInteractionZone(contact,{50,0,47.25}) && !inInteractionZone(contact,{53,0,47}),
        "push arrival requires close aligned contact and ignores sphere-recess talk competition");
    auto opposite=interactionRoute(room,{50,0,20},contact);
    check(opposite.size()>2 && inInteractionZone(contact,opposite.back()),
        "approaching Besaid from the opposite face detours around the physical pedestal");
    bool clear=true;
    for(size_t i=1;i<opposite.size();++i) {
        const auto a=opposite[i-1],b=opposite[i];
        const double x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
        const double t=length>0?std::clamp(((50-a.x)*x+(40-a.z)*z)/length,0.0,1.0):0;
        if(distanceXZ({a.x+t*x,0,a.z+t*z},contact.center)<7) clear=false;
    }
    check(clear,"every push detour leg stays outside the actor collision cylinder");
    contact.contactMinY=-2; contact.contactMaxY=4;
    check(inInteractionZone(contact,{50,3,47.25}) && !inInteractionZone(contact,{50,5,47.25}),
        "native vertical bounds permit slope contact while excluding a separate height");
    contact.contactMinY=-1e30; contact.contactMaxY=1e30;
    contact.approach={};
    approach=interactionRoute(room,{20,0,40},contact);
    check(!approach.empty() && approach.back().x<43 && std::abs(approach.back().z-40)<.65,
        "Kilika chooses a reachable cardinal push face without aiming at the recess");
    contact.contactRadius=10; contact.approach={0,0,1};
    approach=interactionRoute(room,{20,0,40},contact);
    check(!approach.empty() && approach.back().z>50,
        "updated actor collision size changes the push approach endpoint");
    Mesh water; water.openPlane=true; water.fixedHeight=true; water.planeY=-50;
    auto swim=routePoints(water,{1,-50,1},{90,-50,90});
    check(swim.size()==2 && swim.back().y==-50,"native free swimming routes stay on the current height plane");
    contact.center.y=-50; contact.approach={0,0,1}; contact.contactRadius=7;
    const auto openDetour=interactionRoute(water,{50,-50,20},contact);
    check(openDetour.size()>2 && inInteractionZone(contact,openDetour.back()),
        "open-plane contact routes also avoid crossing the target actor");
    zone.valid=true; zone.shape=1; zone.center={50,-50,40}; zone.range=5; zone.height=2; zone.crossing=false;
    check(!interactionRoute(water,{1,-50,1},zone).empty(),"free swimming reaches native interaction zones without a walking mesh");
    zone.center.y=0; check(interactionRoute(water,{1,-50,1},zone).empty(),"free swimming cannot route to ship-deck interactions at another height");
    zone.valid=false; check(interactionRoute(counter,player,zone).empty(),"unresolved geometry never manufactures an interaction destination");
}
