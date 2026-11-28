#include "../text_plan.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include "test_copied_controls.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
namespace ffx {
bool copy(uintptr_t p,void* out,size_t n) {std::memcpy(out,reinterpret_cast<void*>(p),n);return true;}
unsigned generation() {return 1;}
int setting(Setting s) {return s==Setting::Speech?1:0;}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring result;for(auto& part:parts) {if(!result.empty()) result+=separator;result+=part;}return result;
}
bool output(const std::wstring&,bool,const char*) {return true;}
bool outputDialogue(const std::wstring&,bool,const char*,bool) {return true;}
bool hook(unsigned,Handler,Handler) {return false;}
void log(const char*,...) {}
void emitSelectionDetailOwned(std::wstring,SelectionDetailToken,bool,const speechq::Header&) {}
void appendSelectionDetailOwned(std::wstring,SelectionDetailToken) {}
std::wstring menuIndex(unsigned,unsigned) {return {};}
}
#include "test_speech_domains.hpp"
namespace ffx::speechq { void notifyPreparation() noexcept {} }
namespace {
void check(bool value,const char* text) {if(!value) {std::printf("FAIL: %s\n",text);std::exit(1);}}
// Same plan shape as battle.cpp targets(): HP row joined with copied status words.
std::wstring target(uint16_t permanent,uint16_t temporal,uint16_t extra,unsigned doom) {
    ffx::TextPlan p;ffx::prepareTextPlan(p,"battle-target");
    auto row=ffx::planBattleTarget(p,ffx::planLiteral(p,L"Kimahri"),340,340,1);
    if(const auto ailments=ffx::planBattleAilments(p,permanent,temporal,extra,doom)) row=ffx::planJoin(p,row,ailments);
    return ffx::renderTextPlan(p,row,{});
}
}
int main() {
    check(target(0,0,0,0)==L"Kimahri: HP 340/340","no ailment adds no suffix");
    check(target(1u<<3,0,0,0)==L"Kimahri: HP 340/340, poison","poison follows HP");
    check(target(1u<<3|1u<<5,1u<<2|1u<<12,0,0)==L"Kimahri: HP 340/340, poison, magic break, darkness, slow","table order");
    check(target(0,1u<<3|1u<<4|1u<<11,1u<<0|1u<<9,0)==L"Kimahri: HP 340/340","shell, protect, haste, scan and auto-life are not ailments");
    check(target(0,0,1u<<10|1u<<14,2)==L"Kimahri: HP 340/340, curse, doom, 2 turns","curse and doom counter");
    ffx::TextPlan none;ffx::prepareTextPlan(none,"battle-target");
    check(!ffx::planBattleAilments(none,0,1u<<11,0,0) && none.count==0,"no ailment copies no node");
    std::printf("PASS: battle target ailment names, order, positive-status exclusion and Doom counter.\n");
}
