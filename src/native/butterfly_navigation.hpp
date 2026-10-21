#pragma once
#include "navigation.hpp"
namespace ffx {
std::vector<Point> navigationRoute(const Mesh&,Point,const Entity&);
std::vector<Point> navigationRouteOwned(const Mesh&,Point,const Entity&,const std::vector<Entity>& liveTargets);
bool navigationTargetReached(const Entity&,Point);
}
