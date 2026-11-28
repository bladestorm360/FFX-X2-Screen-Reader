#pragma once
#include "navigation.hpp"
#include <cmath>
namespace ffx {
namespace nav {
struct CopiedCamera {Point forward,right;bool valid=false;};
inline bool cameraCut(const CopiedCamera& before,const CopiedCamera& after) noexcept {
    if(!before.valid || !after.valid) return false;
    const auto a=std::hypot(before.forward.x,before.forward.z),b=std::hypot(after.forward.x,after.forward.z);
    if(!std::isfinite(a) || !std::isfinite(b) || a<.001 || b<.001) return false;
    const auto dot=(before.forward.x/a)*(after.forward.x/b)+(before.forward.z/a)*(after.forward.z/b);
    return dot<.7071067811865475244-1e-12;
}
inline thread_local const CopiedCamera* ownedCamera=nullptr;
class CameraScope final {
    CopiedCamera facts;const CopiedCamera* previous;
public:
    CameraScope(Point forward,Point right,bool valid) noexcept :facts{forward,right,valid},previous(ownedCamera) {ownedCamera=&facts;}
    ~CameraScope() noexcept {ownedCamera=previous;}
    CameraScope(const CameraScope&)=delete;CameraScope& operator=(const CameraScope&)=delete;
};
}
// Pure camera-relative octants. Caller owns the copied camera basis; no engine
// getter or string construction belongs to the shared direction policy.
inline const wchar_t* cameraDirectionLabel(Point delta,Point forward,bool valid) {
    if(std::abs(delta.x)<.01 && std::abs(delta.z)<.01) return L"here";
    if(!valid) return L"unknown";
    constexpr double pi=3.14159265358979323846;
    double relative=std::atan2(delta.z,delta.x)-std::atan2(forward.z,forward.x);
    while(relative>pi) relative-=2*pi;
    while(relative< -pi) relative+=2*pi;
    const double degree=relative*180/pi;
    return degree>=-22.5 && degree<22.5?L"north":degree>=22.5 && degree<67.5?L"northwest":
        degree>=67.5 && degree<112.5?L"west":degree>=112.5 && degree<157.5?L"southwest":
        degree>=-67.5 && degree< -22.5?L"northeast":degree>=-112.5 && degree< -67.5?L"east":
        degree>=-157.5 && degree< -112.5?L"southeast":L"south";
}
// Camera-relative clock position plus height, for destination readouts when the
// Clock directions setting is on. 12 o'clock is camera forward, 3 o'clock right.
// FFX world y grows downward, so a target with a smaller y is above the player.
inline std::wstring destinationClockText(Point delta,Point forward,bool valid) {
    constexpr double heightUnits=22.26; // two steps of 11.13 units
    const wchar_t* height=delta.y<=-heightUnits?L", above":delta.y>=heightUnits?L", below":L"";
    std::wstring text;
    if(std::abs(delta.x)<.01 && std::abs(delta.z)<.01) text=L"here";
    else if(!valid) return L"unknown";
    else {
        constexpr double pi=3.14159265358979323846;
        double relative=std::atan2(delta.z,delta.x)-std::atan2(forward.z,forward.x);
        // Positive relative angles are to the left, so clockwise hours count the other way.
        int hour=static_cast<int>(std::lround(-relative*6/pi))%12;
        if(hour<=0) hour+=12;
        text=std::to_wstring(hour)+L" o'clock";
    }
    return text+height;
}
}
