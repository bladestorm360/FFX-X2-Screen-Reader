#include "collision_replay_bytes.inc"
#include "../navigation_scene.hpp"
#include <fstream>
#include <memory>
namespace {
uintptr_t __cdecl collisionControlled() {return playerActor();}
void besaidCollisionFixtures(std::vector<Entity>& list) {
    auto* code=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    check(code!=nullptr,"original collision replay allocation");std::memcpy(code,collisionReplayBody,sizeof(collisionReplayBody));
    for(const auto& fix:collisionReplayFixups) {
        uintptr_t target=0;
        if(fix.va>=0x83e4e0 && fix.va<0x83e5b3)target=reinterpret_cast<uintptr_t>(code)+fix.va-0x83e4e0;
        else if(fix.va==0x1301a44)target=address(0xf01a44);
        else if(fix.va==0x1301998)target=address(0xf01998);
        else if(fix.va==0x82d8e0)target=reinterpret_cast<uintptr_t>(collisionControlled);
        check(target!=0,"original collision has no unresolved external dependency");
        const auto patched=target-(fix.relative?reinterpret_cast<uintptr_t>(code)+fix.offset+4:0);std::memcpy(code+fix.offset,&patched,4);
    }
    FlushInstructionCache(GetCurrentProcess(),code,sizeof(collisionReplayBody));
    using Native=unsigned(__cdecl*)(uintptr_t,int);const auto native=reinterpret_cast<Native>(code);
    unsigned comparisons=0;
    RawNavigationTriangle raw{};put<uintptr_t>(0xf01a44,address(0x2200000));
    for(bool controlled:{false,true})for(unsigned flags:{0u,4u})for(unsigned alias:{0u,1u,2u,13u,14u,15u,47u,64u,128u,129u,0xffffffffu}) {
        nav::CollisionSnapshot snapshot;snapshot.actorIsControlled=controlled;snapshot.actorFlags=flags;snapshot.remaps.fill(alias);
        put<unsigned>(0x2100198,flags);const auto actor=controlled?playerActor():address(0x2110000);put<unsigned>(0x2110198,flags);
        for(unsigned group=48;group<=63;++group)put<unsigned>(0xf01998+group*4,alias);
        for(unsigned group=0;group<256;++group) {raw.collision=group;put(0x2200000,raw);check((native(actor,0)!=0)==nav::blockedForGroup(group,snapshot),"production collision mapping matches original x86 selector/jump tables");++comparisons;}
    }
    // Native aliases retain the full returned value. Only the initial triangle
    // ID is masked. Finite chains are replayed; cycles stay bounded mod-side.
    nav::CollisionSnapshot chain;chain.actorIsControlled=true;
    for(unsigned n=0;n<16;++n) {chain.remaps[n]=n==15?1:49+n;put<unsigned>(0xf01998+(48+n)*4,chain.remaps[n]);}
    raw.collision=48;put(0x2200000,raw);check(native(playerActor(),0)==1 && nav::blockedForGroup(48,chain),"all16 native gate aliases resolve in order");
    chain.remaps[15]=48;check(nav::blockedForGroup(48,chain),"malformed alias cycle remains bounded without executing native infinite loop");

    const std::string root=(ffx_fixture::evidenceRoot()/"offline-output/help_navigation_quit_20261015/besaid").string()+"/";
    const auto file=[&](const char* name,unsigned destination) {std::ifstream s(root+name,std::ios::binary|std::ios::ate);check(s.good(),"retained authorized Besaid collision fixture");const auto size=static_cast<unsigned>(s.tellg());s.seekg(0);s.read(reinterpret_cast<char*>(memory.data()+destination),size);check(s.good(),"complete retained Besaid collision rows");return size;};
    const auto triBytes=file("triangles0.bin",0x2200000),vertexBytes=file("vertices0.bin",0x2300000);
    check(triBytes==979*16 && vertexBytes==657*8,"original Besaid trial floor identity");
    put<int>(0xf01a48,979);put<int16_t>(0x240010a,657);put<float>(0xf01a50,146.87515869140626f);
    put<uintptr_t>(0xf01a44,address(0x2200000));put<uintptr_t>(0xf01a4c,address(0x2300000));put<uintptr_t>(0xf01a54,address(0x2400100));
    put<unsigned>(0x2100198,0);put<int16_t>(0x2100824,-1);
    const Point entrance{-27.49993896484375,-19.530553817749023,82.916259765625};
    put<float>(0x210000c,static_cast<float>(entrance.x));put<float>(0x2100010,static_cast<float>(entrance.y));put<float>(0x2100014,static_cast<float>(entrance.z));
    for(unsigned g=48;g<=63;++g)put<unsigned>(0xf01998+g*4,g<=54?1:0);
    nav::CollisionSnapshot closed;closed.actorIsControlled=true;for(unsigned g=0;g<16;++g)closed.remaps[g]=read<unsigned>(address(0xf01998)+(48+g)*4);
    std::vector<RawNavigationTriangle> triangles(979);std::vector<RawNavigationVertex> vertices(657);
    std::memcpy(triangles.data(),memory.data()+0x2200000,triBytes);std::memcpy(vertices.data(),memory.data()+0x2300000,vertexBytes);
    for(unsigned i=0;i<triangles.size();++i) {check((native(playerActor(),i)!=0)==nav::blockedForGroup(triangles[i].collision,closed),"every actual Besaid triangle matches original native collision");++comparisons;}
    auto mesh=decodeOwnedNavigationMesh(triangles,vertices,146.87515869140626f,closed,false,-1,entrance);
    const Point recess{-24.32514762878418,38.24881362915039,85.58792114257812};
    list.assign(1,Entity{});list[0].id=0xfffe;list[0].worker=40;list[0].position=recess;list[0].name=L"Door sphere recess 1";
    check(routePoints(mesh,entrance,recess).empty() && fixtureDestination(mesh,entrance,list[0]),"actual closed entry passage has No path and is proven unreachable");
    requestReachability(list,entrance);waitHidden(list,true);waitComplete();
    // The production owned-command refresh must preserve proven hidden rows
    // when a new vector describes the same scene. Its worker receives only
    // these copied rows, never the native pointers used by the replay above.
    auto sceneCopy=std::make_unique<nav::Scene>();sceneCopy->epoch=generation();sceneCopy->geometryValid=true;
    check(nav::captureNativeGeometry(sceneCopy->geometry,1),"capture actual Besaid context for owned filter pipeline");
    std::copy(triangles.begin(),triangles.end(),sceneCopy->triangles.begin());std::copy(vertices.begin(),vertices.end(),sceneCopy->vertices.begin());
    const auto ownerId=nativeOwner;nativeOwner=0;
    requestOwnedReachability(*sceneCopy,list);const auto ownedRequestId=reach::latest.load();
    const auto until=GetTickCount64()+5000;
    while(!list[0].unreachable && GetTickCount64()<until) {applyOwnedReachability(*sceneCopy,list);Sleep(1);}
    check(list[0].unreachable,"actual Besaid destination is hidden through owned command worker pipeline");
    for(unsigned repeat=0;repeat<8;++repeat) {
        auto refreshed=list;refreshed[0].unreachable=false;requestOwnedReachability(*sceneCopy,refreshed);
        check(refreshed[0].unreachable && reach::latest.load()==ownedRequestId,"unchanged command refresh preserves hidden verdict and in-flight job before filtering");
    }
    retireOwnedReachability();nativeOwner=ownerId;
    put<unsigned>(0xf01998+48*4,0);closed.remaps[0]=0; // Original Door glyph worker38 PC33914.
    mesh=decodeOwnedNavigationMesh(triangles,vertices,146.87515869140626f,closed,false,-1,entrance);
    check(!routePoints(mesh,entrance,recess).empty() && !fixtureDestination(mesh,entrance,list[0]),"original glyph wall48 change restores actual Besaid walking route");
    requestReachability(list,entrance);waitHidden(list,false);waitComplete();
    VirtualFree(code,0,MEM_RELEASE);
    std::printf("PASS: original x86 collision + dispatch tables: %u comparisons; actual Besaid979-triangle floor closes/opens route and worker filter at original wall48.\n",comparisons);
}
}
