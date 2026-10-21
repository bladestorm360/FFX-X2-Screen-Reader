// @desc Read-only grid movement/exit confirmation and committed node events (offline prototype)
// File-only events; the native reader remains the single production owner.
function gridConfirmation(slot, callback) {
    return slot !== 11 ? -1 : callback === 0x656160 ? 9 : callback === 0x656060 ? 10 : -1;
}
function movementConfirmed(choice, confirmed, source, destination, count) {
    return choice === 0 && !!confirmed && source >= 0 && source < count && destination >= 0 && destination < count && source !== destination;
}
function preparingMovement(character, source, destination, count, busy) {
    return character >= 0 && character < 7 && count > 0 && count <= 1024 && !busy &&
        source >= 0 && source < count && destination >= 0 && destination < count && source !== destination;
}
if (typeof module !== 'undefined') module.exports = { gridConfirmation, movementConfirmed, preparingMovement };
else {
    const main = Process.enumerateModules()[0], base = main.base;
    if (main.name.toLowerCase() !== 'ffx.exe') throw Error('FFX only');
    Process.enumerateModules().forEach(m => m.enumerateExports().forEach(x => {
        if (x.name === 'FfxNative_Features' && new NativeFunction(x.address, 'uint', [], 'mscdecl')())
            throw Error('Native reader owns the grid; this prototype must remain offline');
    }));
    const file = new File('D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/sphere_confirmations_' + new Date().toISOString().replace(/[:.]/g, '-') + '.jsonl', 'w');
    function emit(value) { file.write(JSON.stringify(value) + '\n'); file.flush(); }
    Interceptor.attach(base.add(0x648c80), { onEnter(args) {
        const grid = base.add(0x1f05834).readPointer();
        if (grid.isNull() || !base.add(0x1685f70).readU8()) return;
        const character = args[0].toUInt32(), destination = args[1].toInt32();
        if (character >= 7) return;
        const source = grid.add(0x110cc + character * 0x50).readS16();
        if (preparingMovement(character, source, destination, grid.add(2).readS16(), !grid.add(0x115b0).readPointer().isNull()))
            emit({ event: 'preparing', character, source, destination });
    } });
    let previous = '';
    Interceptor.attach(base.add(0x6583e0), { onLeave() {
        const pool = base.add(0x1686108).readPointer();
        if (pool.isNull()) return;
        const callback = pool.add(0x2704).readPointer().sub(base).toUInt32();
        const promptSlot = gridConfirmation(pool.add(0x2700).readS16(), callback);
        if (promptSlot < 0) { previous = ''; return; }
        const window = pool.add(11 * 0x340), row = window.add(0x18).readS16();
        if (window.add(0x23).readU8() !== 1 || row < 0 || row >= window.add(0x1e).readS16()) return;
        const identity = [pool, callback, row].join(':');
        if (identity === previous) return;
        previous = identity;
        emit({ event: 'confirmation', callback, row, prompt: pool.add(promptSlot * 0x340 + 0x40).readPointer().toString(), choice: window.add(0x40 + row * 12).readPointer().toString() });
    } });
    Interceptor.attach(base.add(0x656160), {
        onEnter(args) {
            this.grid = base.add(0x1f05834).readPointer(); this.choice = args[2].toUInt32(); this.confirmed = args[3].toUInt32();
            if (this.grid.isNull()) return;
            this.character = this.grid.add(0x115bc).readU8(); this.source = this.grid.add(0x11630).readS16();
        },
        onLeave() {
            if (this.grid.isNull() || this.character >= 7) return;
            const destination = this.grid.add(0x110cc + this.character * 0x50).readS16();
            if (movementConfirmed(this.choice, this.confirmed, this.source, destination, this.grid.add(2).readS16()))
                emit({ event: 'moved', character: this.character, source: this.source, destination });
        }
    });
}
