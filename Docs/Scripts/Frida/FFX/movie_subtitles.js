// @desc Movie subtitle coverage: correlate existing readers and ending captions with native playback (file-only)
// User-run diagnostic. No speech, input, game writes, timers, or NativeFunction calls.
// Signatures come from audit_movie_subtitles.py, not a shared address delta.
function audibleMovie(s) {
    return !!(s && s.playing && s.initialized && s.audioStarted &&
        s.channel && s.audioTrack && (s.playbackState === 1 || s.playbackState === 2) &&
        !s.audioPaused && s.volume > 0);
}
function captionIdentity(generation, owner, row, index) {
    return [generation, owner, row, index].join(':');
}
if (typeof module !== 'undefined') module.exports = { audibleMovie, captionIdentity };
else (function() {
    var main = Process.enumerateModules()[0], base = main.base;
    if (main.name.toLowerCase() !== 'ffx.exe' || Process.pointerSize !== 4)
        throw Error('This diagnostic requires native x86 FFX');
    var root = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/';
    var spec = JSON.parse(File.readAllText(root + 'Frida/FFX/movie_subtitles_signatures.json'));
    function pattern(tokens) {
        return tokens.map(function(b) { return b === null ? '??' : ('0' + b.toString(16)).slice(-2); }).join(' ');
    }
    function accepts(profile) {
        return spec.signatures.every(function(s) {
            // Production MinHook can own the first five entry bytes. Check
            // the rest at the individually audited entry before attaching.
            var tail = s.tokens.slice(16);
            return Memory.scanSync(base.add(s[profile] + 16), tail.length, pattern(tail)).length === 1;
        });
    }
    var profiles = ['reference', 'installed'].filter(accepts);
    if (profiles.length !== 1) throw Error('Movie diagnostic refused: unsupported or ambiguous native entries');
    var profile = profiles[0], entries = {};
    spec.signatures.forEach(function(s) { entries[s.label] = base.add(s[profile]); });
    var stamp = new Date().toISOString().replace(/[:.]/g, '-');
    var file = new File(root + 'Frida/output/FFX/movie_subtitles_' + stamp + '.jsonl', 'w');
    var counts = {}, capped = {}, latest = null, stateKey = '', generation = 0;
    var endingCalls = {}, lastEnding = '';
    function log(event, data) {
        counts[event] = (counts[event] || 0) + 1;
        if (counts[event] > 2000) {
            if (!capped[event]) { capped[event] = true; write({ event: 'capture-limit', producer: event }); }
            return;
        }
        data.event = event; data.utc = new Date().toISOString(); data.generation = generation;
        write(data);
    }
    function write(data) { file.write(JSON.stringify(data) + '\n'); file.flush(); }
    function rawText(p) {
        var out = [], terminated = false;
        for (var i = 0; i < 256; ++i) {
            var b = p.add(i).readU8();
            if (b === 0) { terminated = true; break; }
            out.push(('0' + b.toString(16)).slice(-2));
        }
        return { pointer: p.toString(), ffx_hex: out.join(''), terminated: terminated };
    }
    function movie(o) {
        return { owner: o.toString(), movie_id: o.add(0x6dc).readS32(),
            playing: o.add(0x6d0).readU8() !== 0, initialized: o.add(0x6d2).readU8() !== 0,
            paused: o.add(0x6fc).readU8() !== 0, progress: o.add(0x6d8).readS32(),
            previous_progress: o.add(0x6e0).readS32() };
    }
    function guarded(run) { try { run(); } catch(e) { log('read-error', { error: String(e) }); } }
    function stateAfter(label, owner) {
        latest = movie(owner);
        var key = [latest.owner, latest.movie_id, latest.playing, latest.initialized, latest.paused].join(':');
        if (key !== stateKey || label !== 'movie_update') { stateKey = key; log(label, { movie: latest }); }
    }
    ['movie_load', 'movie_start', 'movie_update', 'movie_stop'].forEach(function(label) {
        Interceptor.attach(entries[label], {
            onEnter: function(args) {
                this.owner = this.context.ecx;
                if (label === 'movie_load') { ++generation; lastEnding = ''; this.requested = args[0].toInt32(); }
                if (label === 'movie_stop') guarded(function() { log('movie_stop_enter', { movie: movie(this.owner) }); }.bind(this));
            },
            onLeave: function() { guarded(function() {
                stateAfter(label, this.owner);
                if (label === 'movie_load') log('movie_requested', { movie_id: this.requested });
            }.bind(this)); }
        });
    });
    Interceptor.attach(entries.movie_external_start, { onEnter: function(args) {
        guarded(function() { ++generation; lastEnding = ''; log('movie_external_start', {
            request: args[0].toString(), filename: args[0].readPointer().readCString(256) }); });
    } });
    Interceptor.attach(entries.ending_tick, {
        onEnter: function() { var o = this.context.ecx; guarded(function() {
            endingCalls[this.threadId] = { movie: movie(o), row: o.add(0x72c).readPointer().toString() };
        }.bind(this)); },
        onLeave: function() { delete endingCalls[this.threadId]; }
    });
    Interceptor.attach(entries.ending_draw, { onEnter: function(args) { guarded(function() {
        var parent = endingCalls[this.threadId];
        if (!parent) return;
        var key = generation + ':' + parent.movie.owner + ':' + parent.row;
        // Record the native timeline entry once, not every draw frame.
        if (key === lastEnding) return;
        lastEnding = key;
        log('ending-caption', { movie: parent.movie, row: parent.row, text: rawText(args[0]) });
    }.bind(this)); } });
    var tkPool = base.add(profile === 'installed' ? 0x14676f0 : 0x14676b0);
    Interceptor.attach(entries.npc_install, {
        onEnter: function(args) { this.slot = args[0].toUInt32(); this.caller = this.returnAddress.sub(base).toString(); },
        onLeave: function() { guarded(function() {
            if (this.slot >= 8) return;
            var window = tkPool.add(this.slot * 4).readPointer();
            log('npc-install', { movie: latest, slot: this.slot, caller_rva: this.caller,
                text: rawText(window.add(0x94).readPointer()) });
        }.bind(this)); }
    });
    Interceptor.attach(entries.event_decode, {
        onEnter: function(args) { this.buffer = args[0]; this.source = args[1].toString(); },
        onLeave: function() { guarded(function() { log('event-decode', {
            movie: latest, source: this.source, text: rawText(this.buffer) }); }.bind(this)); }
    });
    log('ready', { profile: profile, expected_executable_sha256: spec[profile + '_sha256'],
        path: root + 'Frida/output/FFX/movie_subtitles_' + stamp + '.jsonl' });
})();
