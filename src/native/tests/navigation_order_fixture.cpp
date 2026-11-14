#include "../navigation_order.hpp"
#include "../camera_direction.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
void check(bool value,const char* message) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
ffx::Entity row(int id,double distance) {ffx::Entity e;e.actor=id;e.distance=distance;return e;}
// Replays ]/[ with unreachable rows hidden. Verdicts are matched to targets by position and exact
// facts, like requestOwnedReachability: a new order or any changed fact starts a fresh request with
// no committed verdicts. The jitter row's contact fact changes on every scan, as a push target's
// contact bounds follow the player's live body.
struct Replay {
    std::vector<ffx::Entity> world,entities;std::vector<std::pair<int,double>> targets;std::vector<int> spoken;std::vector<size_t> counts;std::vector<bool> verdict;
    bool committed=false;int selection=0,step=1,jitter=0;unsigned selected=0,lost=0,scans=0;
    static int id(const ffx::Entity& e) {return e.actor;}
    std::vector<const ffx::Entity*> rows() const {std::vector<const ffx::Entity*> r;for(const auto& e:entities) if(!e.unreachable) r.push_back(&e);return r;}
    bool keep() {const auto r=rows();selected=0;for(unsigned i=0;i<r.size();++i) if(r[i]->actor==selection) {selected=i;return true;}return false;}
    void request(bool preserve) {
        std::vector<std::pair<int,double>> ids;for(const auto& e:entities) ids.emplace_back(e.actor,e.zone.contactRadius);
        if(ids==targets) {for(size_t i=0;i<entities.size();++i) entities[i].unreachable=committed && verdict[i];return;}
        targets=ids;verdict.assign(ids.size(),false);committed=false;if(!preserve) for(auto& e:entities) e.unreachable=false;
    }
    void press(int delta) {
        auto found=world;++scans;for(auto& e:found) if(e.actor==jitter) e.zone.contactRadius=scans;
        ffx::nav::orderEntities(found,entities,id,selection!=0);
        ffx::nav::keepVerdicts(found,entities,id);entities=found;request(true);keep();
        const auto r=rows();selected=(selected+delta+static_cast<unsigned>(r.size()))%r.size();step=delta;selection=r[selected]->actor;spoken.push_back(selection);counts.push_back(r.size());
    }
    void settle(int blocked,int also=0) {
        if(!committed) {for(size_t i=0;i<targets.size();++i) verdict[i]=targets[i].first==blocked || (also && targets[i].first==also);committed=true;}
        for(size_t i=0;i<entities.size();++i) entities[i].unreachable=verdict[i];
        if(!selection || keep()) return;++lost;
        const int next=ffx::nav::rowAfterLoss(entities,selection,id,[](const ffx::Entity& e){return !e.unreachable;},step);
        selection=next<0?0:rows()[next]->actor;selected=next<0?0:next;if(selection) spoken.push_back(selection);
    }
};
unsigned said(const Replay& r,int id) {unsigned n=0;for(const int s:r.spoken) n+=s==id;return n;}
}
int main() {
    using namespace ffx;
    const auto identity=[](const Entity& e){return e.actor;};
    std::vector<Entity> previous{row(1,1),row(2,2),row(3,3)};
    std::vector<Entity> found{row(3,1),row(2,2),row(1,3)};
    nav::orderEntities(found,previous,identity,true);
    check(found[0].actor==1 && found[1].actor==2 && found[2].actor==3,"movement preserves selected row's ordinal and neighbours");
    previous=found;found={row(5,.5),row(3,1),row(4,.2),row(1,3)};
    nav::orderEntities(found,previous,identity,true);
    check(found[0].actor==1 && found[1].actor==3 && found[2].actor==4 && found[3].actor==5,"gone row drops; new rows append by distance without reordering survivors");
    nav::orderEntities(found,previous,identity,false);
    check(found[0].actor==4 && found[1].actor==5 && found[2].actor==3 && found[3].actor==1,"explicit refresh or cleared selection restores distance order");
    found.clear();nav::orderEntities(found,previous,identity,true);check(found.empty(),"empty refresh stays empty");
    nav::CopiedCamera first{{1,0,0},{0,0,-1},true},next{{0,0,1},{1,0,0},true};
    check(nav::cameraCut(first,next),"ninety degree camera cut detected");
    next.forward={1,0,1};check(!nav::cameraCut(first,next),"exactly forty-five degrees is not a cut");
    next.forward={2,0,1};check(!nav::cameraCut(first,next),"normalization preserves gradual-pan threshold");
    next.forward={-1,0,0};check(nav::cameraCut(first,next),"opposite camera heading is a cut");
    next.valid=false;check(!nav::cameraCut(first,next),"invalid camera cannot trigger a cut");
    next.valid=true;next.forward={};check(!nav::cameraCut(first,next),"zero camera vector cannot trigger a cut");
    // Held order differs from distance order; ] lands on row 2, whose verdict then hides it.
    Replay ahead;ahead.world={row(1,.5),row(2,.7),row(3,2),row(4,3)};ahead.entities={row(3,2),row(2,.7),row(1,.5),row(4,3)};
    ahead.request(false);ahead.selection=3;
    for(int i=0;i<4;++i) {ahead.press(1);ahead.settle(2);}
    check(said(ahead,2)==1 && ahead.lost==1,"] does not return to a row hidden as unreachable");
    check(ahead.spoken.size()>1 && ahead.spoken[1]==1,"] loss selects the next reachable neighbour, not the first row");
    // [ wraps to the last row, the farthest target, whose verdict then hides it.
    Replay back;back.world={row(1,.5),row(3,2),row(4,3),row(2,14)};back.entities={row(3,2),row(1,.5),row(4,3),row(2,14)};
    back.request(false);back.selection=3;
    for(int i=0;i<4;++i) {back.press(-1);back.settle(2);}
    check(said(back,2)==1 && back.lost==1,"[ does not return to a row hidden as unreachable");
    check(back.spoken.size()>2 && back.spoken[1]==4 && back.spoken[2]==1,"[ loss selects the previous reachable neighbour and keeps moving back");
    // Verdicts are in and the player stands still, but one target's facts differ on every scan.
    // Each ] rescans; the rows hidden as unreachable must stay hidden and the count stable.
    Replay settled;settled.world={row(1,.5),row(2,.7),row(3,1),row(4,2),row(5,3),row(6,4)};settled.jitter=6;
    settled.entities=settled.world;settled.request(false);settled.settle(2,5);settled.selection=1;
    for(int i=0;i<5;++i) {settled.press(1);settled.settle(2,5);}
    check(!said(settled,2) && !said(settled,5),"] after committed verdicts never lands on a hidden row");
    check(settled.lost==0 && settled.counts==std::vector<size_t>(5,4),"] after committed verdicts keeps the reachable count");
    check(settled.spoken==std::vector<int>({3,4,6,1,3}),"] after committed verdicts steps through reachable rows in order");
    for(int i=0;i<3;++i) {settled.press(-1);settled.settle(2,5);}
    check(settled.spoken==std::vector<int>({3,4,6,1,3,1,6,4}) && settled.lost==0,"[ after committed verdicts steps back through reachable rows");
    std::puts("PASS: synthetic navigation order, insertion/removal, explicit refresh, unreachable selection skip and normalized camera-cut threshold.");
}
