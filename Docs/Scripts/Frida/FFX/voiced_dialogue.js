// @desc Voiced dialogue policy prototype: native voice events and dialogue, file-only (no speech)
// User-run RE only. No timers, game writes or production speech producer.
function audibleVoice(s) {
    return !!(s && s.playing && !s.paused && !s.muted && s.volume > 0 && s.mix > 0);
}
function dialogueMode(voiced, mode) { return voiced ? mode : 2; }
if (typeof module !== 'undefined') module.exports = { audibleVoice, dialogueMode };
else {
    var main = Process.enumerateModules()[0], base = main.base;
    if (main.name.toLowerCase() !== 'ffx.exe') throw Error('FFX only');
    var root = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/';
    var specs = JSON.parse(File.readAllText(root + 'Frida/FFX/native_extensions.json'));
    var stamp = new Date().toISOString().replace(/[:.]/g, '-');
    var file = new File(root + 'Frida/output/FFX/voiced_dialogue_' + stamp + '.jsonl', 'w');
    function log(s) { file.write(JSON.stringify(s) + '\n'); file.flush(); }
    var resolved = {};
    specs.filter(function(s) { return s.rva >= 0x307b40 && s.rva <= 0x30b8e0; }).forEach(function(s) {
        var pattern = s.tokens.map(function(b) { return b === null ? '??' : ('0' + b.toString(16)).slice(-2); }).join(' ');
        var found = Memory.scanSync(base.add(s.rva - 4096), 8192, pattern);
        if (found.length !== 1) throw Error('Ambiguous voice entry ' + s.rva.toString(16));
        resolved[s.rva] = found[0].address;
    });
    var parent = specs.filter(function(s) { return s.rva === 0x30b6b0; })[0];
    [0x548c9a, 0x548ca6].forEach(function(id) {
        var call = parent.calls[id];
        resolved[id] = resolved[parent.rva].add(call.end).add(resolved[parent.rva].add(call.offset).readS32());
    });
    var state = null, starts = {}, serial = 0;
    function snapshot(o, playing) {
        return { owner: o.toString(), event: o.add(0x14).readPointer().toString(), playing: playing,
            paused: o.add(0x54).readU32() === 1, muted: o.add(0x80).readU32() === 1,
            volume: o.add(0x50).readFloat(), mix: o.add(0x88).readFloat(), voice: o.add(0x5c).readU32() };
    }
    Interceptor.attach(resolved[0x548c9a], {
        onEnter: function(args) { this.event = args[0].toString(); },
        onLeave: function(result) { if (result.toInt32() === 0) starts[this.threadId] = { event: this.event, serial: ++serial }; }
    });
    [0x30b6b0, 0x30b250].forEach(function(id) {
        Interceptor.attach(resolved[id], {
            onEnter: function() { this.owner = this.context.ecx; this.serial = serial; },
            onLeave: function() { try {
                var started = starts[this.threadId], s = snapshot(this.owner, true);
                if (started && started.serial > this.serial && started.event === s.event) { state = s; log({ event: 'voice-start', state: s, audible: audibleVoice(s) }); }
            } catch(e) { log({ error: String(e) }); } }
        });
    });
    Interceptor.attach(resolved[0x548ca6], {
        onEnter: function(args) { this.event = args[0].toString(); this.state = args[1]; },
        onLeave: function(result) { if (state && state.event === this.event) state.playing = result.toInt32() === 0 && !!(this.state.readU32() & 8); }
    });
    [0x30b8e0, 0x30b420, 0x30b3a0, 0x30b460].forEach(function(id) {
        Interceptor.attach(resolved[id], { onEnter: function() { this.owner = this.context.ecx; },
            onLeave: function() { if (state && state.owner === this.owner.toString()) state = snapshot(this.owner, state.playing); } });
    });
    [0x30aec0, 0x30a680].forEach(function(id) {
        Interceptor.attach(resolved[id], { onEnter: function() { state = null; } });
    });
    Interceptor.attach(resolved[0x307b40], { onEnter: function(args) {
        if (state && (args[1].toInt32() === 2 || args[1].toInt32() === 4) && args[0].toString() === state.event && args[4].equals(ptr(state.owner).add(0x40))) state.playing = false;
    } });
    var installed = !resolved[0x30b6b0].equals(base.add(0x30b6b0));
    Interceptor.attach(base.add(installed ? 0x376940 : 0x376a50), { onLeave: function() {
        log({ event: 'dialogue-decode', state: state, audible: audibleVoice(state) });
    } });
    log({ event: 'ready', profile: installed ? 'installed' : 'reference' });
}
