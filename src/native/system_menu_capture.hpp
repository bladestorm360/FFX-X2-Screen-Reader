#pragma once
#include "copied_controls.hpp"
#include "selection_details.hpp"
#include "ready_text.hpp"
namespace ffx {
enum class SystemMenuKind {SlotPopulation,SlotFocus,Popup,SaveHelp,YesNo,EscHelp,Config,Reset};
using ReadySystemText=ReadyBytes<1024>;
struct SystemMenuPacket : CopiedControls {
    std::array<ReadySystemText,3> text{};
    SelectionDetailToken detail;
    SystemMenuKind kind=SystemMenuKind::Reset;
    unsigned epoch=0,serial=0,index=0;
    bool interrupt=true,automatic=false,joinLabel=false;
};
void prepareSystemMenuPacket(SystemMenuPacket&,SystemMenuKind,bool encoded=false);
// Ready byte copying only. Native getters/resolvers stay at their call sites.
void copySystemAscii(uintptr_t,ReadySystemText&,unsigned limit=256);
void copySystemEncoded(uintptr_t,ReadySystemText&,unsigned limit=256);
bool commitSystemMenuPacket(const SystemMenuPacket&);
void resetSystemMenuCapture();
void installSystemMenuCapture();
}
