// @python
// @desc Map function table reader + zone hooks
//
// Reads the native function addresses for Map namespace commands,
// hooks map:11 (zone enable), map:9 (zone config), map:2 (zone set)
// to trace how exit zones are configured.
//
// Keyboard:
//   t = Read Map function table (all 108 entries)
//   w = Hook map:11, map:9, map:2 to trace zone operations
//   s = State
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_map_funcs.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_map_funcs — Map function table + zone hooks');

var RVA_MAP_TABLE = 0x85DC90; // Map namespace AtelCallTarget table (Fahrenheit uses RVAs directly)
var ATEL_CT_SIZE = 0x10;
var MAP_FUNC_COUNT = 108; // 0x8000-0x806B

function getPlayerPos() {
    var ab = rva(0x1FC44E4).readPointer();
    if (ab.isNull()) return null;
    return { x: ab.add(0x0C).readFloat(), y: ab.add(0x10).readFloat(), z: ab.add(0x14).readFloat() };
}

function readMapTable() {
    var tableBase = rva(RVA_MAP_TABLE);
    logBoth('[TABLE] Map namespace function table at ' + tableBase + ':');

    for (var i = 0; i < MAP_FUNC_COUNT; i++) {
        var ct = tableBase.add(i * ATEL_CT_SIZE);
        var initFunc = ct.readPointer();
        var execFunc = ct.add(4).readPointer();
        var retFloatFunc = ct.add(8).readPointer();
        var retIntFunc = ct.add(12).readPointer();

        var hasAny = false;
        var parts = [];
        if (!initFunc.isNull() && initFunc.toUInt32() > 0x10000) { parts.push('init=rva(0x' + initFunc.sub(base).toString(16) + ')'); hasAny = true; }
        if (!execFunc.isNull() && execFunc.toUInt32() > 0x10000) { parts.push('exec=rva(0x' + execFunc.sub(base).toString(16) + ')'); hasAny = true; }
        if (!retFloatFunc.isNull() && retFloatFunc.toUInt32() > 0x10000) { parts.push('retF=rva(0x' + retFloatFunc.sub(base).toString(16) + ')'); hasAny = true; }
        if (!retIntFunc.isNull() && retIntFunc.toUInt32() > 0x10000) { parts.push('retI=rva(0x' + retIntFunc.sub(base).toString(16) + ')'); hasAny = true; }

        if (!hasAny) continue;

        var id = '0x' + (0x8000 + i).toString(16);
        logBoth('  map:' + i + ' (' + id + '): ' + parts.join(' | '));
    }
}

function hookZoneFuncs() {
    var tableBase = rva(RVA_MAP_TABLE);

    var funcs = [
        { idx: 2, name: 'map:2' },
        { idx: 6, name: 'map:6' },
        { idx: 9, name: 'map:9' },
        { idx: 11, name: 'map:11' },
        { idx: 13, name: 'map:13' }
    ];

    var ptrOffsets = [
        { off: 0, label: 'init' },
        { off: 4, label: 'exec' },
        { off: 8, label: 'retF' },
        { off: 12, label: 'retI' }
    ];

    for (var fi = 0; fi < funcs.length; fi++) {
        var f = funcs[fi];
        var ct = tableBase.add(f.idx * ATEL_CT_SIZE);
        var hooked = false;

        for (var pi = 0; pi < ptrOffsets.length; pi++) {
            var funcPtr = ct.add(ptrOffsets[pi].off).readPointer();
            if (funcPtr.isNull() || funcPtr.toUInt32() < 0x10000) continue;

            var funcRva = funcPtr.sub(base);
            logBoth('[HOOK] ' + f.name + '.' + ptrOffsets[pi].label + ' at rva(0x' + funcRva.toString(16) + ')');
            hooked = true;

            (function(name, label, addr) {
                Interceptor.attach(addr, {
                    onEnter: function(args) {
                        var pos = getPlayerPos();
                        var info = '';
                        try { info = ' args=(' + args[0] + ',' + args[1] + ',' + args[2] + ')'; } catch (e) {}
                        try {
                            var wp = rva(0xF270A4).readPointer();
                            if (!wp.isNull()) {
                                info += ' i0=' + wp.add(0x48).readS32() + ' i1=' + wp.add(0x4C).readS32();
                            }
                        } catch (e) {}
                        logBoth('[CALL] ' + name + '.' + label + info +
                            (pos ? ' pos=(' + pos.x.toFixed(1) + ',' + pos.z.toFixed(1) + ')' : ''));
                    }
                });
            })(f.name, ptrOffsets[pi].label, funcPtr);
        }

        if (!hooked) logBoth('[!] ' + f.name + ': no function pointers found');
    }

    logBoth('[HOOK] Hooks installed. Walk around or transition to trigger logging.');
}

function showState() {
    var pos = getPlayerPos();
    var room = rva(0xD2CA90).readU16();
    logBoth('[STATE] Room=' + room + (pos ? ' Pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')' : ''));
}

rpc.exports = {
    table: function() { readMapTable(); return 'ok'; },
    watch: function() { hookZoneFuncs(); return 'ok'; },
    state: function() { showState(); return 'ok'; }
};
