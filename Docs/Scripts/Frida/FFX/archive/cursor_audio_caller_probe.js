// @desc Capture callers of the cursor-move audio cue chain.
//
// The stalker probe identified four functions that fire on a title-screen
// cursor-down (all at count=1 during armed, count=0 during idle baseline):
//   FUN_0081e5f0 (RVA 0x41e5f0) -> FUN_0067a5f0 (RVA 0x27a5f0)
//     -> FUN_006fa3c0 (RVA 0x2fa3c0) -> FUN_006fa3e0 (RVA 0x2fa3e0)
//
// FUN_006fa3e0 is the Fmod audio command queue (confirmed by decompile:
// "[Fmod Error] Too many fmod commands..."). So the chain is the audio
// cue path that fires as a consequence of cursor-move. The function
// that CALLED this chain is the cursor handler.
//
// This probe hooks each of the four and logs `this.returnAddress` —
// the caller's RVA — so we can trace upward to the actual handler.
// It also sanity-hooks the per-frame function at 0x471D10 to verify the
// probe is actually running + attaching correctly.
//
// Usage: run via run_frida.bat -> FFX -> cursor_audio_caller_probe.
// Expected startup log lines:
//   - [*] Module: FFX.exe base=0x????????
//   - [*] hooked <label> @ RVA 0x... (x4 + sanity)
//   - [SANITY] per-frame fired N times (appears every ~5s — confirms
//             probe is live and hooks attach correctly)
// Then on title screen: idle on "New Game", press Down once, wait a
// second, Ctrl+C. The log should have [FIRE] lines with caller RVAs.

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/cursor_audio_caller_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

// Explicitly find FFX.exe rather than trusting module-enum order.
let gameModule = null;
Process.enumerateModules().forEach(function(m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) {
    // Fallback: first module is usually the main exe
    gameModule = Process.enumerateModules()[0];
    logBoth('[!] ffx.exe not found by name — falling back to module[0]=' + gameModule.name);
}

const base = gameModule.base;
const modEnd = base.add(gameModule.size);
function rva(offset) { return base.add(offset); }

logBoth('[*] Module: ' + gameModule.name + ' base=0x' + base.toString(16) +
        ' size=0x' + gameModule.size.toString(16));

function describeReturn(retAddr) {
    if (retAddr.isNull()) return 'null';
    if (retAddr.compare(base) < 0 || retAddr.compare(modEnd) >= 0) {
        return 'out-of-module 0x' + retAddr.toString(16);
    }
    return 'RVA 0x' + retAddr.sub(base).toInt32().toString(16);
}

const fireCounts = {};
let attachedCount = 0;

function hook(rvaOffset, label, logArgs) {
    try {
        const absAddr = rva(rvaOffset);
        Interceptor.attach(absAddr, {
            onEnter: function(args) {
                fireCounts[label] = (fireCounts[label] || 0) + 1;
                if (!logArgs) return;
                const a0 = args[0].toInt32();
                const a1 = args[1].toInt32();
                const a2 = args[2].toInt32();
                const a3 = args[3].toInt32();
                const ret = describeReturn(this.returnAddress);
                logFile('[FIRE ' + label + ' #' + fireCounts[label] + '] args=(0x' +
                    a0.toString(16) + ', 0x' + a1.toString(16) + ', 0x' +
                    a2.toString(16) + ', 0x' + a3.toString(16) + ') caller=' + ret);
            }
        });
        attachedCount++;
        logBoth('[*] hooked ' + label + ' @ RVA 0x' + rvaOffset.toString(16) +
                ' (abs 0x' + absAddr.toString(16) + ')');
    } catch (e) {
        logBoth('[!] hook ' + label + ' FAILED: ' + e.message);
    }
}

// The four candidates from the stalker probe.
hook(0x41e5f0, 'FUN_0081e5f0', true);   // top of chain — caller is handler
hook(0x27a5f0, 'FUN_0067a5f0', true);
hook(0x2fa3c0, 'FUN_006fa3c0', true);
hook(0x2fa3e0, 'FUN_006fa3e0', true);   // fmod queue — also log its callers

// Sanity hook: per-frame fn at 0x471D10 — fires every frame. Also
// doubles as periodic-summary driver (Frida has no setInterval).
let sanityFrames = 0;
let lastDumpFrame = 0;
try {
    Interceptor.attach(rva(0x471d10), {
        onLeave: function() {
            sanityFrames++;
            if (sanityFrames - lastDumpFrame < 300) return;
            lastDumpFrame = sanityFrames;
            const parts = ['[SANITY] per-frame=' + sanityFrames];
            for (const k in fireCounts) parts.push(k + '=' + fireCounts[k]);
            logFile(parts.join('  '));
        }
    });
    attachedCount++;
    logBoth('[*] hooked PER_FRAME_SANITY @ RVA 0x471D10');
} catch (e) {
    logBoth('[!] sanity hook FAILED: ' + e.message);
}

logBoth('[*] Attached ' + attachedCount + ' hooks total.');

logBoth('[*] Ready. Title screen idle, press Down ONCE. Ctrl+C when done.');
