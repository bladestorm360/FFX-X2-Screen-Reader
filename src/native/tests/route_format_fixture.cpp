// Standalone route wording fixture: needs no private game evidence.
#include "../nav_format.hpp"
#include "../camera_direction.hpp"
#include <cstdio>
#include <cstdlib>

namespace ffx {
double distanceXZ(Point a,Point b) {return std::hypot(a.x-b.x,a.z-b.z);}
// Camera faces +x, so +x is north and +z is west, as in the reader fixture.
std::wstring direction(Point delta) {return cameraDirectionLabel(delta,{1,0,0},true);}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring text;for(const auto& part:parts) {if(!text.empty()) text+=separator;text+=part;}return text;
}
}
namespace {
void check(bool value,const char* message) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
}
int main() {
    using namespace ffx;
    const auto bent=routeDirectionsText({{0,0,0},{10,0,10},{15,0,20}},1,true);
    check(bent==L"14.1 northwest, 11.2 northwest","an 18 degree bend in one octant keeps its own segment");
    const auto straight=routeDirectionsText({{0,0,0},{10,0,10},{20,0,20}},1,true);
    check(straight==L"28.3 northwest","collinear subdivisions keep one combined distance");
    // Merged route turns: legs within 15 degrees of the run's overall heading are one turn.
    const auto slight=routeDirectionsText({{0,0,0},{10,0,0},{20,0,1.763}},1,true);
    check(slight==L"20.2 north","a ten degree bend merges into one turn with the walked length");
    const auto zigzag=routeDirectionsText({{0,0,0},{10,0,1},{20,0,0},{30,0,1},{40,0,0}},1,false);
    check(zigzag==L"40 north","small zigzag legs merge into one turn");
    // 18 and 27 degrees sit on both sides of the north/northwest edge at 22.5.
    const auto boundary=routeDirectionsText({{0,0,0},{10,0,3.249},{20,0,8.344}},1,true);
    check(boundary==L"21.7 northwest","legs on both sides of an octant edge merge under the overall heading");
    // Twelve degrees per leg: each leg is compared with the run's overall heading, so
    // drift is caught after a few legs instead of becoming one long misleading straight.
    const auto curve=routeDirectionsText({{0,0,0},{10,0,0},{19.78,0,2.08},{28.92,0,6.15},{37.01,0,12.02},{43.70,0,19.46}},1,false);
    check(curve.find(L", ")!=std::wstring::npos,"a gradual curve still splits instead of drifting into one heading");
    const auto corner=routeDirectionsText({{0,0,0},{10,0,0},{10,0,10}},1,false);
    check(corner==L"10 north, 10 west","a real corner stays two turns");
    const auto tiny=routeDirectionsText({{0,0,0},{10,0,0},{10.001,0,0},{20,0,0}},1,false);
    check(tiny==L"20 north","a negligible leg between straight legs does not split the turn");
    std::puts("PASS: route wording merges near-straight legs into one turn and keeps real corners and curves.");
}
