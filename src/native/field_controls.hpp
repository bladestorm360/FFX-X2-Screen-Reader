#pragma once
#include "message_text.hpp"
namespace ffx {
// Native owner only: the same dialogue and controlled-actor facts guard field assistance.
inline const wchar_t* nativeFieldControlReason() {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    if(!controller || !(read<uint8_t>(controller)&2) || !read<uintptr_t>(address(0xf00740))) return L"player control disabled";
    using Mode=int(__cdecl*)();
    return reinterpret_cast<Mode>(address(0x46c840))()!=0?L"dialogue":nullptr;
}
struct FieldControlsSnapshot {
    unsigned epoch=0,revision=0,slot=8,serial=0;
    uintptr_t window=0,source=0;
    bool candidate=false;
};
FieldControlsSnapshot captureFieldControls(unsigned slot,uintptr_t source);
bool observeFieldControlsOwned(const MessagePage&,const FieldControlsSnapshot&);
void installFieldControls();
void clearDialogueReviewOwned(); // Preparation owner only; no native view mutation.
enum class FieldControlsClassification {Pending,Nonblocking,Blocking};
FieldControlsSnapshot fieldControlsForSource(unsigned slot,uintptr_t source);
FieldControlsClassification classifyFieldControls(const FieldControlsSnapshot&);
void invalidateFieldControlsSlot(unsigned slot);
void rejectFieldControlsCapture(const FieldControlsSnapshot&);
void resetFieldControls();
bool fieldControlsCapturesKey(unsigned vk, bool control);
bool dispatchFieldControlsKey(unsigned vk, bool control);
}
