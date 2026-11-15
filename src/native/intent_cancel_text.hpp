#pragma once
// Spoken outcome of a navigation action whose native checks failed. Kept
// separate from navigation_intents.cpp so the wording is testable alone.
namespace ffx::nav {
// Changed: the player, input, destination or route moved on since the request.
// Message: a message box, dialogue or scripted scene holds the field.
// Context: focus, a menu, the console or a battle; the player caused it.
enum class IntentRejection : unsigned {Changed,Message,Context};
inline const wchar_t* intentCancelText(IntentRejection reason) {
    switch(reason) {
    case IntentRejection::Message: return L"Navigation action skipped while a message or scene is open. Request the route again after it closes.";
    case IntentRejection::Context: return nullptr;
    default: return L"Navigation action canceled because the player, input or destination changed. Request the route again.";
    }
}
// The field capture falls back to the first actor-table entry when the
// controlled-actor pointer is not inside the table. The route can still be read
// from that entry, but the native start requires the controlled actor, so an
// action from such an observation could never run.
inline const wchar_t* intentRefusalText(bool controlled);
// Checked before a route is read, so a refused walk is not preceded by route speech.
// fieldOpen: no window, scene, battle or menu; control: the field controller allows
// player input. Scenes clear field control and the controlled actor together.
inline const wchar_t* routeStartRefusal(bool autoWalk,bool fieldOpen,bool control,bool controlled) {
    if(!autoWalk) return nullptr;
    if(!fieldOpen || !control) return L"Auto walk unavailable during a scene.";
    return intentRefusalText(controlled);
}
inline const wchar_t* intentRefusalText(bool controlled) {
    return controlled?nullptr:L"Auto walk unavailable: the game is not reporting the controlled character. Walk manually or try again after moving.";
}
}
