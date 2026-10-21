// @desc Read-only Sphere Grid graph/path prototype, event-driven, file-only details
// Offline prototype first; production has one native owner. No speech or writes.
function sphereRoute(grid, start, goal, character) {
    if (character < 0 || character >= 7 || !grid.nodes[start] || !grid.nodes[goal] || !grid.nodes[start].valid || !grid.nodes[goal].valid) return null;
    var best = grid.nodes.map(function () { return Infinity; });
    var parent = grid.nodes.map(function () { return -1; });
    var done = grid.nodes.map(function () { return false; });
    best[start] = 0;
    for (var iteration = 0; iteration < grid.nodes.length; ++iteration) {
        var at = -1;
        for (var i = 0; i < best.length; ++i)
            if (!done[i] && (at < 0 || best[i] < best[at])) at = i;
        if (at < 0 || !isFinite(best[at])) return null;
        if (at === goal) break;
        done[at] = true;
        grid.links.forEach(function (edge) {
            var next = edge.a === at ? edge.b : edge.b === at ? edge.a : -1;
            if (next < 0 || !grid.nodes[next].valid) return;
            // Fewest locks, then quarter-level movement cost. 4097 exceeds
            // every simple route's movement cost in a 1024-node native grid.
            var movement = next === goal && grid.nodes[goal].ability ? 0 : ((edge.travelled & (1 << character)) ? 1 : 4);
            var cost = best[at] + movement
                + (grid.nodes[next].locked ? 4097 : 0);
            if (cost < best[next]) { best[next] = cost; parent[next] = at; }
        });
    }
    if (!isFinite(best[goal])) return null;
    var path = [goal];
    while (path[0] !== start) {
        if (path.length > grid.nodes.length || parent[path[0]] < 0) return null;
        path.unshift(parent[path[0]]);
    }
    return path;
}
if (typeof Process !== 'undefined') {
    var base = Process.enumerateModules()[0].base;
    if (Process.enumerateModules()[0].name.toLowerCase() !== 'ffx.exe') throw Error('FFX only');
    var nativeActive = false;
    Process.enumerateModules().forEach(function (module) {
        module.enumerateExports().forEach(function (item) {
            if (item.name === 'FfxNative_Features' && new NativeFunction(item.address, 'uint', [], 'mscdecl')()) nativeActive = true;
        });
    });
    if (nativeActive) throw Error('Native reader owns the grid; this prototype must remain offline');
    var stamp = new Date().toISOString().replace(/[:.]/g, '-');
    var file = new File('D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/sphere_planner_' + stamp + '.log', 'w');
    Interceptor.attach(base.add(0x648d70), { onLeave: function () {
        try {
            var engine = base.add(0x1f05834).readPointer(), count = engine.add(2).readS16();
            var links = engine.add(4).readS16(), grid = { nodes: [], links: [] };
            if (count < 1 || count > 1024 || links < 0 || links > 1024) return;
            for (var i = 0; i < count; ++i) {
                var n = engine.add(0x808 + i * 0x28);
                grid.nodes.push({ valid: n.add(6).readS16() >= 0, type: n.add(6).readS16(), x: n.readS16(), y: n.add(2).readS16() });
            }
            for (var j = 0; j < links; ++j) {
                var e = engine.add(0xa808 + j * 0x14);
                grid.links.push({ a: e.readS16(), b: e.add(2).readS16(), travelled: e.add(12).readU8() });
            }
            file.write(JSON.stringify({ selected: engine.add(0x112fc).readS16(), character: engine.add(0x115bc).readU8(), grid: grid }) + '\n'); file.flush();
        } catch (error) { file.write(JSON.stringify({ error: String(error) }) + '\n'); file.flush(); }
    } });
}
