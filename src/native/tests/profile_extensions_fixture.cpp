#include "../profile.hpp"
#include "../events.hpp"
#include "../profile_extensions_data.hpp"
#include "../movie_profile.hpp"
#include "../pedestal_rvas.hpp"
#include "../lightning_rvas.hpp"
#include "../jecht.hpp"
#include "../field_menu_rvas.hpp"
#include "../map_transition.hpp"
#include "../wakka.hpp"
#include "../overdrive.hpp"
#include "../swimming_navigation.hpp"
#include "../jecht_voice.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>
namespace ffx {
bool resolveNativeExtensionsAt(uintptr_t,unsigned);
void log(const char*,...) {}
bool copy(uintptr_t p,void* out,size_t size) {
    __try { memcpy(out,reinterpret_cast<void*>(p),size); return true; }
    __except(1) { return false; }
}
}
using namespace ffx;
void check(bool value,const char* message) { if(!value) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
int main() {
    constexpr unsigned size=0x2400000;
    const auto base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(base!=0,"fixture image");
    std::map<unsigned,unsigned> expected;
    // Keep adjacent native functions in one independently shifted cluster so
    // synthetic relocation cannot overlap their real instruction bytes.
    std::vector<const ExtensionSignature*> ordered;
    for(const auto& spec:extensionSignatures) ordered.push_back(&spec);
    std::sort(ordered.begin(),ordered.end(),[](auto a,auto b){return a->rva<b->rva;});
    unsigned previousEnd=0; int shift=-8;
    for(const auto pointer:ordered) {
        const auto& spec=*pointer;
        if(spec.rva>previousEnd+32) shift=-shift;
        const unsigned actual=spec.rva+shift;
        previousEnd=std::max(previousEnd,spec.rva+spec.size);
        expected[spec.rva]=actual;
        for(unsigned i=0;i<spec.size;++i) reinterpret_cast<uint8_t*>(base+actual)[i]=spec.tokens[i]<0?0:static_cast<uint8_t>(spec.tokens[i]);
    }
    for(const auto& spec:extensionSignatures) if(spec.owner) {
        const int displacement=expected[spec.rva]-(expected[spec.owner]+spec.end);
        memcpy(reinterpret_cast<void*>(base+expected[spec.owner]+spec.offset),&displacement,4);
    }
    for(const auto& operand:extensionOperands) {
        const auto found=expected.find(operand.reference);
        const uintptr_t pointer=base+(found==expected.end()?operand.reference+40:found->second);
        memcpy(reinterpret_cast<void*>(base+expected[operand.owner]+operand.offset),&pointer,4);
        if(found==expected.end()) expected[operand.reference]=operand.reference+40;
    }
    constexpr unsigned collisionOffsets[]={0x41,0x48,0x6a,0x59,0x2e,0x3d};
    for(unsigned i=0;i<6;++i) write<uintptr_t>(base+expected.at(0x43e55c)+i*4,base+expected.at(0x43e4e0)+collisionOffsets[i]);
    for(unsigned group=1;group<=63;++group) write<uint8_t>(base+expected.at(0x43e574)+group-1,
        static_cast<uint8_t>(group==1?0:group==2?1:group==13?2:group==14?3:group>=48?4:5));
    const unsigned getter=0x46af00+80;
    auto putWordGetter=[&](unsigned at,unsigned word) {
        auto bytes=reinterpret_cast<uint8_t*>(base+at); bytes[0]=0x66; bytes[1]=0xa1; bytes[6]=0xc3;
        *reinterpret_cast<uintptr_t*>(bytes+2)=base+word;
    };
    auto putGetterCall=[&](unsigned at,unsigned target) { *reinterpret_cast<int*>(base+at+21)=target-static_cast<int>(at+25); };
    putWordGetter(getter,expected.at(0xf27090));
    putWordGetter(getter+32,expected.at(0xf27080));
    putWordGetter(getter+64,expected.at(0xf27084));
    putGetterCall(expected.at(lightning_rva::Query),getter);
    // Jecht's two remapped practice handlers and raw held handler are
    // distinguished by the original aggregate word getter, not code deltas.
    for(const auto& spec:extensionSignatures) if(spec.rva==0x45e0c0 || spec.rva==0x45e6b0 || spec.rva==0x45eaa0) {
        const auto target=spec.wordOffset==34?getter+32:getter;
        *reinterpret_cast<int*>(base+expected.at(spec.rva)+spec.wordCall+1)=target-static_cast<int>(expected.at(spec.rva)+spec.wordCall+5);
    }
    check(resolveNativeExtensionsAt(base,size),"individually shifted signatures and anchored catalogs");
    check(extendedAddresses==expected,"instruction-derived globals and per-function mappings");
    write<uint8_t>(base+expected.at(0x43e574)+47,5);
    check(!resolveNativeExtensionsAt(base,size),"wrong native gate48 selector refuses startup/preflight");
    write<uint8_t>(base+expected.at(0x43e574)+47,4);
    check(resolveNativeExtensionsAt(base,size),"actual collision selector table restores qualification");
    // The actual supported executables have different fixed FFEscMenu layout
    // bytes. Reject the wrong profile instead of accepting arbitrary layouts.
    nativeProfile=NativeProfile::Installed;
    check(!resolveNativeExtensionsAt(base,size),"reference layout refused by installed profile");
    for(const auto& spec:extensionSignatures) if(spec.installedTokens)
        for(unsigned i=0;i<(spec.installedSize?spec.installedSize:spec.size);++i) if(spec.installedTokens[i]>=0)
            reinterpret_cast<uint8_t*>(base+expected[spec.rva])[i]=static_cast<uint8_t>(spec.installedTokens[i]);
    for(const auto& operand:extensionOperands) if(operand.installedOffset) {
        const uintptr_t pointer=base+expected.at(operand.reference);
        memcpy(reinterpret_cast<void*>(base+expected.at(operand.owner)+operand.installedOffset),&pointer,4);
    }
    *reinterpret_cast<int*>(base+expected.at(0x45e6b0)+54)=getter-static_cast<int>(expected.at(0x45e6b0)+58);
    check(resolveNativeExtensionsAt(base,size),"exact installed layout variant");
    check(extendedAddresses==expected,"installed layout preserves instruction-derived mappings");
    auto* escLayout=reinterpret_cast<uint8_t*>(base+expected.at(0x290130)+103);
    *escLayout=12;check(!resolveNativeExtensionsAt(base,size),"old private Escape fields cannot qualify as Installed update body");
    *escLayout=28;check(resolveNativeExtensionsAt(base,size),"exact Installed Escape update and moved menu operand restore qualification");
    auto* practiceGuard=reinterpret_cast<uint8_t*>(base+expected.at(0x45e6b0)+26);
    *practiceGuard=50;check(!resolveNativeExtensionsAt(base,size),"changed installed practice guard cannot qualify");
    *practiceGuard=49;check(resolveNativeExtensionsAt(base,size),"complete installed practice body restores qualification");
    check(validateNativeDependencies(),"real Installed resolver covers every new pedestal function/global");
    for(const auto rva:jecht_rva::queries) {
        check(profileRva(rva)==expected.at(rva),"Jecht input executor follows its independently qualified original body/getter");
        extendedAddresses.erase(rva);check(!validateNativeDependencies(),"missing Jecht executor refuses startup/preflight");extendedAddresses=expected;
    }
    for(const auto rva:jecht_voice_rva::dependencies) {
        check(profileRva(rva)==expected.at(rva),"Jecht audio observer uses individually qualified code/operands");
        extendedAddresses.erase(rva);check(!validateNativeDependencies(),"missing Jecht audio dependency refuses startup/preflight");extendedAddresses=expected;
    }
    for(const auto rva:{jecht_rva::Show,jecht_rva::Sound}) {
        check(profileRva(rva)==expected.at(rva),"actual Jecht show/sound producer independently qualified");
        extendedAddresses.erase(rva);check(!validateNativeDependencies(),"missing actual Jecht producer refuses startup");extendedAddresses=expected;
    }
    for(const auto rva:{wakka_rva::EffectId,wakka_rva::GlobalBegin,wakka_rva::GlobalTable,0x395560u,0x3955a0u}) {
        check(profileRva(rva)==expected.at(rva),"effect-start and bar getter dependencies are independently matched");
        extendedAddresses.erase(rva);
        check(!validateNativeDependencies(),"missing effect-start or Overdrive getter refuses startup/preflight");
        extendedAddresses=expected;
    }
    for(const auto rva:auron_rva::dependencies) {
        check(profileRva(rva)==expected.at(rva),"Auron initialization and selected array fields are independently matched");
        extendedAddresses.erase(rva);
        check(!validateNativeDependencies(),"missing Auron sequence dependency refuses startup/preflight");
        extendedAddresses=expected;
    }
    check(profileRva(map_transition::Initialize)==expected.at(map_transition::Initialize),"scene initialization follows its independently verified body");
    for(const auto rva:{0x65a270u,0x659800u,0x658ec0u}) {
        check(profileRva(rva)==expected.at(rva),"Grid Status producer follows its own qualified native body");
        extendedAddresses.erase(rva);
        check(!validateNativeDependencies(),"missing Grid Status producer refuses startup/preflight");
        extendedAddresses=expected;
    }
    extendedAddresses.erase(map_transition::Initialize);
    check(!validateNativeDependencies(),"missing map completion dependency refuses startup/preflight");
    extendedAddresses=expected;
    check(profileRva(lightning_rva::Query)==expected.at(lightning_rva::Query),"lightning input query uses its matched native body");
    check(profileRva(map_transition::EnableControl)==expected.at(map_transition::EnableControl),"field control-enable uses its independently matched body");
    extendedAddresses.erase(map_transition::EnableControl);
    check(!validateNativeDependencies(),"missing ActiveField activation hook refuses startup/preflight");
    extendedAddresses=expected;
    for(const auto rva:field_menu_rva::newDependencies) {
        check(profileRva(rva)==expected.at(rva),"field menu handler/state follows independently verified signature/operand");
        extendedAddresses.erase(rva);
        check(!validateNativeDependencies(),"missing field menu dependency refuses startup/preflight");
        extendedAddresses=expected;
    }
    extendedAddresses.erase(lightning_rva::Query);
    check(!validateNativeDependencies(),"missing lightning hook mapping refuses startup");
    extendedAddresses=expected;
    putGetterCall(expected.at(lightning_rva::Query),getter+32);
    check(!resolveNativeExtensionsAt(base,size),"lightning query cannot resolve a held-word getter with the same masked body");
    putGetterCall(expected.at(lightning_rva::Query),getter);
    const auto duplicate=expected.at(lightning_rva::Query)+256;
    memcpy(reinterpret_cast<void*>(base+duplicate),reinterpret_cast<void*>(base+expected.at(lightning_rva::Query)),sizeof(signature_45ef20)/sizeof(int));
    putGetterCall(duplicate,getter+64);
    check(resolveNativeExtensionsAt(base,size),"same-shaped unrelated raw-word handler is excluded by its actual getter operand");
    putGetterCall(duplicate,getter);
    check(!resolveNativeExtensionsAt(base,size),"two qualified pressed-word query bodies are ambiguous and refused");
    memset(reinterpret_cast<void*>(base+duplicate),0,sizeof(signature_45ef20)/sizeof(int));
    check(profileRva(swimming_rva::DiveEnabled)==expected.at(swimming_rva::DiveEnabled),"Dive enable uses its independently resolved full native input function operand");
    extendedAddresses.erase(swimming_rva::DiveEnabled); check(!validateNativeDependencies(),"missing Dive guard refuses startup/preflight"); extendedAddresses=expected;
    for(const auto rva:{0x38fc00u,0x4ddf80u,0x241290u,0x385290u,0x4b1580u,0x4b3810u,0x4b4a00u,0x4b4df0u,0x4b5510u,
        0x1466214u,0x146621cu,0x1466224u,0x1466228u,0x146624au,0x1466250u,0x39fc80u,0x39f680u,0x4999b0u,0xd360b0u,0xd360b4u,0xd360b8u}) {
        check(profileRva(rva)==expected.at(rva),"repair dependency has a separately qualified native mapping");
        extendedAddresses.erase(rva);check(!validateNativeDependencies(),"missing repair dependency refuses startup");extendedAddresses=expected;
    }
    for(const auto rva:pedestal_rva::dependencies) check(profileRva(rva)!=0,"all pedestal hooks and shared inputs have an Installed mapping");
    for(const auto rva:pedestal_rva::newDependencies) {
        check(profileRva(rva)==expected.at(rva),"pedestal dependency follows exact matched function or operand");
        extendedAddresses.erase(rva);
        check(!validateNativeDependencies(),"missing pedestal dependency refuses startup/preflight before hook installation");
        extendedAddresses=expected;
    }
    nativeProfile=NativeProfile::Reference;
    check(!resolveNativeExtensionsAt(base,size),"installed layout refused by reference profile");
    nativeProfile=NativeProfile::Installed;
    const auto& first=extensionSignatures[0];
    memcpy(reinterpret_cast<void*>(base+expected[first.rva]+512),reinterpret_cast<void*>(base+expected[first.rva]),first.size);
    check(!resolveNativeExtensionsAt(base,size),"ambiguous signature refuses hooks");
    check(extendedAddresses==expected,"failed discovery cannot publish partial mappings");
    memset(reinterpret_cast<void*>(base+expected[first.rva]+512),0,first.size);
    const auto& operand=extensionOperands[0];
    uintptr_t invalid=base+size+100;
    memcpy(reinterpret_cast<void*>(base+expected[operand.owner]+operand.offset),&invalid,4);
    check(!resolveNativeExtensionsAt(base,size),"out-of-image data operand refused");
    // Will owns only its independently matched caption/audio subset. It must
    // not require or publish any of FFX's gameplay hooks or data addresses.
    memset(reinterpret_cast<void*>(base),0,size); expected.clear();
    nativeProfile=NativeProfile::MovieCaptions;
    for(const auto& entry:movieAddresses) expected[entry.reference]=entry.installed;
    expected[0x548cf4]=0x53ead4; expected[0x67c90a]=0x673f0a;
    for(const auto& spec:extensionSignatures) if(expected.count(spec.rva))
        for(unsigned i=0;i<spec.size;++i)
            reinterpret_cast<uint8_t*>(base+expected[spec.rva])[i]=spec.tokens[i]<0?0:static_cast<uint8_t>(spec.tokens[i]);
    for(const auto& spec:extensionSignatures) if(spec.owner && expected.count(spec.rva)) {
        const int displacement=expected[spec.rva]-(expected[spec.owner]+spec.end);
        memcpy(reinterpret_cast<void*>(base+expected[spec.owner]+spec.offset),&displacement,4);
    }
    for(const auto& movieOperand:extensionOperands) if(expected.count(movieOperand.owner)) {
        if(!expected.count(movieOperand.reference)) expected[movieOperand.reference]=movieOperand.reference+40;
        const uintptr_t pointer=base+expected[movieOperand.reference];
        memcpy(reinterpret_cast<void*>(base+expected[movieOperand.owner]+movieOperand.offset),&pointer,4);
    }
    check(resolveNativeExtensionsAt(base,size),"independent Will caption profile");
    check(extendedAddresses==expected,"Will publishes only caption/audio mappings");
    check(profileRva(0x505d30)==0x4fbb00,"Will renderer uses its own verified mapping");
    bool refused=false; try { profileRva(0x4b8980); } catch(const std::out_of_range&) { refused=true; }
    check(refused,"Will cannot fall back to an FFX gameplay RVA");
    VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);
    puts("PASS: native extension signatures, instruction-derived globals, anchored catalogs, ambiguity and transactional refusal.");
}
