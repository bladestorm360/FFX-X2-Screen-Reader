#pragma once
#include "navigation.hpp"
#include <cmath>
namespace ffx {
namespace nav {
struct CopiedCamera {Point forward,right;bool valid=false;};
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
}
