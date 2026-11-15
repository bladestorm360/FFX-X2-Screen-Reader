#include "../navigation_order.hpp"
#include "../camera_direction.hpp"
#include "../intent_cancel_text.hpp"
#include <cstdio>
#include <string>
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
    // Toggling Hide unreachable changes which rows are visible, so the selected
    // entity's row number must be found again for the "N of M" readout.
    {
        std::vector<ffx::Entity> list{row(1,1),row(2,2),row(3,3),row(4,4),row(5,5)};list[1].unreachable=list[2].unreachable=true;
        const auto id=[](const ffx::Entity& e){return e.actor;};
        const auto all=[](const ffx::Entity&){return true;};const auto reachable=[](const ffx::Entity& e){return !e.unreachable;};
        check(ffx::nav::visibleRowOf(list,4,id,all)==3,"with every row shown the fourth destination is row 4 of 5");
        check(ffx::nav::visibleRowOf(list,4,id,reachable)==1,"hiding two unreachable rows makes the same destination row 2 of 3");
        check(ffx::nav::visibleRowOf(list,2,id,reachable)==-1,"a selected destination that becomes hidden has no visible row");
        check(ffx::nav::visibleRowOf(list,9,id,all)==-1,"a destination that left the list has no row");
    }
    // A message box or scripted scene is not a changed destination: say so. Losing focus, a menu,
    // the console or a battle are the player's own context changes and stay silent.
    {
        using ffx::nav::IntentRejection;using ffx::nav::intentCancelText;
        const std::wstring changed=intentCancelText(IntentRejection::Changed),message=intentCancelText(IntentRejection::Message)?intentCancelText(IntentRejection::Message):L"";
        check(changed.find(L"changed")!=std::wstring::npos,"a changed player, input or destination keeps the cancel message");
        check(message!=changed && message.find(L"message")!=std::wstring::npos,"a cancel caused by an open message says a message is open");
        check(intentCancelText(IntentRejection::Context)==nullptr,"a cancel caused by focus, menus, console or battle stays silent");
        // The scene capture accepts the actor-table fallback when the controlled-actor pointer is
        // outside the table, but the native start rejects that state every time. Refuse before
        // queueing an action that cannot run, and say why.
        const std::wstring refusal=ffx::nav::intentRefusalText(false)?ffx::nav::intentRefusalText(false):L"";
        check(refusal.find(L"controlled character")!=std::wstring::npos,"an observation without the controlled character refuses Auto walk before queueing it");
        check(ffx::nav::intentRefusalText(true)==nullptr,"an observation of the controlled character queues Auto walk");
        // Backslash during a scene read the route and then refused. Refuse first, truthfully.
        using ffx::nav::routeStartRefusal;
        const auto during=[](const wchar_t* text) {return text && std::wstring(text)==L"Auto walk unavailable during a scene.";};
        check(during(routeStartRefusal(true,false,true,true)) && during(routeStartRefusal(true,true,false,false)),"a route request during a scene or with field control off refuses before any route, saying a scene is playing");
        check(routeStartRefusal(true,true,true,false)==ffx::nav::intentRefusalText(false),"a field without the controlled character refuses before any route");
        check(!routeStartRefusal(true,true,true,true) && !routeStartRefusal(false,false,false,false),"an open field, or Auto walk off, reads the route as before");
        // Djose: Backslash on "Pedestal reset tile, currently inactive" said only "12 west" and never walked.
        check(ffx::nav::inactiveRouteText(L"Pedestal reset tile",L"12 west")==L"Auto walk unavailable: Pedestal reset tile is currently inactive. Route: 12 west","an inactive destination refuses Auto walk by name and still reads the route");
        check(ffx::nav::inactiveRouteText(L"Pedestal",L"")==L"Auto walk unavailable: Pedestal is currently inactive.","an inactive destination inside its zone refuses without an empty route");
        // Turning Hide unreachable on starts a check; until verdicts arrive the visible count is the
        // unfiltered one, so "6 of 35" would later become "6 of 10". Speak the row without "N of M".
        check(!ffx::nav::rowCountSettled(true,true),"turning Hide unreachable on omits N of M while reachability is checked");
        check(ffx::nav::rowCountSettled(true,false) && ffx::nav::rowCountSettled(false,true) && ffx::nav::rowCountSettled(false,false),"a settled or unfiltered list keeps N of M");
        // Entering Macalania - Trials, ] said "2 of 11", "5 of 23", "4 of 13" while the room loaded.
        // A total is spoken only after the destination count has held for ListSettleMs in this room.
        // Djose: while a pushed pedestal dropped and while the central lift rode, verdicts computed
        // mid-animation hid the selected row and the mod said "Pedestal is unreachable" and the next
        // row, unprompted. A loss is spoken only while the player holds the field; otherwise the
        // selection is kept and the next command after control returns reports it.
        check(!ffx::nav::selectionLossSpoken(true,false),"a selection hidden while the game holds player control stays silent");
        check(!ffx::nav::selectionLossSpoken(false,true),"a selection hidden during a scene or window stays silent");
        check(ffx::nav::selectionLossSpoken(true,true),"a selection hidden while the player has the field is still spoken");
        ffx::nav::ListStability list;
        list.observe(7,11,10000);check(!list.settled(10000),"the first scan in a new room omits N of M");
        list.observe(7,23,10400);check(!list.settled(10400),"a destination count that changed omits N of M");
        list.observe(7,23,11000);check(!list.settled(11000),"a count held for less than the settle time omits N of M");
        list.observe(7,23,11900);check(list.settled(11900),"a count held for the settle time speaks N of M");
        list.observe(8,23,12000);check(!list.settled(12000),"a new room starts unsettled even with the same count");
        list.observe(8,23,13600);list.observe(8,22,13700);check(!list.settled(13700),"a destination that appears or leaves unsettles the total");
        // Djose - Great Hall, 7 destinations, standing still: ] presses 0.5 to 1.3 seconds apart lost
        // their totals long after the room settled; presses 3 seconds apart kept them. Every press
        // rescans, so a scan whose count differs once, or a press that restarts the reachability check,
        // must not take the totals away again once this room has settled.
        {
            ffx::nav::ListStability hall;bool all=true;
            hall.observe(9,7,20000);check(!hall.totals(20000,true,true),"Great Hall entry omits N of M before the list settles");
            hall.observe(9,7,21600);check(hall.totals(21600,true,false),"Great Hall speaks N of M once its seven destinations settle and are checked");
            unsigned long long at=50000;
            for(unsigned press=0;press<8;++press,at+=500) {
                hall.observe(9,press%2?8:7,at);
                all=hall.totals(at,true,press%3==0) && all;
            }
            check(all,"fast ] presses in a settled room keep N of M even when a rescan's count or reachability check changes");
            for(unsigned press=0;press<4;++press,at+=1300) {hall.observe(9,7,at);all=hall.totals(at,true,true) && all;}
            check(all,"presses 1.3 seconds apart in a settled room keep N of M");
            check(hall.totals(at,false,true),"with Hide unreachable off a settled room keeps N of M");
            check(!hall.totals(at,true,true),"turning Hide unreachable on in a settled room still omits N of M until it is checked");
            check(hall.totals(at+100,true,false),"the count returns once the filter has checked the room");
            hall.observe(10,7,at+200);check(!hall.totals(at+200,true,false),"a new room starts unsettled again");
        }
    }
    std::puts("PASS: synthetic navigation order, insertion/removal, explicit refresh, unreachable selection skip and normalized camera-cut threshold.");
}
