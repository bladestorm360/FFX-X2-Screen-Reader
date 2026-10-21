#pragma once
#include "navigation.hpp"
namespace ffx {
namespace nav {struct Scene;}
// Preparation owner consumes copied scenes; the existing compute owner still
// publishes complete result prefixes. No engine getter is used by this path.
void requestOwnedReachability(const nav::Scene&,std::vector<Entity>&,bool preserve=false);
bool applyOwnedReachability(const nav::Scene&,std::vector<Entity>&);
bool ownedReachabilityChecking();
void retireOwnedReachability();
using ReachabilityPrepared=void(*)() noexcept;
void onReachabilityPrepared(ReachabilityPrepared);
// Native owner retains this mod-owned vector. Worker receives only POD copies.
void requestReachability(std::vector<Entity>&,Point,bool preserve=false);
// Called after the native collision commit; do not capture a pre-physics pose.
void refreshReachabilityAfterMovement(std::vector<Entity>&);
bool reachabilityContextValid();
bool reachabilityChecking();
bool sameReachabilityTarget(const Entity&,const Entity&);
void resetReachability();
void navigationReachabilityChanged();
void installReachability();
#ifdef FFX_READER_TEST
void drainReachabilityForTest();
void advanceReachabilityForTest();
bool reachabilityCompleteForTest();
#endif
}
