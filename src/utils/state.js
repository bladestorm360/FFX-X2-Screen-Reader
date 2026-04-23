// Cross-cutting mutable state. Features read/write shared context flags here
// instead of reaching into each other's internal module state.

export const state = {
    sceneId: 0,           // set by scene-activate hook
    inBattle: false,      // maintained by battle tick
    mainMenuLastFrame: 0, // set when MsMenuGetText arg0===9 fires
    rewardsAnnounced: false
};
