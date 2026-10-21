// Owned planner menus and per-character ability/path/stat destinations. Native
// callers queue SphereWork; only preparation owns vectors, route computation and
// menu text. Preview-cursor steps and character-origin steps intentionally differ.
// See Docs/features/menus/index.md#sphere-grid and dylan_reports_20261018.md.
#include "sphere_owned.hpp"
#include "navigation_cancel.hpp"
#include "events.hpp"
#include "sphere_grid.hpp"
#include "sphere_sections_data.hpp"
#include "settings.hpp"
#include "controller.hpp"
#include "nav_audio.hpp"
#include <atomic>
#include <algorithm>
namespace ffx {
namespace {
std::atomic<bool> active{false};
GridSnapshot menuGrid;
unsigned epoch=0,row=0;
int menuCharacter=-1;
bool pathMenu=false;
bool expertMenu=false;
std::vector<int> abilities;
struct Target { int node=-1; unsigned ability=0; int section=-1; int anchor=-1,stat=-1; };
std::array<Target,7> targets;
std::array<Target,7> guidance;
std::array<bool,7> guidanceRoute{};
unsigned count() { return pathMenu?7:expertMenu?10:static_cast<unsigned>(abilities.size()); }
void close(bool speak=true) {
    active=false; menuGrid={}; abilities.clear(); pathMenu=expertMenu=false; captureSpeech(false); drainController(); pauseNavAudio();
    if(speak) sphereEmit(L"Sphere Grid planner closed",true,"mod-sphere-menu");
}
std::wstring statName(int stat) { return sphereStatLabel(stat); }
SectionRoute abilityPlan(const GridSnapshot& grid,int node) {
    std::map<unsigned,unsigned> inventory;
    for(const auto& item:grid.nodes) if(item.locked && item.sphere && !inventory.count(item.sphere)) inventory[item.sphere]=sphereStock(item.sphere);
    return sphereAbilityRoute(grid,node,inventory);
}
std::wstring abilityAccess(const GridSnapshot& grid,const SectionRoute& plan) {
    if(!plan.complete) return L"Path validation could not finish.";
    if(plan.route.nodes.empty()) return L"Invalid path: no connected route to this ability.";
    std::wstring text;
    if(!plan.reachable) text=L"Invalid path with your current Key Spheres.";
    else if(plan.route.levels>grid.levels[grid.character]) text=L"Currently unreachable: needs "+std::to_wstring(plan.route.levels)+L" Sphere Levels; you have "+std::to_wstring(grid.levels[grid.character])+L".";
    else text=L"Accessible with your current Key Spheres and Sphere Levels.";
    std::map<unsigned,unsigned> keys;
    for(int at:plan.route.nodes) if(grid.nodes[at].locked) ++keys[grid.nodes[at].sphere];
    for(const auto& [id,total]:keys) text+=L" Requires "+std::to_wstring(total)+L" "+sphereItemLabel(id)+L"; you have "+std::to_wstring(sphereStock(id))+L".";
    return text;
}
SectionRoute statPlan(const GridSnapshot& grid,int stat) {
    std::vector<int> goals;
    std::map<unsigned,unsigned> inventory;
    for(const auto& node:grid.nodes) {
        goals.push_back(node.valid && !node.locked && !node.ability && (node.flags&(1u<<stat)) &&
            !(node.activated&(1u<<grid.character))?0:-1);
        if(node.locked && node.sphere && !inventory.count(node.sphere)) inventory[node.sphere]=sphereStock(node.sphere);
    }
    return sphereNearestSection(grid,goals,0,inventory);
}
std::wstring statReport(const GridSnapshot& grid,int stat,bool nextOnly=false) {
    if(!sphereExpert(grid)) return L"Expert stat guidance requires the verified Expert Sphere Grid layout.";
    const auto plan=statPlan(grid,stat); const auto& route=plan.route;
    auto text=statName(stat)+L" focus. ";
    if(!plan.complete) return text+L"The route could not be fully resolved.";
    if(route.nodes.empty()) return text+L"No connected, unactivated node for this stat was found.";
    const bool available=plan.reachable && route.standingLevels<=grid.levels[grid.character];
    if(available && nextOnly && route.nodes.size()>1) return sphereNextText(grid,route);
    const auto& goal=grid.nodes[route.nodes.back()];
    text+=(plan.reachable?L"Next unactivated node: ":L"Key Spheres you do not have block the next node: ")+sphereNodeLabel(grid,route.nodes.back())+L".";
    if(route.nodes.size()==1) return text+L" Activate this node with 1 "+sphereItemLabel(goal.sphere)+L".";
    text+=L" "+std::to_wstring(route.standingLevels)+L" Sphere Levels.";
    if(route.standingLevels>grid.levels[grid.character]) text+=L" Currently unreachable: you have "+std::to_wstring(grid.levels[grid.character])+L" Sphere Levels.";
    std::map<unsigned,unsigned> keys;
    for(int at:route.nodes) if(grid.nodes[at].locked) ++keys[grid.nodes[at].sphere];
    for(const auto& [id,total]:keys) text+=L" Requires "+std::to_wstring(total)+L" "+sphereItemLabel(id)+L"; you have "+std::to_wstring(sphereStock(id))+L".";
    if(available) text+=L" Next: "+sphereNextText(grid,route)+L".";
    return text;
}
SectionRoute pathPlan(const GridSnapshot& grid,int section) {
    std::map<unsigned,unsigned> inventory;
    for(const auto& node:grid.nodes) if(node.locked && node.sphere && !inventory.count(node.sphere)) inventory[node.sphere]=sphereStock(node.sphere);
    return sphereNearestSection(grid,sphereSections(grid),section,inventory);
}
std::wstring pathReport(const GridSnapshot& grid,int section,bool nextOnly=false) {
    const auto name=spherePlayerLabel(section)+L" path";
    if(sphereSections(grid).empty()) return L"Character paths require the standard Sphere Grid layout.";
    const auto plan=pathPlan(grid,section); const auto& route=plan.route;
    if(!plan.complete) return L"The path could not be fully resolved.";
    if(route.nodes.empty()) return L"No connected entry to "+name+L" could be resolved.";
    std::wstring text;
    if(!plan.reachable) text=L"Entry to "+name+L" is blocked by Key Spheres you do not have. Nearest entry: ";
    else if(route.nodes.size()==1) return L"Already on "+name+L" at "+sphereNodeLabel(grid,route.nodes.back())+L".";
    else text=L"Nearest entry to "+name+L": ";
    const bool available=plan.reachable && route.standingLevels<=grid.levels[grid.character];
    if(nextOnly) return sphereNextText(grid,route);
    text+=sphereNodeLabel(grid,route.nodes.back())+L". "+std::to_wstring(route.nodes.size()-1)+L" connections. "+std::to_wstring(route.standingLevels)+L" Sphere Levels.";
    if(route.standingLevels>grid.levels[grid.character]) text+=L" Currently unreachable: you have "+std::to_wstring(grid.levels[grid.character])+L" Sphere Levels.";
    std::map<unsigned,unsigned> keys;
    for(int node:route.nodes) if(grid.nodes[node].locked) ++keys[grid.nodes[node].sphere];
    for(const auto& [id,total]:keys) text+=L" Requires "+std::to_wstring(total)+L" "+sphereItemLabel(id)+L"; you have "+std::to_wstring(sphereStock(id))+L".";
    if(available) text+=L" Next: "+sphereNextText(grid,route)+L".";
    return text;
}
void description(bool interrupt) {
    if(pathMenu) { sphereEmit(pathReport(menuGrid,static_cast<int>(row)),interrupt,"mod-sphere-menu"); return; }
    if(expertMenu) { sphereEmit(statReport(menuGrid,static_cast<int>(row)),interrupt,"mod-sphere-menu"); return; }
    if(row<abilities.size()) {
        const auto& text=menuGrid.nodes[abilities[row]].description;
        sphereEmit(text.empty()?L"No native description is available for this ability.":text,interrupt,"mod-sphere-menu");
    }
}
void announce(bool opening=false) {
    if(pathMenu) {
        sphereEmit((opening?L"Sphere Grid path selection. ":L"")+spherePlayerLabel(static_cast<int>(row))+L" path, "+
            std::to_wstring(row+1)+L" of 7",true,"mod-sphere-menu");
        if(setting(Setting::AutoDetail)) description(false);
        return;
    }
    if(expertMenu) {
        sphereEmit((opening?L"Expert Sphere Grid stat focus. ":L"")+statName(static_cast<int>(row))+L", "+
            std::to_wstring(row+1)+L" of 10",true,"mod-sphere-menu");
        if(setting(Setting::AutoDetail)) description(false);
        return;
    }
    const int node=abilities[row]; auto text=spherePlayerLabel(menuCharacter)+L": "+sphereNodeLabel(menuGrid,node);
    if(menuGrid.nodes[node].activated&(1u<<menuCharacter)) text+=L", activated";
    if(targets[menuCharacter].node==node) text+=L", selected destination";
    text+=L". "+abilityAccess(menuGrid,abilityPlan(menuGrid,node));
    sphereEmit((opening?L"Sphere Grid ability planner. ":L"")+text+menuIndex(row,count()),true,"mod-sphere-menu");
    if(setting(Setting::AutoDetail)) description(false);
}
bool collectAbilities() {
    row=0; abilities.clear();
    for(unsigned i=0;i<menuGrid.nodes.size();++i)
        if(menuGrid.nodes[i].valid && menuGrid.nodes[i].ability && !menuGrid.nodes[i].locked) abilities.push_back(static_cast<int>(i));
    std::stable_sort(abilities.begin(),abilities.end(),[](int a,int b) { return menuGrid.nodes[a].name<menuGrid.nodes[b].name; });
    if(abilities.empty()) {
        sphereEmit(L"No ability nodes were found in this grid.",true,"mod-sphere-menu"); return false;
    }
    return true;
}
void chooseAbility() {
    const int node=abilities[row],character=menuCharacter;
    targets[character]={node,menuGrid.nodes[node].ability};
    guidance[character]={}; guidanceRoute[character]=false;
    const auto name=sphereNodeLabel(menuGrid,node);
    const auto access=abilityAccess(menuGrid,abilityPlan(menuGrid,node));
    close(false);
    sphereEmit(spherePlayerLabel(character)+L": destination "+name+L". "+access+L" F4/F5 guide from the character; Backslash/Shift+backslash from the cursor.",true,"mod-sphere-menu");
}
void chooseStat() {
    const int stat=static_cast<int>(row),character=menuCharacter;
    const auto report=statReport(menuGrid,stat);
    targets[character]={};
    guidance[character]={-1,0,-1,menuGrid.positions[character],stat}; guidanceRoute[character]=true; close(false);
    sphereEmit(spherePlayerLabel(character)+L": "+report+L" F4 guides toward this stat; F5 guides backward. Activate each node to continue to the next.",true,"mod-sphere-menu");
}
void choosePath() {
    const int section=static_cast<int>(row),character=menuCharacter;
    const auto report=pathReport(menuGrid,section);
    targets[character]={};
    guidance[character]={-1,0,section}; guidanceRoute[character]=true; close(false);
    sphereEmit(spherePlayerLabel(character)+L": "+report+L" F4/F5 guide from the character; Backslash/Shift+backslash from the cursor.",true,"mod-sphere-menu");
}
}
bool spherePlannerActive() { return active.load() || sphereOpeningPending(); }
void prepareSphereRetirement() {startSphereWork();}
void resetSpherePlanner() { if(!spherePreparationOwner()) {active=false;captureSpeech(false);pauseNavAudio();cancelSphereWork();return;} if(active) close(false);else {menuGrid={};abilities.clear();pathMenu=expertMenu=false;} }
void clearSphereTargets() { if(!spherePreparationOwner()) {active=false;captureSpeech(false);pauseNavAudio();cancelSphereWork(true);return;} resetSpherePlanner(); targets={}; guidance={}; guidanceRoute={}; }
void openSpherePlanner(bool shift) {
    if(queueSphereWork(SphereWork::Abilities,shift)) return;
    if(active) { close(); return; }
    GridSnapshot snapshot;
    if(!sphereSnapshot(snapshot,true)) { sphereEmit(L"Open the Sphere Grid to choose an ability path.",true,"mod-sphere-menu"); return; }
    if(snapshot.character<0 || snapshot.character>=static_cast<int>(targets.size())) return;
    menuGrid=std::move(snapshot); epoch=generation(); menuCharacter=menuGrid.character;
    if(!collectAbilities()) { menuGrid={}; return; }
    active=true;
    captureSpeech(true); drainController(); pauseNavAudio(); announce(true);
}
void openSpherePathMenu(bool shift) {
    if(queueSphereWork(SphereWork::Paths,shift)) return;
    if(shift) { clearSphereTargets(); sphereEmit(L"Sphere Grid planner selections cleared. Following the character's current path.",true,"mod-sphere-menu"); return; }
    if(active) { close(); return; }
    GridSnapshot snapshot;
    if(!sphereSnapshot(snapshot,true)) { sphereEmit(L"Open the Sphere Grid to select a character path.",true,"mod-sphere-menu"); return; }
    const auto sections=sphereSections(snapshot);
    const bool expert=sphereExpert(snapshot);
    if(sections.empty() && !expert) { sphereEmit(L"This Sphere Grid layout is not supported by path guidance.",true,"mod-sphere-menu"); return; }
    if(snapshot.character<0 || snapshot.character>=7) return;
    menuGrid=std::move(snapshot); epoch=generation(); menuCharacter=menuGrid.character; pathMenu=true;
    if(expert) {
        pathMenu=false; expertMenu=true;
        row=guidance[menuCharacter].stat>=0?guidance[menuCharacter].stat:0;
        active=true; captureSpeech(true); drainController(); pauseNavAudio(); announce(true); return;
    }
    const int at=menuGrid.positions[menuCharacter];
    row=at>=0 && static_cast<size_t>(at)<sections.size() && sections[at]>=0?sections[at]:menuCharacter;
    active=true; captureSpeech(true); drainController(); pauseNavAudio(); announce(true);
}
void spherePlannerKey(unsigned vk,bool shift) {
    if(queueSphereWork(SphereWork::Key,shift,vk)) return;
    if(!active) return;
    if(vk==VK_F2 && shift) { openSpherePathMenu(true); return; }
    if(epoch!=generation() || !sphereGridOpen()) { close(); return; }
    switch(vk) {
    case VK_F2: case VK_F3: case VK_F8: case VK_ESCAPE: case VK_BACK: case VK_LEFT: close(); break;
    case VK_UP: row=(row+count()-1)%count(); announce(); break;
    case VK_DOWN: row=(row+1)%count(); announce(); break;
    case VK_HOME: row=0; announce(); break;
    case VK_END: row=count()-1; announce(); break;
    case VK_PRIOR: row=row>10?row-10:0; announce(); break;
    case VK_NEXT: row=std::min(row+10,count()-1); announce(); break;
    case VK_RETURN: case VK_SPACE: case VK_RIGHT: if(pathMenu) choosePath(); else if(expertMenu) chooseStat(); else chooseAbility(); break;
    case 'I': case 'N': description(true); break;
    }
}
namespace {
// Substitute only the planning origin in this owned copy. The real character
// position remains unchanged while the user explores successive cursor nodes.
GridSnapshot cursorGrid(const GridSnapshot& grid) {
    auto cursor=grid; cursor.positions[cursor.character]=grid.selected; return cursor;
}
std::wstring abilityStep(const GridSnapshot& grid,const Target& target,bool forward) {
    const auto cursor=cursorGrid(grid);
    const auto original=abilityPlan(grid,target.node);
    GridRoute step;
    if(forward) {
        const auto plan=abilityPlan(cursor,target.node);
        if(!plan.complete || plan.route.nodes.empty()) return abilityAccess(cursor,plan);
        step=plan.route;
    } else {
        const auto at=std::find(original.route.nodes.begin(),original.route.nodes.end(),grid.selected);
        if(at!=original.route.nodes.end()) {
            if(at==original.route.nodes.begin()) return L"Start of this ability path.";
            step.nodes={grid.selected,*(at-1)};
        } else step=sphereRoute(grid,grid.selected,grid.positions[grid.character],grid.character);
    }
    return sphereNextText(grid,step);
}
}
void spherePathKey(bool backward) { sphereSectionKey(backward,true); }
std::wstring sphereExpertGuidance(const GridSnapshot& grid,bool forward) {
    const auto target=guidance[grid.character]; const auto cursor=cursorGrid(grid);
    if(target.stat>=0) {
        const auto plan=statPlan(cursor,target.stat);
        if(forward && (!plan.reachable || plan.route.nodes.size()<2)) return statReport(cursor,target.stat);
        return sphereExpertStep(grid,forward,plan.route.nodes.empty()?-1:plan.route.nodes.back(),target.anchor,
            forward && plan.route.nodes.size()>1?plan.route.nodes[1]:-1);
    }
    const auto ability=targets[grid.character];
    if(ability.node>=0 && static_cast<size_t>(ability.node)<grid.nodes.size() && grid.nodes[ability.node].ability==ability.ability)
        return abilityStep(grid,ability,forward);
    return sphereExpertStep(grid,forward);
}
std::wstring sphereStandardGuidance(const GridSnapshot& grid,bool forward) {
    const auto target=guidance[grid.character],ability=targets[grid.character];
    if(!guidanceRoute[grid.character] && ability.node>=0 && static_cast<size_t>(ability.node)<grid.nodes.size() && grid.nodes[ability.node].ability==ability.ability)
        return abilityStep(grid,ability,forward);
    const auto sections=sphereSections(grid); const int at=grid.selected,pawn=grid.positions[grid.character];
    if(sections.empty()) return sphereSectionStep(grid,forward);
    if(at<0 || pawn<0 || static_cast<size_t>(std::max(at,pawn))>=sections.size()) return {};
    const int section=target.section>=0?target.section:sections[pawn];
    if(section>=0 && sections[at]!=section) {
        if(forward) return pathReport(cursorGrid(grid),section,true);
        return spherePlayerLabel(section)+L" path. Backward: "+sphereNextText(grid,sphereRoute(grid,at,sphereSectionStarts[section],grid.character))+L".";
    }
    return sphereSectionStep(grid,forward,section);
}
void sphereOrientationKey(bool shift) {
    if(queueSphereWork(SphereWork::Orientation,shift)) return;
    GridSnapshot grid; if(sphereSnapshot(grid)) sphereEmit(sphereBranchOrientation(grid,guidance[grid.character].anchor)+L" "+sphereOrientation(grid),true,"sphere-orientation");
}
}
