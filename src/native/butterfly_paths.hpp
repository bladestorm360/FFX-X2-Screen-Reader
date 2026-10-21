#pragma once
#include "navigation.hpp"
namespace ffx {
// Butterfly zones are live body cylinders: range is the combined actor radius;
// contactMinY/contactMaxY are native conditional vertical-overlap limits.
bool butterflyInTarget(const Entity&,Point);
bool butterflyRouteClear(const std::vector<Point>&,const std::vector<Entity>& red);
std::vector<Point> butterflyRoute(const Mesh&,Point,const Entity& blue,const std::vector<Entity>& red);
}
