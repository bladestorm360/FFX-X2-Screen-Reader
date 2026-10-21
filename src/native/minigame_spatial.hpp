#pragma once
#include "navigation.hpp"
#include <algorithm>
#include <cmath>
namespace ffx {
struct PickupSolution { double ahead=0,lateral=0; float pan=0,proximity=0; bool valid=false,aligned=false; };
inline PickupSolution pickupSolution(Point player,Point target,double angle,double radius,double height) {
    PickupSolution result;
    if(!std::isfinite(player.x) || !std::isfinite(player.y) || !std::isfinite(player.z) ||
       !std::isfinite(target.x) || !std::isfinite(target.y) || !std::isfinite(target.z) ||
       !std::isfinite(angle) || !std::isfinite(radius) || !std::isfinite(height) || radius<=.1 || height<=0) return result;
    const auto x=target.x-player.x,z=target.z-player.z;
    result.ahead=x*std::cos(angle)+z*std::sin(angle);
    result.lateral=x*std::sin(angle)-z*std::cos(angle);
    if(result.ahead < -radius || std::abs(target.y-player.y)>=height) return result;
    result.pan=static_cast<float>(std::clamp(result.lateral/radius,-1.0,1.0));
    result.proximity=static_cast<float>(std::max(.2,1-std::max(0.0,result.ahead)/450));
    result.aligned=std::abs(result.lateral)<radius-.1; result.valid=true; return result;
}
inline bool blitzPlayer(unsigned local) { return (local>=2 && local<=7) || (local>=10 && local<=15); }
inline int blitzTeam(unsigned local) { return local>=2 && local<=7?0:local>=10 && local<=15?1:-1; }
}
