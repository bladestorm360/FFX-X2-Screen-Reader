#pragma once
#include "runtime.hpp"
#include <array>
#include <map>
namespace ffx {
struct GridNode {
    int x=0,y=0,type=-1;
    unsigned flags=0,ability=0,activated=0;
    unsigned sphere=0;
    bool valid=false,locked=false;
    std::wstring name,description;
};
struct GridLink { int a=-1,b=-1; unsigned travelled=0; };
struct GridSnapshot {
    std::vector<GridNode> nodes;
    std::vector<GridLink> links;
    std::array<int,7> positions{};
    std::array<unsigned,7> levels{};
    int selected=-1,character=-1;
};
struct GridRoute {
    std::vector<int> nodes;
    unsigned quarters=0,levels=0,standingLevels=0,locks=0;
};
struct SectionRoute { GridRoute route; bool reachable=false,complete=true; };
std::array<int,8> sphereArrowChoices(const GridSnapshot&,int from);
std::vector<int> sphereSections(const GridSnapshot&);
std::wstring sphereSectionStep(const GridSnapshot&,bool forward,int section=-1);
SectionRoute sphereNearestSection(const GridSnapshot&,const std::vector<int>& sections,int section,const std::map<unsigned,unsigned>& inventory);
SectionRoute sphereAbilityRoute(const GridSnapshot&,int goal,const std::map<unsigned,unsigned>& inventory);
void sphereSectionKey(bool backward,bool fromCursor=false);
bool sphereExpert(const GridSnapshot&);
std::wstring sphereExpertStep(const GridSnapshot&,bool forward,int destination=-1,int anchor=-1,int first=-1);
std::wstring sphereExpertGuidance(const GridSnapshot&,bool forward);
std::wstring sphereStandardGuidance(const GridSnapshot&,bool forward);
std::wstring sphereBranchOrientation(const GridSnapshot&,int anchor=-1);
void openSpherePathMenu(bool);
void sphereRouteCosts(const GridSnapshot&,GridRoute&,int character,bool standOnGoal=false);
bool sphereGridOpen();
void rememberSphereCharacter(unsigned id);
bool sphereSnapshot(GridSnapshot&,bool materials=false);
GridNode sphereNode(int index,bool materials=false);
std::wstring sphereItemDescription(unsigned id);
GridRoute sphereRoute(const GridSnapshot&,int start,int goal,int character);
std::wstring sphereRouteText(const GridSnapshot&,const GridRoute&,int character);
std::wstring sphereNextText(const GridSnapshot&,const GridRoute&);
std::wstring sphereOrientation(const GridSnapshot&);
std::wstring sphereNodeLabel(const GridSnapshot&,int index);
bool readSphereDescription();
void spherePathKey(bool);
void sphereOrientationKey(bool);
void openSpherePlanner(bool);
bool spherePlannerActive();
void spherePlannerKey(unsigned,bool shift=false);
void resetSpherePlanner();
void clearSphereTargets();
}
