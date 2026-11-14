#include "../reachability_shared.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace {
unsigned searches=0;
ffx::Point checkedPlayer,meshPlayer;
double planeY=0;
void check(bool value,const char* message) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
}
namespace ffx {
bool destinationUnreachable(const Mesh& mesh,Point player,const Entity&) {
    ++searches;checkedPlayer=player;meshPlayer=mesh.player;planeY=mesh.planeY;return false;
}
}
namespace {
void submit(const ffx::reach::Context& context,unsigned targetId=1) {
    using namespace ffx::reach;
    const auto slot=claimSnapshot();check(slot<SnapshotSlots,"claim synthetic worker snapshot");
    auto& frame=snapshot(slot);frame.request=latest.fetch_add(1)+1;frame.context=context;frame.count=1;
    frame.targets[0]={};frame.targets[0].id=targetId;
    Geometry ready;frame.geometryCopied=!cachedGeometryReady.tryRead(ready) || !sameGeometry(ready,context.geometry);
    if(frame.geometryCopied) {
        frame.triangles[0]={{0,1,2},{-1,-1,-1},0};
        frame.triangles[1]={{0,1,2},{-1,-1,-1},0};
        frame.vertices[0]={{0,0,0,0}};frame.vertices[1]={{100,0,0,0}};frame.vertices[2]={{0,0,100,0}};
    }
    check(captures.tryPush(slot),"publish synthetic worker snapshot");drainWorker();
    ResultBatch result;check(results.tryRead(result) && result.request==frame.request && result.count==1,"worker publishes current verdict");
}
}
int main() {
    using namespace ffx;using namespace ffx::reach;
    Context first;first.actor=1;first.geometry.epoch=1;first.geometry.count=2;first.geometry.vertexCount=3;first.geometry.scale=1;
    first.floor=0;first.player={2,0,2};auto moved=first;moved.player={4,0,4};
    check(sameContext(first,moved),"steps within one native floor retain reachability cache");
    moved.floor=1;check(!sameContext(first,moved),"floor transition invalidates reachability");
    moved=first;moved.collision.remaps[0]=1;check(!sameContext(first,moved),"native gate change invalidates reachability");
    moved=first;++moved.geometry.epoch;check(!sameContext(first,moved),"map epoch invalidates reachability");
    first.floor=-1;moved=first;moved.player.x=4;check(sameContext(first,moved),"missing floor uses one-step position bucket");
    moved.player.x=12;check(!sameContext(first,moved),"missing floor crossing a bucket invalidates reachability");
    moved=first;moved.player.x=-.01;check(!sameContext(first,moved),"negative positions use floor buckets");
    moved=first;moved.player.x=std::numeric_limits<double>::quiet_NaN();check(!sameContext(first,moved),"nonfinite position never matches cache");
    prepareSnapshots();first.floor=0;submit(first);const auto initialSearches=searches;
    moved=first;moved.player={4,0,4};submit(moved);
    check(searches==initialSearches,"worker reuses actual verdict after a step on the same floor");
    submit(moved,2);check(searches==initialSearches+1 && checkedPlayer.x==4 && meshPlayer.x==4,"new target uses current player in reused mesh");
    moved.floor=1;submit(moved);check(searches==initialSearches+2,"new floor recomputes actual verdict");
    moved.swimming=true;moved.player.y=20;submit(moved,3);moved.player.y=30;submit(moved,4);
    check(planeY==30 && meshPlayer.y==30,"reused swimming mesh tracks current depth for new targets");
    std::puts("PASS: synthetic reachability floor cache, fallback buckets, invalidation, verdict reuse and swimming pose refresh.");
}
