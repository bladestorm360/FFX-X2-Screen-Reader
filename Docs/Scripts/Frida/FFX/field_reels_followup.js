// Offline Frida prototype; production has one native implementation.
function activeFieldSnapshot(field) {
    return !!field.controller && !!(field.control & 2) && field.workers > 0 && field.workers <= 500 &&
        !!field.actors && field.capacity > 0 && field.capacity <= 128 &&
        field.player >= field.actors && field.player < field.actors + field.capacity * 0x880 &&
        (field.player - field.actors) % 0x880 === 0 && field.enabled > 0 && field.enabled <= 255 &&
        field.collision !== 0xcdcdcdcd && field.position.every(x => Number.isFinite(x) && x !== -431602080) &&
        !!field.chunk && field.begin > 0 && field.end > field.begin && field.end <= 0x800000 &&
        (field.end - field.begin) % 32 === 0 && field.entrance < (field.end - field.begin) / 32;
}
function nativeOverdrivePercent(current, maximum) {
    return maximum > 0 ? Math.min(100, Math.floor((current * 100 + Math.floor(maximum / 2)) / maximum)) : null;
}
function attackReels(patterns) {
    return patterns.length === 3 && patterns.every(p => p.length > 0 && p.length <= 32 &&
        p.includes(6) && p.every(symbol => symbol >= 4 && symbol <= 6));
}
function automaticReelMatch(patterns) {
    return attackReels(patterns) ? 6 : null;
}
function qualifyReelOverlay(base, table, expected, entries) {
    return table === base + expected.table && entries[0] === base + expected.begin &&
        (entries[4] === base + expected.initial || entries[4] === base + expected.control);
}
function cueSymbol(patterns, symbol) { return !attackReels(patterns) || symbol === 5 || symbol === 6; }
function effectSource(handle, command) {
    if ((handle & 255) !== 4 || !Number.isInteger(command) || command < 0x3074 || command > 0x3077) return null;
    const type = (handle >>> 8) & 255;
    return type === 2 ? 'character' : type === 0x80 ? 'global' : null;
}
function installWakkaStartPrototype(resolve, obtained) {
    const character = new NativeFunction(resolve(0x394030), 'pointer', ['uint32']);
    return Interceptor.attach(resolve(0x388eb0), {
        onLeave(result) {
            const actor = character(4);
            if (actor.isNull()) return;
            const source = effectSource(result.toUInt32(), actor.add(0xf5c).readU16());
            if (!source) return;
            const table = source === 'global' ? resolve(0xd33360).readPointer() : actor.add(0xf8c).readPointer();
            if (!table.isNull()) obtained(table);
        }
    });
}
if (typeof module !== 'undefined') module.exports = {
    activeFieldSnapshot, nativeOverdrivePercent, attackReels, automaticReelMatch, cueSymbol, effectSource,
    installWakkaStartPrototype, qualifyReelOverlay
};
