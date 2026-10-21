// Owns bounded native-return command dispatch and context/focus retirement; ordinary pass-through stays separate. See Docs/features/runtime/index.md#input-and-settings and Docs/direct_input_ownership_20261012.md.
#include "input_device.hpp"
#include "events.hpp"
#include "utilities.hpp"
#include "mod_menu.hpp"
#include "puzzle_solver.hpp"
#include "controller.hpp"
#include "settings.hpp"
#include "nav_audio.hpp"
#include "minigames.hpp"
#include "blitz_review.hpp"
#include "field_controls.hpp"
#include "battle_sensor.hpp"
#include "overdrive.hpp"
#include "wakka.hpp"
#include "auto_walk.hpp"
#include "pedestal.hpp"
#include "lightning.hpp"
#include "esc_details.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>

namespace ffx {
namespace {
struct Key { unsigned vk; bool shift,control; unsigned epoch,revision; bool menu,activity,escape; void(*scoped)(bool)=nullptr; bool(*guard)()=nullptr; };
struct Binding {void(*handler)(bool)=nullptr;bool(*available)()=nullptr;bool exclusive=false;};
// Installed on bootstrap before enabling input hooks; read-only afterward.
std::array<Binding,256> bindings;
std::array<std::array<Binding,2>,256> chords;
const Binding* chord(unsigned vk,bool shift) {
    if(vk>=chords.size()) return nullptr;
    const auto& binding=chords[vk][shift?1:0];
    return binding.handler && binding.available && binding.available()? &binding:nullptr;
}
async::TryCaptureQueue<Key,64> queue;
std::atomic<unsigned> inputRevision{1};
std::array<bool,256> swallowedMessages{};
std::atomic<bool> keyboard{false};
thread_local unsigned messageDepth = 0;
void messageEnter(Event& e) { e.before[0] = ++messageDepth; }
void received(Event& e) {
    if (messageDepth) --messageDepth;
    if(e.before[0]>1 || !nativeGameThread()) return;
    if (!e.result || e.result == static_cast<uintptr_t>(-1) ||
        (e.rva < 0xffff0002 && !(e.args[4] & PM_REMOVE))) return;
    MSG message{};
    if (!copy(e.args[0], &message, sizeof(message))) return;
    const bool down=message.message==WM_KEYDOWN || message.message==WM_SYSKEYDOWN;
    const bool up=message.message==WM_KEYUP || message.message==WM_SYSKEYUP;
    if(!down && !up) return;
    if(down && !(message.lParam & (1u<<30)) && gameFocused()) observeInputDevice(false);
    const unsigned vk = static_cast<unsigned>(message.wParam);
    if(vk>=swallowedMessages.size()) return;
    if(!gameFocused() || utilityConsoleActive()) {
        // Opening the console changes focus before F1 is necessarily released.
        // Retain each captured key's owner through repeat/release; do not queue
        // new commands or let an already captured console chord become turbo.
        const bool captured=swallowedMessages[vk];
        if(up) swallowedMessages[vk]=false;
        if(captured) {message.message=WM_NULL;write<MSG>(e.args[0],message);}
        return;
    }
    // Consume repeats and releases as well as presses. Queuing a mod action
    // is still edge-only and DirectInput remains the producer when available.
    const bool repeat=down && (message.lParam & (1u<<30));
    const bool shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
    const bool control=(GetKeyState(VK_CONTROL)&0x8000)!=0;
    bool captured=swallowedMessages[vk] || capturesKey(vk,shift,control);
    if(down && !repeat) captured=keyboard?capturesKey(vk,shift,control):consumeKey(vk,shift,control);
    swallowedMessages[vk]=down && captured;
    if(captured) {
        message.message=WM_NULL; write<MSG>(e.args[0],message);
    }
}
void drainKeys() {
    if(!nativeGameThread()) return;
    Key event;
    // One bounded FIFO drain; publications and availability handback never wait.
    for(unsigned count=0;count<64 && queue.tryPop(event);++count)
      if(event.epoch==generation() && event.revision==inputRevision.load(std::memory_order_acquire) && gameFocused() && event.escape==escMenuActive() && (!event.guard || event.guard())) {
        if(event.menu) { if(modMenuActive()) modMenuKey(event.vk,event.shift); }
        else if(!modMenuActive() && !utilityConsoleActive()) {
            if(event.activity) dispatchMinigameKey(event.vk,event.shift);
            else if(event.scoped) {
                const auto binding=chord(event.vk,event.shift);
                if(binding && binding->handler==event.scoped) binding->handler(event.shift);
            } else dispatchHotkey(event.vk,event.shift,event.control);
        }
    }
}
void inputEvent(Event&) {
    if(!nativeGameThread()) return;
    updateUtilityContext();
    if(!gameFocused() || utilityConsoleActive() || !inBattle() || scene) resetWakka();
    if(!gameFocused() || utilityConsoleActive() || modMenuActive() || !inBattle() || scene) resetOverdrive();
    if(!gameFocused() || utilityConsoleActive() || modMenuActive() || inBattle() || scene) {
        pauseNavAudio(); cancelLightningWarning();
    }
    drainKeys();
    pumpUtilities();
    updateAutoWalk();
    updatePedestalInput();
    if (!utilityConsoleActive() && !modMenuActive()) pumpPuzzleSolver();
}
void inputBefore(Event&) { if(observeGameThread()) { updateUtilityContext(); pumpController(); } }
void inputAfter(Event&) {
    // Dispatch copied commands at their native input return, without running
    // field movement/utility pumps in a menu update.
    if(nativeGameThread()) drainKeys();
}
}
void dispatchHotkey(unsigned vk,bool shift,bool control) {
    if(dispatchBlitzKey(vk,control)) return;
    if(dispatchFieldControlsKey(vk,control)) return;
    if(dispatchBattleSensorKey(vk,control)) return;
    if(dispatchMinigameKey(vk,shift)) return;
    if(const auto binding=chord(vk,shift)) { binding->handler(shift); return; }
    if(vk>=bindings.size()) return;
    const auto& binding=bindings[vk];
    if(binding.handler && (!binding.available || binding.available())) binding.handler(shift);
}
bool queueControllerHotkey(unsigned vk,bool shift,bool control,bool(*guard)()) {
    // Route controller commands through the keyboard native-return phase; refuse busy or stale context.
    if(!nativeGameThread() || !vk || !gameFocused() || utilityConsoleActive() || modMenuActive() || !guard || !guard()) return false;
    // Do not call consumeKey: that would misclassify this controller event as
    // keyboard input. Keep physical passthrough and native input sampling immediate.
    const bool accepted=queue.tryPush({vk,shift,control,generation(),inputRevision.load(std::memory_order_acquire),false,false,escMenuActive(),nullptr,guard});
    if(!accepted) output(L"Input is busy. Try the command again.",true,"input-pressure");
    return accepted;
}
void hotkey(unsigned vk,void(*handler)(bool)) {if(vk<bindings.size()) bindings[vk]={handler,nullptr};}
void hotkeyWhen(unsigned vk,void(*handler)(bool),bool(*available)()) {if(vk<bindings.size()) bindings[vk]={handler,available};}
void hotkeyChordWhen(unsigned vk,bool shift,void(*handler)(bool),bool(*available)(),bool exclusive) {if(vk<chords.size()) chords[vk][shift?1:0]={handler,available,exclusive};}
void directKeyboard(bool active) {
    keyboard.store(active,std::memory_order_release);
    inputRevision.fetch_add(1,std::memory_order_acq_rel);
}
void queueKey(unsigned vk, bool shift) {
    consumeKey(vk,shift);
}
bool consumeKey(unsigned vk,bool shift,bool control) {
    if(!nativeGameThread() || !vk || !gameFocused() || utilityConsoleActive()) return false;
    observeInputDevice(false);
    autoWalkPlayerInput(vk);
    const bool activity=minigameCapturesKey(vk);
    const bool captured=activity || capturesKey(vk,shift,control);
    // Unbound function keys belong to the game/screen reader, including NVDA
    // history on F12. Do not cancel the addon speech they can initiate.
    if(captured || vk<VK_F1 || vk>VK_F24) interruptSpeechForInput();
    if(!captured) return false;
    const bool menu=modMenuActive();
    const auto registeredChord=chord(vk,shift);
    const auto scoped=!menu && !activity && !blitzCapturesKey(vk,control) && !fieldControlsCapturesKey(vk,control) &&
        !battleSensorCapturesKey(vk,control)?registeredChord:nullptr;
    const bool accepted=queue.tryPush({vk,shift,control,generation(),inputRevision.load(std::memory_order_acquire),menu,activity,escMenuActive(),scoped?scoped->handler:nullptr});
    // No retry/wait on native input. A refused exclusive command is dropped,
    // while its physical press/repeat/release stays ours instead of native F1.
    return accepted || (registeredChord && registeredChord->exclusive);
}
bool capturesKey(unsigned vk,bool shift,bool control) {
    if(!nativeGameThread() || !gameFocused() || utilityConsoleActive()) return false;
    if(modMenuActive()) return true;
    if(blitzCapturesKey(vk,control)) return true;
    if(fieldControlsCapturesKey(vk,control)) return true;
    if(battleSensorCapturesKey(vk,control)) return true;
    if(minigameCapturesKey(vk)) return true;
    if(chord(vk,shift)) return true;
    if(vk>=bindings.size()) return false;
    const auto& binding=bindings[vk];
    return binding.handler && (!binding.available || binding.available());
}
void installInput() {
    const auto user = GetModuleHandleW(L"user32.dll");
    const char* names[] = {"PeekMessageA", "PeekMessageW", "GetMessageA", "GetMessageW"};
    for (unsigned i = 0; i < 4; ++i) hookAddress(reinterpret_cast<uintptr_t>(GetProcAddress(user, names[i])), 0xffff0000 + i, messageEnter, received);
    // PInput::Update also runs while the save/load UI bypasses the game's
    // field-input aggregator. Refresh SDL before native device reads there.
    hook(0x225930,inputBefore,inputAfter);
    // Help 76F580 reads its own menu input and always calls 776220, including
    // when no native arrow/Confirm is pressed. PInput/field returns alone do
    // not deliver the captured I/F7/review commands on information pages.
    hook(0x376220,nullptr,inputAfter);
    hook(0x471d10,nullptr,inputEvent);
    installDirectInputKeyboard();
    installController();
}
}
