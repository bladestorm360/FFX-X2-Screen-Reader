// @desc Lock in FUN_00787EC0 as the action dispatcher; find the ability-ID offset.
//
// CONTEXT (from action_backtrace_probe, session 22):
// Damage stack on every attack threads through FUN_00787EC0 (RVA 0x387EC0,
// 285 bytes). Its body iterates actor slots 0..0x1E, and for each actor
// whose state byte at +0xDFB is 4 or 5, fetches a vtable at actor[+0xF8C]
// and invokes its +0xC slot — that's the ATEL script execution of the
// queued ability. So FUN_00787EC0 fires once per frame while an action
// is in progress, NOT once per action start. To announce "<actor>:
// <ability>" exactly once per action we need to detect the *transition*
// into state 4/5 on each actor.
//
// This probe:
//   1. AOB-validates FUN_00787EC0 and FUN_00794030 (actor-lookup).
//   2. Hooks FUN_00787EC0 onEnter. Inside, walks actor slots 0..0x1E,
//      reads each actor's +0xDFB byte, and compares to a per-slot cache.
//      On any 0→{4,5} or x→{4,5} transition, logs "[ACTION START]".
//   3. For each action-start: dumps (a) the actor's script-pointer field
//      at +0xF8C, (b) 128 bytes around the actor's queued-command region
//      (+0xD80..+0xE00), (c) scans that region for u16 values that
//      resolve to command/item ExcelFile text → these are candidate
//      ability-ID offsets.
//   4. Also tracks state exits ({4,5}→other) to confirm the action
//      finished, so we can verify the edge detection is clean.
//   5. Heartbeat every 300 frames to confirm probe is live.
//
// Pre-flight: see action_backtrace_probe.js header (same protocol —
// pre-battle save, ally actions until enemies strike, paste log).
//
// Output: Docs/Scripts/Frida/output/FFX/action_start_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_start_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }

let consoleBudget = 25;
function logBoth(msg) {
    if (consoleBudget > 0) { console.log(msg); consoleBudget--; }
    logFile(msg);
}

// ── Module + RVA helpers ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
function rva(off) { return base.add(off); }

logBoth('[*] action_start_probe  module=' + gameModule.name + '  base=0x' + base.toString(16));

// ── RVAs ──
const RVA_DISPATCHER  = 0x387EC0;   // FUN_00787EC0 — action dispatcher
const RVA_ALLY_PTR    = 0xD334CC;   // ptr to ally actor array (inline repl of FUN_00794030)
const RVA_FRAME       = 0x471D10;   // frame loop (sanity)
const RVA_BTL_STATE   = 0xD2A8E0;   // battle state byte
const RVA_CMD_BIN     = 0xD2A92C;   // command ExcelFile ptr
const RVA_ITEM_BIN    = 0xD2A940;   // item ExcelFile ptr
const ALLY_STRIDE     = 0xF90;

// ── AOB validation ──
function patternAt(r, n) {
    try {
        const b = rva(r).readByteArray(n);
        if (!b) return null;
        const v = new Uint8Array(b);
        const parts = [];
        for (let i = 0; i < v.length; i++) {
            let h = v[i].toString(16);
            if (h.length === 1) h = '0' + h;
            parts.push(h);
        }
        return parts.join(' ');
    } catch (e) { return null; }
}
function verifyAOB(label, r, n) {
    const pat = patternAt(r, n);
    if (!pat) { logFile('[AOB ' + label + '] unreadable'); return; }
    logFile('[AOB ' + label + '] rva=0x' + r.toString(16) + '  pattern=' + pat);
    try {
        const m = Memory.scanSync(base, gameModule.size, pat);
        if (m.length === 1 && m[0].address.sub(base).toInt32() === r) {
            logFile('[AOB ' + label + '] OK unique at expected RVA');
        } else {
            logFile('[AOB ' + label + '] ! ' + m.length + ' matches');
        }
    } catch (e) { logFile('[AOB ' + label + '] scan err: ' + e.message); }
}
verifyAOB('dispatcher', RVA_DISPATCHER, 20);

// ── Battle-state gate ──
function inBattle() {
    try { return rva(RVA_BTL_STATE).readU8() !== 0; } catch (e) { return false; }
}

// ── Actor lookup (inline FUN_00794030 for allies) ──
function getActor(slot) {
    try {
        if (slot >= 0x1f) return null;
        const p = rva(RVA_ALLY_PTR).readPointer();
        if (p.isNull() || p.compare(ptr(0x10000)) < 0) return null;
        return p.add(slot * ALLY_STRIDE);
    } catch (e) { return null; }
}

// ── FFX charset + ExcelFile command/item text resolver (same offsets as src/core/charset.js) ──
const CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz{|}~';
function decodeFFXFirstSeg(addr, max) {
    try {
        if (addr.isNull() || addr.compare(ptr(0x10000)) < 0) return '';
        const raw = addr.readByteArray(max);
        if (!raw) return '';
        const bytes = new Uint8Array(raw);
        let result = '', i = 0;
        while (i < max) {
            let b = bytes[i];
            if (b === 0x00 || b === 0x03 || b === 0x0A) {
                if (result.length > 0) return result.trim();
                i++; continue;
            }
            if (b === 0x09 || b === 0x10 || b === 0x13) { i += 2; continue; }
            if (b < 0x2B) { i++; continue; }
            let idx = 0;
            if (b < 0x30) { idx = (b - 0x2B) * 0xD0; i++; if (i >= max) break; b = bytes[i]; }
            idx += (b - 0x30);
            if (idx >= 0 && idx < CHARSET.length) result += CHARSET[idx];
            i++;
        }
        return result.trim();
    } catch (e) { return ''; }
}
function resolveCmdText(cmdId, binAddrRva) {
    try {
        const excelPtr = rva(binAddrRva).readPointer();
        if (excelPtr.isNull() || excelPtr.compare(ptr(0x10000)) < 0) return null;
        const hdr = excelPtr.add(0x8);
        const firstIdx = hdr.readU16();
        const lastIdx = hdr.add(2).readU16();
        const elSize = hdr.add(4).readU16();
        const dataLen = hdr.add(6).readU16();
        const dataStart = hdr.add(8).readU32();
        if (dataStart === 0 || dataStart > 0x100000) return null;
        if (elSize === 0 || elSize > 0x1000) return null;
        const excelIdx = cmdId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;
        const elAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elSize);
        const nameOff = elAddr.readU16();
        const txt = excelPtr.add(dataStart + dataLen + nameOff);
        const s = decodeFFXFirstSeg(txt, 64);
        return (s && s.length > 0) ? s : null;
    } catch (e) { return null; }
}
function resolveCandidate(v) {
    if (v === 0 || v === 0xFFFF) return null;
    let n = resolveCmdText(v, RVA_CMD_BIN);
    if (n) return { name: n, source: 'cmd' };
    n = resolveCmdText(v, RVA_ITEM_BIN);
    if (n) return { name: n, source: 'item' };
    return null;
}

function hexDump(addr, from, len) {
    try {
        const raw = addr.add(from).readByteArray(len);
        if (!raw) return '';
        const v = new Uint8Array(raw);
        const parts = [];
        for (let i = 0; i < v.length; i++) {
            let h = v[i].toString(16);
            if (h.length === 1) h = '0' + h;
            parts.push(h);
        }
        return parts.join(' ');
    } catch (e) { return ''; }
}

function readActorName(slot) {
    try {
        const a = getActor(slot);
        if (!a) return null;
        return decodeFFXFirstSeg(a.add(0x540), 40);
    } catch (e) { return null; }
}

// ── Per-slot state cache ──
const prevState = new Array(0x1f).fill(0);
let actionStartCount = 0;
let dispatcherFires = 0;

// ── Dispatcher hook ──
try {
    Interceptor.attach(rva(RVA_DISPATCHER), {
        onEnter: function (args) {
            dispatcherFires++;
            if (!inBattle()) return;

            // Walk ally actor slots; detect +0xDFB transitions.
            for (let slot = 0; slot < 0x1f; slot++) {
                const actor = getActor(slot);
                if (!actor) continue;
                let curState = 0;
                try { curState = actor.add(0xDFB).readU8(); } catch (e) { continue; }
                const prior = prevState[slot];
                prevState[slot] = curState;

                // Detect ENTRY into the "acting" states (4 or 5) from any other.
                const nowActing = (curState === 4 || curState === 5);
                const wasActing = (prior === 4 || prior === 5);
                if (nowActing && !wasActing) {
                    actionStartCount++;
                    const name = readActorName(slot) || ('actor#' + slot);
                    const scriptPtrRaw = actor.add(0xF8C).readU32();
                    logBoth('[ACTION START #' + actionStartCount + '] slot=' + slot +
                        ' "' + name + '" stateByte=' + curState +
                        ' prev=' + prior +
                        ' scriptPtr=0x' + scriptPtrRaw.toString(16));

                    // Scan queued-command region for ExcelFile-resolvable IDs.
                    const hits = [];
                    for (let off = 0xD80; off < 0xE40; off += 2) {
                        try {
                            const v = actor.add(off).readU16();
                            const r = resolveCandidate(v);
                            if (r) hits.push({ off: off, id: v, name: r.name, source: r.source });
                        } catch (e) {}
                    }
                    for (let i = 0; i < hits.length; i++) {
                        const h = hits[i];
                        logFile('    cmd hit: +0x' + h.off.toString(16) +
                            ' = 0x' + h.id.toString(16) +
                            ' "' + h.name + '" (' + h.source + ')');
                    }
                    logFile('    actor+0xD80..+0xE40 : ' + hexDump(actor, 0xD80, 0xC0));

                    // Also follow the script pointer; dump its first 64 bytes
                    // if it points into mapped memory.
                    try {
                        const scriptPtr = actor.add(0xF8C).readPointer();
                        if (!scriptPtr.isNull() && scriptPtr.compare(ptr(0x10000)) > 0) {
                            logFile('    scriptObj+0x00..+0x40 : ' + hexDump(scriptPtr, 0, 0x40));
                            // Try to resolve u16 values from the script object too.
                            for (let off = 0; off < 0x40; off += 2) {
                                try {
                                    const v = scriptPtr.add(off).readU16();
                                    const r = resolveCandidate(v);
                                    if (r) {
                                        logFile('    script hit: +0x' + off.toString(16) +
                                            ' = 0x' + v.toString(16) +
                                            ' "' + r.name + '" (' + r.source + ')');
                                    }
                                } catch (e) {}
                            }
                        }
                    } catch (e) {}
                }

                // Transition OUT of acting — confirms we detected the start.
                if (!nowActing && wasActing) {
                    logFile('[ACTION END] slot=' + slot + ' state ' + prior + ' -> ' + curState);
                }
            }
        }
    });
    logBoth('[*] hooked dispatcher @ 0x' + RVA_DISPATCHER.toString(16));
} catch (e) {
    logBoth('[!] dispatcher hook FAILED: ' + e.message);
}

// ── Frame sanity ──
let frameFires = 0;
try {
    Interceptor.attach(rva(RVA_FRAME), {
        onLeave: function () {
            frameFires++;
            if (frameFires % 300 === 0) {
                logFile('[HEARTBEAT] frames=' + frameFires +
                    '  dispatcher=' + dispatcherFires +
                    '  actionStarts=' + actionStartCount +
                    '  inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked frame (sanity)');
} catch (e) { logBoth('[!] frame hook FAILED: ' + e.message); }

logBoth('[*] Ready. In battle, do a few ally actions + wait for an enemy turn. Ctrl+C.');
