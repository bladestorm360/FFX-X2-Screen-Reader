// @desc Native list input coverage (shops/remaining lists); event-only, no speech
// @auto
// Registered automatically by run_frida.bat's filename/@desc discovery.
// User runs the launcher. This diagnostic never calls a game text function.
// Open the silent surface, move selection, return; retain the timestamped log.
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
var stamp = new Date().toISOString().replace(/[:.]/g, '-');
var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/native_list_events_' + stamp + '.log';
var file = new File(logPath, 'w');
function log(value) { file.write(value + '\n'); file.flush(); console.log(value); }
function offset(pointer) { return pointer.sub(base).toString(); }
function valid(pointer) { return pointer && pointer.compare(ptr(0x10000)) > 0; }
function executable(pointer) {
    var range = Process.findRangeByAddress(pointer);
    return range && range.protection.indexOf('x') >= 0;
}
function windowState(window) {
    if (!valid(window)) return null;
    var input = window.add(0x0C).readPointer();
    var renderer = window.add(0x10).readPointer();
    if (!executable(input) || !executable(renderer)) return null;
    var count = window.add(0x30).readU16();
    if (count > 512) return null;
    return {
        phase: window.add(0x28).readS32(), row: window.add(0x48).readS16(),
        depth: window.add(0x45).readS8(), count: count,
        input: offset(input), renderer: offset(renderer)
    };
}
function installInput(address) {
    Interceptor.attach(rva(address), {
        onEnter: function(args) {
            try {
                this.window = args[0];
                this.before = JSON.stringify(windowState(this.window));
                this.caller = offset(this.returnAddress);
            } catch (error) { this.window = null; }
        },
        onLeave: function() {
            if (!this.window) return;
            try {
                var after = windowState(this.window);
                if (!after || JSON.stringify(after) === this.before) return;
                log('[INPUT] hook=' + address.toString(16) + ' owner=' + Process.getCurrentThreadId() +
                    ' caller=' + this.caller + ' window=' + this.window +
                    ' before=' + this.before + ' after=' + JSON.stringify(after));
            } catch (error) { log('[READ] ' + error.message); }
        }
    });
}
[0x4B4460, 0x4C6DA0, 0x4D0E80, 0x4E3880].forEach(installInput);
Interceptor.attach(rva(0x4AA0B0), {
    onEnter: function(args) { this.scene = args[0].toInt32(); },
    onLeave: function() {
        try {
            var scene = this.scene;
            if (scene < 0 || scene > 0x18) return;
            var menu = rva(0x1440848).add(scene * 4).readPointer();
            if (valid(menu)) log('[SCENE] id=' + scene.toString(16) +
                ' control=' + offset(menu.readPointer()) + ' draw=' + offset(menu.add(4).readPointer()));
        } catch (error) { log('[SCENE READ] ' + error.message); }
    }
});
log('Native list coverage diagnostic installed; log=' + logPath);
