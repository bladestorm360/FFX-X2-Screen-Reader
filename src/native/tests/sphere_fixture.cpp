#include "../events.hpp"
#include "../sphere_grid.hpp"
#include "../sphere_owned.hpp"
#include "../settings.hpp"
#include "../mod_menu.hpp"
#include "ready_speech_adapter.hpp"
#include <cstdio>
#include <cstdlib>
namespace ffx {
// Semantic algorithm host: actual Native capture/domain transport is tested separately.
void startSphereWork() {}
bool queueSphereWork(SphereWork,bool,unsigned,bool) {return false;}
void cancelSphereWork(bool) {}
bool spherePreparationOwner() {return true;}
bool sphereOpeningPending() {return false;}
bool nativeGameThread() {return false;}
std::wstring spherePlayerLabel(int id) {return playerName(id);}
std::wstring sphereItemLabel(unsigned id) {return recordName(0x390a40,id);}
unsigned sphereStock(unsigned id) {return quantity(id);}
std::wstring sphereStatLabel(int stat) {return stat<8?menuText(11,stat+7):stat==8?L"HP":L"MP";}
bool sphereEmit(std::wstring text,bool interrupt,const char* producer) {return output(text,interrupt,producer);}
GridSnapshot fixture;
bool opened=true,captured=false;
bool testForeground=true;
unsigned epoch=1,drains=0;
std::vector<std::wstring> said;
bool output(const std::wstring& text,bool,const char*) { said.push_back(text); return true; }
unsigned generation() { return epoch; }
void log(const char*,...) {}
void captureSpeech(bool value) { captured=value; }
void drainController() { ++drains; }
void pauseNavAudio() {}
bool sphereGridOpen() { return opened; }
bool sphereSnapshot(GridSnapshot& result,bool) { result=fixture; return opened; }
std::wstring playerName(int id) { return L"Character "+std::to_wstring(id); }
std::wstring recordName(unsigned,unsigned id) { return L"Sphere "+std::to_wstring(id); }
std::wstring menuText(int,unsigned id) {
    const wchar_t* stats[]={L"Strength",L"Defense",L"Magic",L"Magic Defense",L"Agility",L"Luck",L"Evasion",L"Accuracy"};
    return id>=7 && id<=14?stats[id-7]:L"Native label";
}
unsigned quantity(unsigned id) { return id==0x2049?0:1; }
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring result; for(const auto& part:parts) if(!part.empty()) { if(!result.empty()) result+=separator; result+=part; } return result;
}
}
using namespace ffx;
void check(bool ok,const char* why) { if(!ok) { fprintf(stderr,"FAIL: %s\n",why); exit(1); } }
bool contains(const wchar_t* text) { return said.back().find(text)!=std::wstring::npos; }
#include "sphere_sections_followups.inc"
int main() {
    setSetting(Setting::MenuIndex,1);
    GridSnapshot grid; grid.nodes.resize(8); grid.selected=0; grid.character=0; grid.positions.fill(0); grid.levels.fill(2);
    for(unsigned i=0;i<grid.nodes.size();++i) {
        grid.nodes[i].valid=true; grid.nodes[i].name=L"Stat"; grid.nodes[i].x=static_cast<int>(i)*10;
    }
    grid.nodes[4].name=L"Ability"; grid.nodes[4].description=L"Native ability effect"; grid.nodes[4].ability=7; grid.nodes[4].sphere=0x2049;
    grid.nodes[5].name=L"Passive"; grid.nodes[5].description=L"Native passive effect"; grid.nodes[5].ability=8; grid.nodes[5].sphere=0x2049;
    grid.links={{0,1,1},{1,2,1},{2,3,1},{3,4,1},{0,6,0},{6,4,0},{4,5,0}};
    auto path=sphereRoute(grid,0,4,0);
    check(path.nodes==std::vector<int>({0,1,2,3,4}) && path.quarters==3 && path.levels==1 && path.standingLevels==1,"ability activation stops at neighbour; travelled links use quarter costs");
    auto second=sphereRoute(grid,0,4,1);
    check(second.nodes==std::vector<int>({0,6,4}) && second.levels==1 && second.standingLevels==2,"travel history belongs to requested character; no unnecessary last step");
    check(sphereRoute(grid,0,7,0).nodes.empty(),"disconnected goal");
    check(sphereRoute(grid,-1,4,0).nodes.empty() && sphereRoute(grid,0,4,7).nodes.empty(),"invalid indexes refused");
    grid.nodes[2].locked=true; grid.nodes[2].sphere=0x2051;
    check(sphereRoute(grid,0,4,0).nodes==std::vector<int>({0,6,4}),"open route preferred to a locked shortcut");
    grid.links.resize(4);
    path=sphereRoute(grid,0,4,0);
    check(path.locks==1 && path.levels==2,"lock requires separate movement confirmations with rounded costs");
    const auto report=sphereRouteText(grid,path,0);
    check(report.find(L"1 Sphere 8273")!=std::wstring::npos && report.find(L"1 Sphere 8265")!=std::wstring::npos,"sphere type totals");
    check(report.find(L"Standing")==std::wstring::npos && report.find(L"path from")==std::wstring::npos && report.size()<120,"concise summary excludes route narration");
    grid.nodes[1].sphere=grid.nodes[3].sphere=0x2046;
    check(sphereRouteText(grid,path,0).find(L"2 Sphere 8262")!=std::wstring::npos,"unactivated intermediate stat spheres are counted by actual type");
    grid.nodes[4].activated=1;
    check(sphereRouteText(grid,path,0).find(L"Sphere 8265")==std::wstring::npos,"already activated destination spends no ability sphere");
    grid.selected=3;
    check(sphereOrientation(grid).find(L"node 4")!=std::wstring::npos,"stable node orientation");
    grid.nodes[4].activated=0; grid.selected=0; grid.nodes[2].locked=false;
    grid.links.push_back({4,5,0}); fixture=grid;
    setSetting(Setting::AutoDetail,0); openSpherePlanner(false);
    check(spherePlannerActive() && captured && contains(L"Character 0: Ability") && contains(L"1 of 2"),"F3 opens active character's abilities directly with speech capture");
    spherePlannerKey('I'); check(said.back()==L"Native ability effect","manual ability description");
    spherePlannerKey(VK_DOWN); spherePlannerKey('I'); check(said.back()==L"Native passive effect","passive ability included");
    spherePlannerKey(VK_RETURN); check(!spherePlannerActive() && !captured && contains(L"destination Passive"),"selection saves target, closes capture");
    check(fixture.selected==0 && fixture.positions[0]==0 && fixture.nodes[5].activated==0,"planner leaves game state untouched");
    spherePathKey(false); check(said.back()==L"right to Stat, node 2","backslash starts forward at cursor");
    spherePathKey(true); check(contains(L"Start of this ability path"),"backward at the ability route start");
    fixture.selected=4;
    sphereSectionKey(false); check(said.back()==L"right to Stat, node 2","F4 ability guidance starts at pawn despite a distant cursor");
    sphereSectionKey(true); check(contains(L"Start of this ability path"),"F5 ability guidance starts at pawn");
    for(int cursor=0;cursor<=4;++cursor) {
        fixture.selected=cursor;
        spherePathKey(false);
        const wchar_t* nextName=cursor==3?L"Ability":(cursor==4?L"Passive":L"Stat");
        const auto next=std::wstring(L"right to ")+nextName+L", node "+std::to_wstring(cursor+2);
        check(said.back()==next && fixture.positions[0]==0,"forward ability guidance follows each preview cursor without moving the pawn");
        sphereSectionKey(false);
        check(said.back()==L"right to Stat, node 2" && fixture.positions[0]==0,"F4 guidance remains anchored to the pawn at every preview cursor");
    }
    for(int cursor=1;cursor<=4;++cursor) {
        fixture.selected=cursor;
        spherePathKey(true);
        const auto previous=std::wstring(L"left to Stat, node ")+std::to_wstring(cursor);
        check(said.back()==previous && fixture.positions[0]==0,"backward ability guidance follows each route predecessor without moving the pawn");
    }
    fixture.selected=4;
    fixture.character=1; spherePathKey(false); check(contains(L"standard Sphere Grid"),"no stale destination for another character");
    setSetting(Setting::AutoDetail,1); openSpherePlanner(false);
    check(said.back()==L"Native ability effect","planner Autodetail");
    setSetting(Setting::AutoDetail,0); spherePlannerKey(VK_DOWN);
    check(contains(L"Character 1: Passive"),"planner follows current native character on reopening");
    spherePlannerKey(VK_RETURN); spherePathKey(false);
    check(contains(L"Passive, node 6"),"destination saved to newly active character");
    fixture.character=0; spherePathKey(false);
    check(contains(L"Passive, node 6"),"previous character's destination is preserved");
    openSpherePlanner(false); spherePlannerKey(VK_ESCAPE);
    check(!spherePlannerActive() && !captured,"Escape closes contextual planner directly");
    openSpherePlanner(false);
    ++epoch; spherePlannerKey(VK_DOWN); check(!spherePlannerActive() && !captured,"generation change closes stale planner");
    for(auto& node:fixture.nodes) node.ability=0;
    openSpherePlanner(false); check(!spherePlannerActive() && !captured,"empty ability list does not leave modal capture open");
    opened=false; openSpherePlanner(false); check(!spherePlannerActive(),"planner unavailable outside grid");
    clearSphereTargets(); setSetting(Setting::AutoDetail,0);
    sectionFollowups();
    puts("PASS: Sphere Grid routes, locks, costs/material deficits, contextual active-character abilities, independent destinations, modal capture and read-only state.");
}
