// Offline Frida prototype; never bundled or enabled by the production proxy.
// resolve must supply independently verified RVAs for the target profile.
// Load field_reels_followup.js first for the shared pure readiness predicate.
function installMapTransitionPrototype(resolve, emit, generation = () => 1) {
    const table = new NativeFunction(resolve(0x4b7f10), 'pointer', ['uint32']);
    const index = new NativeFunction(resolve(0x46bd20), 'int', ['pointer', 'uint32']);
    const name = new NativeFunction(resolve(0x46c3c0), 'pointer', ['uint32', 'uint32']);
    const decode = new NativeFunction(resolve(0x2460e0), 'void', ['pointer', 'pointer']);
    let pending = null;
    const finish = () => {
            if (!pending) return;
            const loaded = resolve(0xd2ca90).readU16();
            if (pending.epoch !== generation() || pending.room !== loaded || resolve(0xd2a8e0).readU8()) { pending = null; return; }
            const controller = resolve(0xf26ae8).readPointer();
            if (controller.isNull()) return;
            const chunk = controller.add(0x1f8).readPointer();
            if (chunk.isNull()) return;
            const entrance = resolve(0xd2ca90).add(0xc).readU8();
            const begin = chunk.add(4).readU32(), end = chunk.add(8).readU32();
            if (!chunk.equals(pending.chunk) || entrance !== pending.entrance) { pending = null; return; }
            const player = resolve(0xf00740).readPointer(), actors = resolve(0x1fc44e4).readPointer();
            if (player.isNull()) return;
            if (!activeFieldSnapshot({controller: controller.toUInt32(), control: controller.readU8(),
                workers: controller.add(0xc).readU16(), actors: actors.toUInt32(), capacity: resolve(0x1fc44e0).readU32(),
                player: player.toUInt32(), enabled: player.add(2).readU16(), collision: player.add(0x824).readU32(),
                position: [0xc, 0x10, 0x14].map(offset => player.add(offset).readFloat()),
                chunk: chunk.toUInt32(), begin, end, entrance})) return;
            const area = index(chunk, entrance), catalog = table(0);
            if (catalog.isNull() || area < 0 || area >= (catalog.readU16() >> 2)) return;
            const encoded = name(area, 0);
            if (encoded.isNull()) return;
            const buffer = Memory.alloc(256);
            buffer.writeByteArray(new Uint8Array(256));
            decode(encoded, buffer);
            const text = buffer.readCString(255);
            if (text && text.length >= 2) { pending = null; emit('Entering ' + text, false, 'map-transition'); }
    };
    const hooks = [0x46fed0, 0x5093b0].map(rva => Interceptor.attach(resolve(rva), {onEnter() { pending = null; }}));
    hooks.push(Interceptor.attach(resolve(0x48dfe0), {
        onEnter(args) { pending = null; this.room = args[0].toUInt32(); },
        onLeave() {
            if (this.room === 23 || this.room !== resolve(0xd2ca90).readU16()) return;
            const controller = resolve(0xf26ae8).readPointer();
            if (controller.isNull()) return;
            pending = {room: this.room, epoch: generation(), chunk: controller.add(0x1f8).readPointer(),
                entrance: resolve(0xd2ca90).add(0xc).readU8()};
            finish();
        }
    }));
    for (const rva of [0x471a50, 0x425010, 0x42b500, 0x426770]) hooks.push(Interceptor.attach(resolve(rva), {onLeave: finish}));
    return hooks;
}
if (typeof module !== 'undefined') module.exports = { installMapTransitionPrototype };
