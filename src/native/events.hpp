#pragma once
#include <atomic>
#include "runtime.hpp"
#include <array>
#include <functional>
#include "native_action.hpp"

namespace ffx {
// Copied call arguments and reader-local state. No borrowed game pointer is
// retained beyond the corresponding native call's return.
struct Event {
    unsigned rva = 0;
    unsigned parentRva = 0;
    std::array<uintptr_t, 12> args{};
    uintptr_t ecx = 0, result = 0, caller = 0;
    uintptr_t edi = 0, nativeStack = 0; // Ready registers/opaque stack identity for qualified resource receipts.
    std::array<intptr_t, 32> before{};
    std::array<float,8> values{};
    std::array<unsigned,64> copied{}; // Bounded native configuration/party before-values.
    unsigned copiedCount=0;
    unsigned epoch = 0;
    bool overrideResult = false;
    bool captureOnly = false;
};
using Handler = void(*)(Event&);
bool hook(unsigned rva, Handler enter, Handler leave);
// A reviewed capture-only listener copies ready data/POD on the originating
// native owner; it never touches shared reader containers or waits.
bool hookSnapshot(unsigned rva,Handler enter,Handler leave);
bool hookAddress(uintptr_t target, unsigned id, Handler enter, Handler leave);
bool enableHooks();
bool applyAddedHooks();
void discardHooks();
// Native mutations run only at the outermost return boundary on the verified
// PInput owner. Prepare/complete serialize reader state; invoke holds no reader
// lock and permits the ordinary nested native hooks to run.
bool deferNativeAction(NativeAction);
bool observeGameThread();
bool nativeGameThread();
// One nonblocking attempt. Shared COM adapters forward native input unchanged
// while resource loading/reset owns reader state. No original call is locked.
bool nativeReaderResourcesReady() noexcept;
bool tryNativeReaderAccess() noexcept;
void releaseNativeReaderAccess() noexcept;
class NativeReaderAccess final {
    bool owned_;
public:
    NativeReaderAccess() noexcept : owned_(tryNativeReaderAccess()) {}
    ~NativeReaderAccess() { if(owned_) releaseNativeReaderAccess(); }
    NativeReaderAccess(const NativeReaderAccess&)=delete;
    NativeReaderAccess& operator=(const NativeReaderAccess&)=delete;
    explicit operator bool() const noexcept { return owned_; }
};
void installFieldMenus();
void installMessages();
void installSystemMenus();
void installSphereGrid();
void installBattle();
void installNavigation();
void installInput();
void installDirectInputKeyboard();
void observeDirectInput(uintptr_t object);
void directKeyboard(bool active);
void queueKey(unsigned vk, bool shift);
void hotkey(unsigned vk, void(*callback)(bool shift));
void hotkeyWhen(unsigned vk, void(*callback)(bool shift), bool(*available)());
// Exclusive chords retain input ownership if the bounded command queue is full.
// Opt in only when native fallthrough would trigger a conflicting game action.
void hotkeyChordWhen(unsigned vk, bool shift, void(*callback)(bool), bool(*available)(), bool exclusive=false);
void resetNavigation();
void resetBattle();
void resetMessages();
void resetDialogueReview();
void installGameContext();
void resetSystemMenus();
void resetSphereGrid();
extern std::atomic<int> scene;
std::wstring first(uintptr_t source, size_t length = 256);
std::wstring join(const std::vector<std::wstring>& parts, const wchar_t* separator = L", ");
std::wstring ascii(uintptr_t source, size_t length = 256);
std::wstring clean(std::wstring value);
std::wstring menuText(int space, unsigned id);
std::wstring menuDescription(int space, unsigned id);
std::wstring textDescription(uintptr_t source);
std::wstring recordName(unsigned getter, unsigned id);
std::wstring nativeText(unsigned getter, unsigned id);
std::wstring recordDescription(unsigned getter, unsigned id);
std::wstring equipmentName(uintptr_t record);
std::wstring equipmentDetails(uintptr_t record);
std::wstring equipmentDescription(uintptr_t record);
std::wstring characterDescription(unsigned id);
std::wstring battleSelectionVitals(unsigned command);
bool readBattleSummonDetail();
std::wstring hpReadout(int current,int maximum);
std::wstring battleMpReadout(int current,int maximum);
std::wstring shopEquipmentDescription(uintptr_t record);
void selectionDetail(const std::wstring& text, bool automatic);
void appendSelectionDetail(const std::wstring& text);
bool readSelectionDetail();
void resetSelectionDetail();
unsigned quantity(unsigned id);
std::wstring battleName(unsigned id);
bool inBattle();
template<class T> bool write(uintptr_t target, T value) {
    __try { *reinterpret_cast<T*>(target) = value; return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
