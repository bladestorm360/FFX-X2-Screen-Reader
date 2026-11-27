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
    ++searches;checkedPlayer=player;meshPlayer=mesh.player;planeY=mesh.planeY;return player.x>3;
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
    ResultBatch result;check(results.tryRead(result) && result.request==frame.request && result.count==1 &&
        (result.unreachable[0]!=0)==(context.player.x>3),"worker publishes the current pose's verdict");
}
}
int main() {
    using namespace ffx;using namespace ffx::reach;
    Context first;first.actor=1;first.geometry.epoch=1;first.geometry.count=2;first.geometry.vertexCount=3;first.geometry.scale=1;
    first.floor=0;first.player={2,0,2};auto moved=first;moved.player={4,0,4};
    check(!sameContext(first,moved),"movement within the same native floor can change reachability");
    moved=first;
    moved.floor=1;check(!sameContext(first,moved),"floor transition invalidates reachability");
    moved=first;moved.collision.remaps[0]=1;check(!sameContext(first,moved),"native gate change invalidates reachability");
    moved=first;++moved.geometry.epoch;check(!sameContext(first,moved),"map epoch invalidates reachability");
    moved=first;moved.roomState=first.roomState+1;check(!sameContext(first,moved),"trial state change (a wall or door opening) invalidates reachability");
    first.floor=-1;moved=first;moved.player.x=4;check(!sameContext(first,moved),"missing floor still requires the same pose");
    moved=first;moved.player.x=std::numeric_limits<double>::quiet_NaN();check(!sameContext(first,moved),"nonfinite position never matches cache");
    Target band;band.id=7;band.minY=29.8397;band.maxY=57.8443;auto bobbed=band;bobbed.minY+=.01;bobbed.maxY-=.01;
    check(!sameTarget(band,bobbed),"a changed native contact band retires the target verdict");
    prepareSnapshots();first.floor=0;submit(first);const auto initialSearches=searches;
    moved=first;moved.player={4,0,4};submit(moved);
    check(searches==initialSearches+1,"worker recomputes a verdict after a step on the same floor");
    submit(moved,2);check(searches==initialSearches+2 && checkedPlayer.x==4 && meshPlayer.x==4,"new target uses current player in copied mesh");
    moved.floor=1;submit(moved);check(searches==initialSearches+3,"new floor recomputes actual verdict");
    // Besaid (F18): a cached "unreachable" verdict survived walls opening mid-trial. The same
    // target is searched again once the room state changes.
    moved.roomState=7;submit(moved);check(searches==initialSearches+4,"room state change recomputes the cached verdict");
    submit(moved);check(searches==initialSearches+4,"unchanged room state keeps reusing the verdict");
    moved.swimming=true;moved.player.y=20;submit(moved,3);moved.player.y=30;submit(moved,4);
    check(planeY==30 && meshPlayer.y==30,"reused swimming mesh tracks current depth for new targets");
    std::puts("PASS: synthetic reachability retains exact pose, floor and room-state invalidation, verdict reuse and swimming depth.");
}
