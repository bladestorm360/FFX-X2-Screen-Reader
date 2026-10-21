// Original trial script rules. This module neither changes state nor requests scripts.
#include "pedestal.hpp"
#include "nav_format.hpp"
#include <cmath>
namespace ffx {
namespace {
void move(PedestalPlan& plan,Point goal,unsigned held,PushMode mode=PushMode::Slide) {
    plan.moves.push_back({goal,held,mode});
}
}
PedestalPlan pedestalPlan(unsigned control,unsigned local,const PedestalSnapshot& s) {
    PedestalPlan plan;
    const auto p=s.position;
    switch(control) {
    case 1: // bsyt0100: four original edges around the final depression.
        plan.producer=7; plan.entry=4;
        plan.complete=s.get(0x5c000100)==1;
        if(plan.complete) break;
        if(s.get(0x5c0000cc)!=1) { plan.blocked=L"Insert the Besaid Sphere into the pedestal sphere recess first."; break; }
        if(std::abs(p.x+14.4117183685)>1) move(plan,{-14.4117183685,p.y,p.z},p.x< -14.4117183685?0x2000:0x8000);
        move(plan,{-14.4117183685,p.y,-147.6743469238},p.z> -147.6743469238?0x1000:0x4000);
        break;
    case 2: // klyt1000: reset position, then the actor-only depression edge.
        plan.producer=6; plan.entry=4;
        plan.complete=s.get(0x0c000064)==1;
        if(plan.complete) break;
        if(s.get(0x0c00005a)!=1) { plan.blocked=L"Insert a Kilika Sphere into the pedestal first."; break; }
        if(p.z<100) { plan.blocked=L"Step on the pedestal reset glyph before this push."; break; }
        if(std::abs(p.x-50.0109901428)>1) move(plan,{50.0109901428,p.y,p.z},p.x<50.0109901428?0x2000:0x8000);
        // Crossing Z196.276 starts the native plunge to Z205.153. Keep the
        // requested destination past the original edge, not on its boundary.
        move(plan,{50.0109901428,p.y,198.2762908936},0x1000);
        break;
    case 3: // Djose's type4 worker42 owns continuous directional pushing.
        plan.producer=42; plan.entry=1;
        plan.complete=s.get(0x0c0000d9)==1;
        if(plan.complete) break;
        if(s.get(0x0c0000db)) { plan.blocked=L"Pushing is currently inactive."; break; }
        if(s.get(0x0c00009b)==3 && s.get(0x0c00009d)>0 && s.get(0x0c00009d)!=10 &&
            s.get(0x0c00009e)>0 && s.get(0x0c00009e)!=10) {
            plan.phase=1;
            if(std::abs(p.x)>1) move(plan,{0,p.y,p.z},p.x<0?0x2000:0x8000);
            move(plan,{0,p.y,86},0x1000);
        } else if(s.get(0x0c000099)>0 && s.get(0x0c000099)!=10 &&
                  s.get(0x0c00009a)>0 && s.get(0x0c00009a)!=10) {
            plan.complete=p.x>69 && p.x<71 && p.z>22 && p.z<26;
            if(!plan.complete) {
                if(std::abs(p.z-24)>1) move(plan,{p.x,p.y,24},p.z<24?0x1000:0x4000);
                move(plan,{70,p.y,24},p.x<70?0x2000:0x8000);
            }
        } else plan.blocked=L"Complete the charging wall spheres or load both pedestal sockets for the gap first.";
        break;
    case 4: // Macalania: one launch per native state; original script rides the ramp.
        plan.producer=29; plan.entry=1; plan.phase=s.get(0x0c0000a3);
        switch(plan.phase) {
        case 0:
            if(!s.get(0x0c0000c6)) plan.blocked=L"Restore the first ice mound with its Macalania Sphere before pushing.";
            else move(plan,{p.x,p.y,52},0x2000,PushMode::Scripted);
            break;
        case 1:
            if(s.get(0x0c000096)!=4) plan.blocked=L"Restore the lower-floor ramp with its Macalania Sphere before pushing.";
            else move(plan,{-83,p.y,p.z},0x1000,PushMode::Scripted);
            break;
        case 3:
            if(s.get(0x0c000097)!=4) plan.blocked=L"Insert a Macalania Sphere into the pedestal before the bridge-support push.";
            else move(plan,{p.x,p.y,66},0x2000,PushMode::Scripted);
            break;
        case 5: move(plan,{-26,p.y,p.z},0x8000,PushMode::Scripted); break;
        case 4: case 6: plan.complete=true; break;
        case 2: plan.blocked=L"The pedestal took the alternate ice-mound route. Use the main-floor reset tile first."; break;
        default: plan.blocked=L"The pedestal is not available on this floor. Use the appropriate reset tile first."; break;
        }
        break;
    case 5: // Djose's five upper controls are native Confirm animations.
        plan.producer=local; plan.entry=2; plan.phase=local;
        plan.complete=s.get(0x0c0000de+local-56)!=0;
        if(!plan.complete) {
            const double yaw[]={-.7,.7,1.5,2.4,-2.6};
            move(plan,{p.x+std::cos(yaw[local-56]),p.y,p.z+std::sin(yaw[local-56])},0,PushMode::Single);
        }
        break;
    case 6: // Zanarkand activates a pattern; this never solves its floor tiles.
        plan.producer=local; plan.entry=2; plan.phase=local;
        plan.complete=s.get(0x0c000021)==static_cast<int>(local-43);
        if(!plan.complete) move(plan,{p.x+(local<46?-1:1),p.y,p.z},0,PushMode::Single);
        break;
    case 7:
        if(p.z<0) {
            plan.producer=39; plan.entry=6;
            move(plan,{p.x,p.y,p.z+23},0x1000,PushMode::Crossing);
        } else {
            plan.phase=1; plan.producer=38; plan.entry=15;
            plan.complete=s.get(0x0c000007)==87 || s.get(0x0c000008)==65;
            if(plan.complete) break;
            if(s.get(0x0c000029)!=1 || (s.get(0x0c00001f)!=1 && s.get(0x0c000020)!=1) || s.get(0x0c000024))
                plan.blocked=L"Power the optional upper-platform recess and finish the current rail ride first.";
            else move(plan,{p.x,p.y,203.59248},0x8000,PushMode::Scripted);
        }
        break;
    default: plan.blocked=L"Pedestal movement rules are unavailable."; break;
    }
    return plan;
}
std::wstring describePedestalPlan(const PedestalPlan& plan,Point position,const std::wstring& action) {
    if(!plan.blocked.empty()) return std::wstring(plan.blocked);
    if(plan.complete) return L"This push is complete.";
    std::vector<std::wstring> parts;
    for(const auto& m:plan.moves) {
        const Point delta{m.goal.x-position.x,0,m.goal.z-position.z};
        const auto bearing=distanceXZ(position,m.goal)>.1?direction(delta):L"toward the wall";
        if(m.mode==PushMode::Single || m.mode==PushMode::Scripted || m.mode==PushMode::Crossing)
            parts.push_back(bearing+L" once");
        else if(distanceXZ(position,m.goal)>.1)
            parts.push_back(bearing+L" "+routeDistanceText(distanceXZ(position,m.goal)/11.13,true)+L" steps");
        position=m.goal;
    }
    if(parts.empty()) return L"The pedestal is in position.";
    const bool single=plan.moves.front().mode==PushMode::Single;
    return L"Push "+join(parts,L", then ")+L". "+(single?L"Press "+action+L" once. ":L"Hold "+action+L" and move in the announced direction. ")+
        L"With Auto walk on, hold "+action+L" to push into position.";
}
std::wstring describePedestalSide(Point pedestal,Point player) {
    const Point offset{player.x-pedestal.x,0,player.z-pedestal.z};
    if(!std::isfinite(offset.x) || !std::isfinite(offset.z) || distanceXZ(pedestal,player)<.1) return {};
    const auto side=direction(offset);
    if(side==L"unknown" || side==L"here") return {};
    return L"You are on the "+side+L" side of the pedestal.";
}
std::wstring describePedestalApproach(const PedestalPlan& plan,Point pedestal,Point player) {
    auto text=describePedestalSide(pedestal,player);
    if(plan.complete || !plan.blocked.empty() || plan.moves.empty() || plan.moves.front().mode==PushMode::Single) return text;
    const auto goal=plan.moves.front().goal;
    const auto side=direction({pedestal.x-goal.x,0,pedestal.z-goal.z});
    if(side!=L"unknown" && side!=L"here") {
        if(!text.empty()) text+=L" ";
        text+=L"Push from the "+side+L" side.";
    }
    return text;
}
}
