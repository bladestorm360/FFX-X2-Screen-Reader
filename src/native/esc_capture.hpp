#pragma once
#include "events.hpp"
#include "ready_text.hpp"
#include "selection_details.hpp"
#include "game_bindings.hpp"
namespace ffx {
inline constexpr unsigned escVideoFields[]={1,2,9,10,13,11,12,14,15,16,17,18};
inline constexpr unsigned escVideoNames[]={0x231,0x233,0x238,0x23a,0x249,0x23e,0x244,0x24e,0x254,0x258,0x260,0x264};
enum class EscCaptureKind {Quit,Popup,QuitFocus,BindingFocus,BindingChanges,DetailOnly,SettingFocus,AudioChanged,VideoChanged,LabelFocus,SpecialFocus,SpecialControls,SpecialChanged};
struct EscBindingText {ReadyBytes<512> label;ReadyBytes<64> first,second;};
struct EscPacket {
    std::array<ReadyBytes<512>,3> text{};
    std::array<EscBindingText,23> bindings{};
    SelectionDetailToken detail;
    EscCaptureKind kind=EscCaptureKind::DetailOnly;
    unsigned epoch=0,serial=0,index=0,value=0,count=0;
    bool pad=false,video=false,automatic=false;
};
void prepareEscPacket(EscPacket&,EscCaptureKind);
void copyEscLocalized(unsigned,ReadyBytes<512>&,unsigned limit=512); // Native resolver, ready copy only.
void copyEscKey(unsigned action,bool pad,ReadyBytes<64>&);
void copyEscBinding(const Binding&,bool pad,EscBindingText&,unsigned selected=2);
void copyEscVideo(unsigned index,unsigned value,ReadyBytes<512>&); // Resolve only a displayed option label.
bool commitEscPacket(const EscPacket&);
void installEscCapture();
void resetEscCapture();
}
