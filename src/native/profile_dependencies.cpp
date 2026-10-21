// Resolve the explicitly listed feature dependencies before enabling the hook
// batch. Hook/extension signature validation is separate from this RVA coverage.
// See ../../Docs/features/runtime/index.md.
#include "profile.hpp"
#include "pedestal_rvas.hpp"
#include "lightning_rvas.hpp"
#include "jecht.hpp"
#include "jecht_voice.hpp"
#include "field_menu_rvas.hpp"
#include "butterfly_rvas.hpp"
#include "map_transition.hpp"
#include "overdrive.hpp"
#include "swimming_navigation.hpp"
#include "wakka.hpp"
namespace ffx {
bool validateNativeDependencies() {
    if(nativeProfile==NativeProfile::MovieCaptions) return true;
    try {
        for(const auto rva:{0x241290u,0x385290u,0x4b1580u,0x4b3810u,0x4b4a00u,0x4b4df0u,0x4b5510u,
            0x1466214u,0x146621cu,0x1466224u,0x1466228u,0x146624au,0x1466250u}) profileRva(rva);
        for(const auto rva:{0x39fc80u,0x39f680u,0x4999b0u,0xd360b0u,0xd360b4u,0xd360b8u}) profileRva(rva);
        for(const auto rva:{0x48d230u,0x2871c0u,0x46ca90u,0x48d490u,0xf01a54u}) profileRva(rva);
        profileRva(0x38fc00);
        profileRva(0x465390);
        profileRva(0x46c6c0);
        for(const auto rva:{0x51a830u,0x51ab30u,0x51a7c0u,0x51a930u,0x153bf80u,0xf30804u}) profileRva(rva);
        for(const auto rva:{0x39af20u,0x505890u,0x4fb940u,0xf3d742u})profileRva(rva);
        for(const auto rva:{0x49b2d0u,0x4a0520u,0x4a1660u,0x49e5d0u,0x503bb0u,0x4e5a20u,0x4963c0u,0x496050u,0x4e58e0u,0x4f4b20u,0xf3f76cu,0x21d0aa4u})profileRva(rva);
        profileRva(0x4ddf80);
        profileRva(map_transition::Initialize);
        profileRva(map_transition::EnableControl);
        for(const auto rva:{wakka_rva::EffectId,wakka_rva::GlobalBegin,wakka_rva::GlobalTable,0x395560u,0x3955a0u}) profileRva(rva);
        profileRva(swimming_rva::DiveEnabled);
        for(const auto rva:{0x65a270u,0x659800u,0x658ec0u}) profileRva(rva);
        for(const auto rva:{0x241870u,0x257b60u,0x260580u}) profileRva(rva);
        for(const auto rva:{0x2e96d0u,0x2e99b0u,0x2e9840u}) profileRva(rva);
        profileRva(0x4e52e0);
        profileRva(0x374630);
        for(const auto rva:{0x4e75c0u,0x4e52b0u,0x4ee210u}) profileRva(rva);
        profileRva(0x390850);
        profileRva(0x4d2b10);profileRva(0x146a9e8);
        for(const auto rva:{0x4ba620u,0x1469ec8u,0x1469ed4u,0x1469ed8u,0x1469ee6u,0x302740u,0x83a2bcu,0x290130u,0x8cc868u}) profileRva(rva);
        for(const auto rva:{0x28fb60u,0x628ef0u,0x459a80u,0x43e55cu,0x43e574u}) profileRva(rva);
        profileRva(0x2e88b0);
        for(const auto rva:{0x4a3250u,0xf40424u,0xf40418u,0x28df70u})profileRva(rva);
        profileRva(0x385330);
        profileRva(0xd34568);
        for(const auto rva:auron_rva::dependencies) profileRva(rva);
        for(const auto rva:{0x3810f0u,0x38e680u}) profileRva(rva);
        for(const auto rva:{0x38fa30u,0x38f9e0u,0x38f810u,0x4913f0u,0x5da370u,0xf3c8feu,0xf3d1b4u,0xf3d6a0u,0xf3f73cu}) profileRva(rva);
        for(const auto rva:pedestal_rva::dependencies) profileRva(rva);
        for(const auto rva:lightning_rva::dependencies) profileRva(rva);
        for(const auto rva:jecht_rva::queries) profileRva(rva);
        profileRva(jecht_rva::Show);profileRva(jecht_rva::Sound);
        for(const auto rva:jecht_voice_rva::dependencies) profileRva(rva);
        for(const auto rva:field_menu_rva::dependencies) profileRva(rva);
        for(const auto rva:butterfly_rva::dependencies) profileRva(rva);
        return true;
    } catch(const std::exception& error) {
        recordLog("native dependency refused: %s",error.what()); return false;
    }
}
}
