// Offline Frida navigation prototype. Called by discovery/movement events,
// never a timer or a production speech/input producer.
function swimmingDepthGoal(state) {
    return state.verifiedScript === 'cdsp0000' && state.localWorker === 30 &&
        state.entry === 6 && state.pc >= 60444 && state.pc <= 60510 &&
        state.room === 71 && state.controlled && state.enabled && state.fieldControl &&
        state.mode === 2 && state.motion === 2 && state.gravity === 2 && state.diveEnabled &&
        Number.isFinite(state.y);
}
function swimmingDepthRoute(state) {
    if (!swimmingDepthGoal(state) || !Number.isFinite(state.x) || !Number.isFinite(state.z)) return [];
    return [{x: state.x, y: state.y, z: state.z}, {x: state.x, y: 251, z: state.z}];
}
if (typeof module !== 'undefined') module.exports = {swimmingDepthGoal, swimmingDepthRoute};
