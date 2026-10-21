#include "../sphere_sections_data.hpp"
#include "../sphere_expert_data.hpp"
void sectionFollowups() {
    GridSnapshot small; small.character=0; small.positions.fill(0); small.nodes.resize(6);
    for(auto& node:small.nodes) { node.valid=true; node.name=L"Stat"; }
    const std::vector<int> owners={0,-1,-1,1,-1,1};
    small.links={{0,1,0},{1,2,0},{2,3,0},{0,4,0},{4,5,0}};
    small.nodes[1].locked=small.nodes[2].locked=true;
    small.nodes[1].sphere=small.nodes[2].sphere=0x2051;
    small.nodes[4].locked=true; small.nodes[4].sphere=0x2052;
    auto plan=sphereNearestSection(small,owners,1,{{0x2051,1},{0x2052,1}});
    check(plan.reachable && plan.route.nodes==std::vector<int>({0,4,5}),"nearest accessible entry avoids a route requiring two of an owned single key");
    plan=sphereNearestSection(small,owners,1,{{0x2051,2}});
    check(plan.reachable && plan.route.nodes==std::vector<int>({0,1,2,3}) && plan.route.locks==2,"cumulative same-type key requirements are counted");
    plan=sphereNearestSection(small,owners,1,{});
    check(!plan.reachable && !plan.route.nodes.empty(),"missing keys produce a blocked plan, never a reachable claim");
    small.nodes[4].locked=false; small.nodes[5].ability=7;
    plan=sphereNearestSection(small,owners,1,{});
    check(plan.route.quarters==8 && plan.route.standingLevels==2,"character-section arrival stands on ability node rather than stopping at its neighbour");
    auto ability=sphereAbilityRoute(small,5,{});
    check(ability.reachable && ability.route.levels==1 && ability.route.standingLevels==2,"F3 validates owned keys while retaining adjacent ability activation");
    small.nodes[4].locked=true;
    check(!sphereAbilityRoute(small,5,{}).reachable,"F3 rejects missing keys on the only ability routes");
    small.nodes[4].locked=false; small.links[3].travelled=small.links[4].travelled=1;
    small.links[0].travelled=small.links[1].travelled=small.links[2].travelled=1;
    plan=sphereNearestSection(small,owners,1,{{0x2051,2}});
    check(plan.route.nodes==std::vector<int>({0,4,5}) && plan.route.standingLevels==1,
        "route search uses separate rounded movement commits at locks instead of raw connection cost");
    GridSnapshot real; real.character=0; real.selected=341; real.positions=sphereSectionStarts; real.levels.fill(99);
    for(const auto& site:sphereSectionSites) {
        GridNode node; node.valid=true; node.x=site.x; node.y=site.y; node.name=L"Visible node"; real.nodes.push_back(node);
    }
    for(const auto& edge:sphereSectionEdges) real.links.push_back({edge.a,edge.b,0});
    auto sections=sphereSections(real);
    check(sections.size()==860,"complete preserved standard layout matches");
    real.nodes[4].name=L"Defense"; real.nodes[6].name=L"Flee";
    real.positions[0]=real.selected=5;
    check(sphereNextText(real,GridRoute{{5,6}})==L"diagonally up and left to Flee, node 7",
        "Flee requires both Up and Left instead of rounding its steep angle to Up");
    check(sphereNextText(real,GridRoute{{5,4}})==L"up or right to Defense, node 5",
        "Defense can be selected with either single arrow, not a required diagonal");
    check(sphereSectionStep(real,true)==L"Character 0 path. Forward: diagonally up and left to Flee, node 7.",
        "F4 uses the native diagonal selection rule");
    check(sphereSectionStep(real,false)==L"Character 0 path. Backward: up or right to Defense, node 5.",
        "F5 distinguishes alternative arrow choices");
    const auto orientation=sphereOrientation(real);
    check(orientation.find(L"diagonally up and left: Flee, node 7")!=std::wstring::npos &&
        orientation.find(L"up or right: Defense, node 5")!=std::wstring::npos,"O shares the same directional wording");
    real.positions=sphereSectionStarts; real.selected=0;
    for(int i=0;i<7;++i) check(sections[real.positions[i]]==i,"all seven native fresh-save starting nodes bind to their own section");
    check(sphereSectionStep(real,false)==L"Start of Character 0 path.","F5 reports path start at the cursor");
    real.positions[0]=real.selected=341;
    check(sphereSectionStep(real,true).find(L"Character 1 path")!=std::wstring::npos,"character entering Yuna's section follows Yuna's path");
    check(sphereBranchOrientation(real).find(L"Character 0 is on Character 1 path")!=std::wstring::npos,
        "O announces occupied section rather than highlighted or original character path");
    check(sphereBranchOrientation(real).find(L"forward branch")!=std::wstring::npos && sphereBranchOrientation(real).find(L"backward branch")!=std::wstring::npos,
        "Standard O describes branch orientation in both directions");
    real.positions[0]=real.selected=35;
    check(sphereSectionStep(real,true).find(L"End of this branch")!=std::wstring::npos,"small stat branch has a spoken endpoint");
    check(sphereSectionStep(real,false).find(L"to Visible node")!=std::wstring::npos,"branch has backward direction and native node label");
    real.positions[0]=real.selected=377;
    check(sphereSectionStep(real,true).find(L"End of Character 0 path")!=std::wstring::npos,"Tidus main path endpoint is explicit");
    real.positions[0]=real.selected=0; real.nodes[0].x++;
    check(sphereSections(real).empty(),"changed/expert topology cannot silently receive standard metadata");
    real.nodes[0].x--; real.links[0].b=5;
    check(sphereSections(real).empty(),"coordinate match alone does not authorize a changed link layout");
    real.links[0].b=1; fixture=real; opened=true; setSetting(Setting::AutoDetail,0);
    openSpherePathMenu(false); check(captured && contains(L"Character 0 path") && contains(L"1 of 7"),"F2 captures accessible seven-character path menu");
    spherePlannerKey(VK_DOWN); spherePlannerKey(VK_DOWN); spherePlannerKey(VK_RETURN);
    check(!captured && contains(L"Character 2 path") && contains(L"Next:"),"selecting Auron closes menu with nearest-entry route and first direction");
    spherePathKey(false); const auto first=said.back();
    fixture.selected=500;
    sphereSectionKey(false); check(said.back()==first,"F4 selected path guidance stays at pawn while cursor moves");
    spherePathKey(false);
    check(said.back()!=first && fixture.positions[0]==0,"F2 guidance follows cursor while retaining selected path and pawn");
    fixture.selected=540; spherePathKey(false);
    check(contains(L"Character 2 path"),"selected Auron path remains selected after cursor reaches it");
    fixture.selected=541; spherePathKey(true);
    check(contains(L"Character 2 path"),"selected path is retained for backward cursor guidance");
    openSpherePathMenu(true); check(!captured && contains(L"selections cleared"),"Shift F2 clears path selection");
    fixture.selected=5; spherePathKey(false);
    check(contains(L"Character 0 path") && contains(L"Flee, node 7"),"default forward follows occupied pawn path from cursor");
    spherePathKey(true); check(contains(L"Defense, node 5"),"default backward follows occupied pawn path from cursor");
    sphereSectionKey(true); check(contains(L"Start of Character 0 path"),"F5 default starts at pawn instead of cursor");
    fixture.positions[0]=5; fixture.selected=0;
    sphereSectionKey(false); check(contains(L"Flee, node 7"),"F4 follows the moved character while highlight stays behind");
    sphereSectionKey(true); check(contains(L"Defense, node 5"),"F5 follows the moved character backward");
    fixture.positions[0]=0; fixture.selected=5;
    openSpherePlanner(false); spherePlannerKey(VK_F2,true);
    check(!spherePlannerActive() && !captured,"Shift F2 closes and clears an open planner");
    openSpherePathMenu(false); ++epoch; spherePlannerKey(VK_DOWN);
    check(!captured && !spherePlannerActive(),"path menu releases capture on generation change");
    clearSphereTargets();
    GridSnapshot expert; expert.character=0; expert.positions=sphereExpertStarts; expert.selected=expert.positions[0]; expert.levels.fill(99);
    for(const auto site:sphereExpertSites) { GridNode node; node.x=site.x; node.y=site.y; node.valid=site.present; node.name=L"Native stat"; expert.nodes.push_back(node); }
    for(const auto edge:sphereExpertEdges) expert.links.push_back({edge.a,edge.b,0});
    check(sphereExpert(expert) && sphereSections(expert).empty(),"Expert layout including 23 non-existent nodes stays separate from standard sections");
    const int start=expert.positions[0]; int neighbour=-1;
    for(const auto edge:expert.links) if(edge.a==start || edge.b==start) { neighbour=edge.a==start?edge.b:edge.a; if(expert.nodes[neighbour].valid) break; }
    check(neighbour>=0,"Expert fresh start has a visible neighbour");
    expert.nodes[neighbour].flags=1; expert.nodes[neighbour].sphere=0x2046; expert.nodes[neighbour].name=L"Strength +4";
    fixture=expert; openSpherePathMenu(false);
    check(contains(L"stat focus. Strength") && contains(L"1 of 10"),"Expert F2 selects statistics rather than duplicating F3");
    spherePlannerKey(VK_RETURN); check(contains(L"Next unactivated node: Strength +4"),"stat focus targets nearest unactivated native boost");
    fixture.levels[0]=0; sphereSectionKey(false); check(contains(L"Toward Strength +4"),"character guidance remains available when pawn lacks levels");
    fixture.levels[0]=99;
    sphereSectionKey(false); check(contains(L"Toward Strength +4"),"Expert F4 follows selected stat route and describes branches");
    fixture.selected=neighbour;
    sphereSectionKey(false); check(contains(L"Toward Strength +4"),"Expert F4 stays at pawn despite cursor reaching the stat");
    spherePathKey(false); check(contains(L"Activate this node with 1"),"stat focus waits for actual activation");
    fixture.nodes[neighbour].activated=1; sphereSectionKey(false); check(contains(L"No connected, unactivated node"),"activated stat node is not proposed again");
    check(sphereBranchOrientation(fixture).find(L"Nearest backward branch")!=std::wstring::npos || sphereBranchOrientation(fixture).find(L"No backward branch")!=std::wstring::npos,"Expert O includes backward branch orientation");
    fixture.nodes[neighbour].activated=0; fixture.positions[0]=fixture.selected=start;
    int other=start==0?1:0; while(!fixture.nodes[other].valid || other==neighbour) ++other;
    fixture.nodes[other].ability=7; fixture.nodes[other].description=L"Native ability effect";
    openSpherePlanner(false); spherePlannerKey(VK_RETURN);
    sphereSectionKey(false); check(!contains(L"Toward Strength +4"),"F3 replaces the prior F2 stat focus");
    openSpherePathMenu(true); sphereSectionKey(false); check(contains(L"Expert Sphere Grid"),"clear returns Expert guidance to defaults");
    clearSphereTargets();
    fixture=small; fixture.levels.fill(0); fixture.selected=0;
    openSpherePlanner(false); check(contains(L"Currently unreachable"),"F3 menu validates actual Sphere Levels before claiming access");
    spherePlannerKey(VK_RETURN); spherePathKey(false);
    check(contains(L"node 5"),"F3 directions start at cursor without claiming pawn accessibility");
    fixture.nodes[4].locked=true; fixture.nodes[4].sphere=0x2049;
    spherePathKey(false); check(contains(L"node 5"),"locked routes retain cursor directions; F3 selection validates resources");
    clearSphereTargets();
}
