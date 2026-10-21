#pragma once
#include "message_text.hpp"
namespace ffx {
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
