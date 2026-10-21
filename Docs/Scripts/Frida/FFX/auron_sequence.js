// Event-only offline prototype; never bundled or enabled by the native proxy.
// resolve and bindingName must supply verified native addresses/action names.
function installAuronSequencePrototype(resolve, context, bindingName, emit) {
    const masks = [0x1000, 0x4000, 0x8000, 0x2000, 0x20, 0x40, 0x10,
        0x80, 4, 1, 8, 2, 0x800, 0x100];
    return Interceptor.attach(resolve(0x498a40), {
        onLeave() {
            const state = context();
            if (!state.battle || !state.focused || state.scene || state.menu || state.utility ||
                !state.cues || state.autoSuccess || resolve(0xf3d6b4).readU16() !== 1) return;
            const count = resolve(0xf3d6c0).readU32(), source = resolve(0xf3d6c4).readPointer();
            if (source.isNull() || count < 1 || count > 15 || source.add(count * 2).readU16() !== 0xffff) return;
            const names = [];
            for (let i = 0; i < count; ++i) {
                const action = masks.indexOf(source.add(i * 2).readU16());
                if (action < 0) return;
                const name = bindingName(action + (state.controller ? 14 : 0), state.controller);
                if (!name) return;
                names.push(name);
            }
            emit('Sequence: ' + names.join(', '), true, 'overdrive-auron-sequence');
        }
    });
}
if (typeof module !== 'undefined') module.exports = {installAuronSequencePrototype};
