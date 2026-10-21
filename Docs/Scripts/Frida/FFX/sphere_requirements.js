// @desc Read-only Sphere Grid disabled flags and node material requirements (offline prototype)
// One native production owner; this reference never speaks or writes game state.
function sphereUnavailable(disabled) { return disabled !== 0; }
function requiredSphere(flags, nodeType, catalog) {
    const locks = { 0x12: 1, 0x11: 2, 0x00: 3, 0x10: 4 };
    const special = flags & 0x8000 ? locks[nodeType] : 0;
    const bit = flags & 0x8000 ? 0 : flags & -flags;
    const sphere = catalog.find(s => special ? s.special === special : !s.special && (s.flags & bit));
    return sphere ? sphere.id : 0;
}
if (typeof module !== 'undefined') module.exports = { sphereUnavailable, requiredSphere };
else {
    const main = Process.enumerateModules()[0], base = main.base;
    if (main.name.toLowerCase() !== 'ffx.exe') throw Error('FFX only');
    Process.enumerateModules().forEach(m => m.enumerateExports().forEach(x => {
        if (x.name === 'FfxNative_Features' && new NativeFunction(x.address, 'uint', [], 'mscdecl')())
            throw Error('Native reader owns the grid; this prototype must remain offline');
    }));
    const item = new NativeFunction(base.add(0x390a40), 'pointer', ['uint', 'pointer'], 'mscdecl');
    const definition = new NativeFunction(base.add(0x3ab890), 'pointer', ['uint', 'pointer', 'pointer'], 'mscdecl');
    const file = new File('D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/sphere_requirements_' + new Date().toISOString().replace(/[:.]/g, '-') + '.jsonl', 'w');
    function emit(value) { file.write(JSON.stringify(value) + '\n'); file.flush(); }
    Interceptor.attach(base.add(0x648d70), { onLeave() {
        const grid = base.add(0x1f05834).readPointer();
        if (grid.isNull()) return;
        const index = grid.add(0x112fc).readS16(), count = grid.add(2).readS16();
        if (index < 0 || index >= count || count > 1024) return;
        const type = grid.add(0x80e + index * 0x28).readS16();
        const nodes = base.add(0x16860e0).readPointer(), spheres = base.add(0x16860e4).readPointer();
        if (type < 0 || nodes.isNull() || spheres.isNull()) return;
        const record = definition(type, nodes, ptr(0)), catalog = [];
        if (record.isNull()) return;
        for (let id = 0; id < 256; ++id) {
            const data = item(id, ptr(0));
            if (data.isNull() || data.add(0x5d).readS8() < 0) continue;
            const sphere = definition(data.add(0x5d).readS8(), spheres, ptr(0));
            if (!sphere.isNull()) catalog.push({ id: 0x2000 + id, special: sphere.add(0xd).readU8(), flags: sphere.add(0xa).readU16() });
        }
        emit({ event: 'node', index, sphere: requiredSphere(record.add(0x10).readU16(), record.add(0x16).readU8(), catalog) });
    } });
    Interceptor.attach(base.add(0x6598a0), { onLeave() {
        const pool = base.add(0x1686108).readPointer();
        if (pool.isNull() || pool.add(0x2700).readS16() !== 8 || !pool.add(0x2704).readPointer().equals(base.add(0x6560d0))) return;
        const window = pool.add(8 * 0x340), row = window.add(0x18).readS16(), count = window.add(0x1e).readS16();
        if (window.add(0x23).readU8() !== 1 || row < 0 || row >= count || count > 64) return;
        const entry = window.add(0x40 + row * 12);
        emit({ event: 'inventory', row, sphere: entry.add(8).readU16(), unavailable: sphereUnavailable(entry.add(4).readU8()) });
    } });
}
