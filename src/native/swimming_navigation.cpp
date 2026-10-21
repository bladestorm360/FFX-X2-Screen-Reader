// cdsp0000's type-4 controller is a depth transition, not a collision worker.
// Expose its verified entry6 goal; native controls and ATEL still own movement/warp.
#include "swimming_navigation.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>
namespace ffx {
namespace {
bool active(uintptr_t worker) {
    const auto actor=playerActor(),controller=read<uintptr_t>(address(0xf26ae8));
    const auto chunk=read<uintptr_t>(worker+4);
    if(!actor || !controller || !worker || !chunk || read<uintptr_t>(controller+0x48)!=chunk ||
        read<uint8_t>(read<uintptr_t>(worker))!=4 ||
        read<uint16_t>(address(0xd2ca90))!=71 || !(read<uint8_t>(controller)&2) ||
        (read<uint16_t>(address(0xd2ca90)+0xe)!=19 && read<uint16_t>(address(0xd2ca90)+0xe)!=20) ||
        read<uintptr_t>(address(0xf00740))!=actor || !read<uint16_t>(actor+2) ||
        !std::isfinite(playerPosition().y)) return false;
    // Native 42D200 uses +186 and DiveEnabled. The depth worker keeps
    // looping while the scheduler's current signal/PC changes; it need not
    // be executing its test at the instant the player requests navigation.
    return true;
}
}
bool configureSwimmingGoal(Entity& target,const char* script,unsigned local,uintptr_t worker) {
    if(!script || std::strcmp(script,"cdsp0000") || local!=30 || !active(worker)) return false;
    target.swimDepth=true; target.category=L"Exit"; target.name=L"Exit"; target.id=0xffff;
    target.swimChunk=read<uintptr_t>(worker+4);
    target.swimReady=read<uint8_t>(playerActor()+0x186)==2 && read<uint8_t>(address(swimming_rva::DiveEnabled));
    const auto clearance=read<float>(playerActor()+0x4fc);
    target.swimClearance=std::isfinite(clearance)?std::max(0.0,static_cast<double>(clearance)):0;
    target.position=playerPosition(); target.position.y=251;
    target.distance=std::max(0.0,251-playerPosition().y);
    return true;
}
namespace {
template<class Target> bool goalActive(const Target& target) {
    if(!target.swimDepth || target.worker<0) return false;
    using Worker=uintptr_t(__cdecl*)(int);
    const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(target.worker);
    return read<uintptr_t>(worker+4)==target.swimChunk && active(worker) &&
        read<uint8_t>(playerActor()+0x186)==2 && read<uint8_t>(address(swimming_rva::DiveEnabled));
}
}
bool swimmingGoalActive(const Entity& target) {return goalActive(target);}
bool swimmingGoalActive(const NativeRouteTarget& target) {return goalActive(target);}

void refreshSwimmingGoal(Entity& target) {
    using Worker=uintptr_t(__cdecl*)(int);
    const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(target.worker);
    target.inactive=read<uintptr_t>(worker+4)!=target.swimChunk || !active(worker);
    if(!target.inactive) configureSwimmingGoal(target,"cdsp0000",30,worker);
}
std::wstring swimmingGoalInstruction() {
    if(read<uint8_t>(playerActor()+0x186)!=2 || !read<uint8_t>(address(swimming_rva::DiveEnabled)))
        return L"Dive first to enter the water, then request the route again";
    const auto name=nativeButtonMaskName(0x40);
    auto text=L"Dive first. Hold "+(name.empty()?L"the configured Dive button":name)+L" and follow the route until the map changes";
    const auto controller=read<uintptr_t>(address(0xf26ae8)),chunk=read<uintptr_t>(controller+0x48);
    const auto start=read<unsigned>(chunk+4),end=read<unsigned>(chunk+8);
    if(read<uint16_t>(address(0xd2ca90))==71 && read<uint16_t>(address(0xd2ca90)+0xe)==20 &&
        chunk && start && end>=start+64 && end<0x800000) {
        const auto surface=read<float>(chunk+start+32+16);
        const auto y=playerPosition().y;
        if(std::isfinite(surface) && std::isfinite(y) && surface<250) {
            const auto remaining=static_cast<unsigned>(std::ceil(std::clamp((250-y)/(250-surface),0.0,1.0)*100));
            text+=L". "+std::to_wstring(remaining)+L" percent of the descent remains";
        }
    }
    return text;
}
}
