// Pure Frida prototype, evaluated offline. No hooks or production producer.
function plannerStep(route, cursor, backward, fallback) {
    const index = route.indexOf(cursor);
    if (index >= 0) return route[index + (backward ? -1 : 1)] ?? -1;
    return fallback[1] ?? -1;
}
function battleNumberKind(kind) { return kind === 0 ? 'HP' : kind === 1 ? 'MP' : ''; }
function controllerCommand(control, mod, battle, menu) {
    if (mod) return control === 'Start' ? 'settings' : control === 'X' ? (battle ? 'active' : 'gil') :
        !battle ? ({Y:'map', A:'trial-step', B:'trial-path'}[control] || '') : '';
    if (menu) return control === 'RSUp' ? 'description' : '';
    if (battle) return '';
    return {Left:'previous',Right:'next',Up:'previous-category',Down:'next-category',RT:'route',L3:'beacon',R3:'filter'}[control] || '';
}
function keepDestination(meshKnown, route, hideUnreachable) {
    return !hideUnreachable || !meshKnown || route.length > 0;
}

function plannerOrigin(key, character, cursor) { return key === 'F4' || key === 'F5' ? character : cursor; }
