#include "../owned_nav_target.hpp"
#include <cstdlib>
#include <iostream>
#include <new>

namespace {
bool countAllocations=false;
unsigned allocations=0,frees=0;
void check(bool value,const char* message) {
    if(!value) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); }
}
bool samePoint(ffx::Point a,ffx::Point b) {return a.x==b.x&&a.y==b.y&&a.z==b.z;}
bool sameShape(const ffx::InteractionShape& a,const ffx::InteractionShape& b) {
    return samePoint(a.center,b.center)&&samePoint(a.end,b.end)&&a.angle==b.angle&&
        a.width==b.width&&a.depth==b.depth&&a.range==b.range&&a.height==b.height&&
        a.viewAngle==b.viewAngle&&a.shape==b.shape&&a.worker==b.worker&&
        a.valid==b.valid&&a.crossing==b.crossing&&a.talk==b.talk;
}
ffx::InteractionShape shape(unsigned n) {
    ffx::InteractionShape s;
    s.center={double(n)+.1,double(n)+.2,double(n)+.3};
    s.end={double(n)+.4,double(n)+.5,double(n)+.6};
    s.angle=n+.7;s.width=n+.8;s.depth=n+.9;s.range=n+1.1;
    s.height=n+1.2;s.viewAngle=n+1.3;s.shape=n+4;s.worker=int(n)-3;
    s.valid=(n&1)!=0;s.crossing=(n&2)!=0;s.talk=(n&4)!=0;
    return s;
}
ffx::Entity makeEntity(size_t nameLength=17,size_t categoryLength=11,size_t competitors=7) {
    ffx::Entity e;
    e.name.assign(nameLength,L'N');e.category.assign(categoryLength,L'C');
    e.id=71;e.actor=-9;e.worker=123;e.position={1.25,2.5,3.75};e.distance=19.875;
    auto& z=e.zone;
    static_cast<ffx::InteractionShape&>(z)=shape(91);
    z.contactRadius=8.25;z.contactMinY=-17.5;z.contactMaxY=32.75;
    z.approach={4.1,5.2,6.3};z.obstacle={7.4,8.5,9.6};z.obstacleRadius=10.7;
    z.competing.reserve(competitors);
    for(size_t i=0;i<competitors;++i) z.competing.push_back(shape(static_cast<unsigned>(i+1)));
    e.unreachable=true;e.inactive=true;e.pushControl=0x12345678;e.pushLocal=0x87654321;
    e.usefulEntries.words={1,2,3,4,5,6,7,8,9,10};
    e.authoredTrialControl=true;e.butterfly=true;e.hazardous=true;e.butterflyStart=true;
    e.butterflyEpoch=0xA1234567;e.swimApproach=true;e.swimDepth=true;e.swimReady=false;
    e.swimClearance=12.125;e.swimChunk=uintptr_t(0x76543210);
    return e;
}
void roundTrip(size_t n,size_t c,size_t competitors) {
    auto source=makeEntity(n,c,competitors);
    ffx::OwnedNavTarget captured{};
    check(ffx::captureOwnedNavTarget(source,captured),"capture within fixed bounds");
    ffx::Entity restored;
    check(ffx::restoreOwnedNavTarget(captured,restored),"restore valid capture");
    check(source.name==restored.name&&source.category==restored.category,"names preserved");
    check(source.id==restored.id&&source.actor==restored.actor&&source.worker==restored.worker&&
        samePoint(source.position,restored.position)&&source.distance==restored.distance,"identity and position preserved");
    check(sameShape(static_cast<const ffx::InteractionShape&>(source.zone),
                    static_cast<const ffx::InteractionShape&>(restored.zone)),"zone base shape preserved");
    check(source.zone.contactRadius==restored.zone.contactRadius&&source.zone.contactMinY==restored.zone.contactMinY&&
        source.zone.contactMaxY==restored.zone.contactMaxY&&samePoint(source.zone.approach,restored.zone.approach)&&
        samePoint(source.zone.obstacle,restored.zone.obstacle)&&source.zone.obstacleRadius==restored.zone.obstacleRadius,
        "zone contact and obstacle fields preserved");
    check(source.zone.competing.size()==restored.zone.competing.size(),"competitor count preserved");
    for(size_t i=0;i<source.zone.competing.size();++i)
        check(sameShape(source.zone.competing[i],restored.zone.competing[i]),"ordered competitor preserved");
    check(source.unreachable==restored.unreachable&&source.inactive==restored.inactive&&
        source.pushControl==restored.pushControl&&source.pushLocal==restored.pushLocal&&
        source.usefulEntries.words==restored.usefulEntries.words&&
        source.authoredTrialControl==restored.authoredTrialControl&&source.butterfly==restored.butterfly&&
        source.hazardous==restored.hazardous&&source.butterflyStart==restored.butterflyStart&&
        source.butterflyEpoch==restored.butterflyEpoch&&source.swimApproach==restored.swimApproach&&
        source.swimDepth==restored.swimDepth&&source.swimReady==restored.swimReady&&
        source.swimClearance==restored.swimClearance&&source.swimChunk==restored.swimChunk,
        "remaining Entity fields preserved");
}
}

void* operator new(std::size_t n) {
    if(countAllocations) ++allocations;
    if(void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    if(countAllocations) ++allocations;
    if(void* p=std::malloc(n?n:1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {if(countAllocations && p) ++frees;std::free(p);}
void operator delete[](void* p) noexcept {if(countAllocations && p) ++frees;std::free(p);}
void operator delete(void* p,std::size_t) noexcept {if(countAllocations && p) ++frees;std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {if(countAllocations && p) ++frees;std::free(p);}

int main() {
    roundTrip(0,0,0);roundTrip(2048,128,512);
    roundTrip(2048,128,3);

    auto source=makeEntity();ffx::OwnedNavTarget destination{};
    destination.native.id=0xFEDCBA98;destination.category[0]=L'Q';destination.distance=999.5;
    const auto before=destination;
    source.name.assign(2049,L'x');
    check(!ffx::captureOwnedNavTarget(source,destination),"oversize name refused");
    check(std::memcmp(&before,&destination,sizeof(before))==0,"name refusal leaves destination untouched");
    source=makeEntity();source.category.assign(129,L'x');
    check(!ffx::captureOwnedNavTarget(source,destination),"oversize category refused");
    check(std::memcmp(&before,&destination,sizeof(before))==0,"category refusal leaves destination untouched");
    source=makeEntity(1,1,513);
    check(!ffx::captureOwnedNavTarget(source,destination),"oversize competitors refused");
    check(std::memcmp(&before,&destination,sizeof(before))==0,"competitor refusal leaves destination untouched");

    source=makeEntity(2048,128,512);
    countAllocations=true;
    const bool captured=ffx::captureOwnedNavTarget(source,destination);
    countAllocations=false;
    check(captured&&allocations==0&&frees==0,"successful capture performs no heap operations");

    auto corrupted=destination;corrupted.native.name.fill(L'X');
    ffx::Entity ignored;
    check(!ffx::restoreOwnedNavTarget(corrupted,ignored),"unterminated name rejected");
    corrupted=destination;corrupted.category.fill(L'X');
    check(!ffx::restoreOwnedNavTarget(corrupted,ignored),"unterminated category rejected");
    corrupted=destination;corrupted.native.zone.competing.count=513;
    check(!ffx::restoreOwnedNavTarget(corrupted,ignored),"corrupt competitor count rejected");
    std::cout<<"owned navigation target fixture passed\n";
}
