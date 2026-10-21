#include "navigation.hpp"
#include <cmath>
namespace ffx {
namespace {
template<class Target> bool faceTarget(const Target& target) {
    // ChSetRotY (82B230) updates both facing and movement direction. The
    // angle matches ChGetDir2Point (88B3A0): atan2(delta Z, delta X).
    const auto actor=playerActor();
    const auto player=playerPosition();
    if(!actor || actor!=read<uintptr_t>(address(0xf00740)) || !read<uint16_t>(actor+2) ||
        target.inactive || !target.zone.valid || target.zone.crossing || !inInteractionZone(target.zone,player)) return false;
    const auto center=target.pushControl?target.position:target.zone.center;
    const double x=center.x-player.x,z=center.z-player.z;
    if(!std::isfinite(x) || !std::isfinite(z) || std::hypot(x,z)<.001) return false;
    using Face=void(__cdecl*)(uintptr_t,float);
    reinterpret_cast<Face>(address(0x42b230))(actor,static_cast<float>(std::atan2(z,x)));
    return true;
}
}
bool faceNavigationTarget(const Entity& target) {return faceTarget(target);}
bool faceNavigationTarget(const NativeRouteTarget& target) {return faceTarget(target);}
}
