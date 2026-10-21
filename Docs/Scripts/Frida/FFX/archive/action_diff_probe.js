// @desc Find the ability-ID offset via actor-struct diff across actions.
//
// CONTEXT (from action_start_probe, session 23):
// FUN_00787EC0 dispatcher + actor-slot + name resolution all confirmed
// working. User saw slot=10 "Ixion" and slot=21 "Aerouge" correctly
// identified as actors. BUT the u16-scan for ability-IDs hit dozens of
// offsets — every actor struct embeds the full command/ability list
// (Flare Fury, Sleep Attack, Cura, etc.), so scanning surfaces all of
// them plus coincidental pointer-low-halves. Noise, not signal.
//
// User also summoned Ixion but the SUMMONER's action (Yuna's "Summon
// Aeon" command) never registered. Hypothesis: summoner transitions
// through +0xDF0 instead of +0xDFB, or through a completely different
// dispatcher. This probe widens state-byte coverage to catch it.
//
// NEW APPROACH — diff-based discovery:
//   1. On ACTION START (any monitored state byte transitions into
//      "acting"), snapshot the actor's full relevant struct region
//      (+0xC00..+0xF90, 912 bytes) as a baseline.
//   2. On ACTION END (state transitions back out), snapshot again and
//      diff against baseline. Log every byte that changed with its
//      offset + old + new. Any u16 that decoded to ExcelFile text,
//      also mark with the name.
//   3. Across multiple actions, the offset(s) that consistently change
//      and match the executed ability are the real "queued ability"
//      field. The user + I correlate manually after one or two probe
//      runs.
//
// Also hooks MsDamageDisplay (RVA 0x39FA20) to pinpoint the caster at
// damage time: list every actor currently in an "acting" state byte.
// Cross-correlates with the diff data.
//
// Pre-flight: pre-battle save → battle → run probe → do a mix of
// actions (Attack, spell, item, summon, let enemy act). Ctrl+C when
// 3+ actions have executed. Paste the [ACTION END diff] lines.
//
// Output: Docs/Scripts/Frida/output/FFX/action_diff_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_diff_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
let consoleBudget = 20;
function logBoth(msg) {
    if (consoleBudget > 0) { console.log(msg); consoleBudget--; }
    logFile(msg);
}

// ── Module + RVAs ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
function rva(off) { return base.add(off); }

logBoth('[*] action_diff_probe  module=' + gameModule.name + '  base=0x' + base.toString(16));

const RVA_DISPATCHER  = 0x387EC0;
const RVA_DAMAGE      = 0x39FA20;
const RVA_FRAME       = 0x471D10;
const RVA_ALLY_PTR    = 0xD334CC;
const RVA_BTL_STATE   = 0xD2A8E0;
const RVA_CMD_BIN     = 0xD2A92C;
const RVA_ITEM_BIN    = 0xD2A940;
const ALLY_STRIDE     = 0xF90;
const NUM_ACTORS      = 0x1f;

// Region to diff. +0xD80..+0xE80 covers the action/state block; extend
// up to +0xF00 for the script-handle fields. 384 bytes per actor.
const DIFF_FROM = 0xD80;
const DIFF_TO   = 0xF00;
const DIFF_LEN  = DIFF_TO - DIFF_FROM;

// ── AOB validation (mandatory) ──
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
verifyAOB('damage',     RVA_DAMAGE,     20);

// ── Helpers ──
function inBattle() {
    try { return rva(RVA_BTL_STATE).readU8() !== 0; } catch (e) { return false; }
}
function getActor(slot) {
    try {
        if (slot >= NUM_ACTORS) return null;
        const p = rva(RVA_ALLY_PTR).readPointer();
        if (p.isNull() || p.compare(ptr(0x10000)) < 0) return null;
        return p.add(slot * ALLY_STRIDE);
    } catch (e) { return null; }
}

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
function readActorName(slot) {
    const a = getActor(slot);
    if (!a) return null;
    return decodeFFXFirstSeg(a.add(0x540), 40);
}

// ── State cache ──
// We monitor BOTH +0xDF0 and +0xDFB because the Summon command may
// transition a different state byte than regular actions. Widen the
// "acting" predicate: true if +0xDFB in {4,5} OR +0xDF0 == 1 (the
// value documented by FUN_00791230 as "acting").
function readState(actor) {
    try {
        return {
            df0: actor.add(0xDF0).readU8(),
            dfb: actor.add(0xDFB).readU8()
        };
    } catch (e) { return null; }
}
function isActing(s) {
    if (!s) return false;
    return (s.dfb === 4 || s.dfb === 5 || s.df0 === 1);
}

const prevSt = new Array(NUM_ACTORS);   // per-slot previous state struct
const baseline = new Array(NUM_ACTORS); // per-slot Uint8Array baseline snapshot
for (let i = 0; i < NUM_ACTORS; i++) { prevSt[i] = null; baseline[i] = null; }

let actionStartCount = 0;
let actionEndCount = 0;
let damageCount = 0;

// Snapshot actor+DIFF_FROM..DIFF_TO as a plain Uint8Array copy.
function snapshot(actor) {
    try {
        const raw = actor.add(DIFF_FROM).readByteArray(DIFF_LEN);
        if (!raw) return null;
        return new Uint8Array(raw).slice();  // detach from live memory
    } catch (e) { return null; }
}

// Compare old vs new, return list of {off, oldVal, newVal} byte diffs
// inside DIFF_FROM..DIFF_TO. Groups adjacent same-byte changes into
// u16 pairs for cleaner reporting when offset is even and newVal looks
// like a plausible command ID.
function diff(oldSnap, newSnap) {
    const out = [];
    if (!oldSnap || !newSnap) return out;
    for (let i = 0; i < DIFF_LEN; i++) {
        if (oldSnap[i] !== newSnap[i]) {
            out.push({ off: DIFF_FROM + i, oldVal: oldSnap[i], newVal: newSnap[i] });
        }
    }
    return out;
}

// On diff emission, also try to interpret adjacent pairs as u16 and
// resolve via ExcelFile. The user reads the log and looks for ONE
// offset whose u16 value decodes to the ability they just executed.
function logDiff(diffs, actor) {
    if (diffs.length === 0) { logFile('    (no diffs)'); return; }
    // Group into runs of contiguous offsets
    const runs = [];
    let cur = null;
    for (const d of diffs) {
        if (cur && d.off === cur.to) {
            cur.to = d.off + 1;
            cur.bytes.push({ oldVal: d.oldVal, newVal: d.newVal });
        } else {
            cur = { from: d.off, to: d.off + 1, bytes: [{ oldVal: d.oldVal, newVal: d.newVal }] };
            runs.push(cur);
        }
    }
    logFile('    changed bytes: ' + diffs.length + ' total, ' + runs.length + ' runs');
    for (const r of runs) {
        const oldHex = r.bytes.map(function (b) {
            let h = b.oldVal.toString(16); if (h.length === 1) h = '0' + h; return h;
        }).join(' ');
        const newHex = r.bytes.map(function (b) {
            let h = b.newVal.toString(16); if (h.length === 1) h = '0' + h; return h;
        }).join(' ');
        let resolved = '';
        // If the run is 2 bytes or the first two bytes of a longer run
        // look like a u16 that resolves, flag it.
        if (r.bytes.length >= 2) {
            const u16 = r.bytes[0].newVal | (r.bytes[1].newVal << 8);
            const r2 = resolveCandidate(u16);
            if (r2) resolved = ' ← u16 0x' + u16.toString(16) + ' "' + r2.name + '" (' + r2.source + ')';
        }
        logFile('      +0x' + r.from.toString(16) + ' (' + r.bytes.length + 'b)  ' +
            oldHex + ' → ' + newHex + resolved);
    }
}

// ── Dispatcher hook: action-start + action-end detection ──
try {
    Interceptor.attach(rva(RVA_DISPATCHER), {
        onEnter: function () {
            if (!inBattle()) return;
            for (let slot = 0; slot < NUM_ACTORS; slot++) {
                const actor = getActor(slot);
                if (!actor) continue;
                const curSt = readState(actor);
                if (!curSt) continue;
                const prior = prevSt[slot];
                prevSt[slot] = curSt;

                const wasActing = prior ? isActing(prior) : false;
                const nowActing = isActing(curSt);

                if (!wasActing && nowActing) {
                    actionStartCount++;
                    const name = readActorName(slot) || ('actor#' + slot);
                    const scriptPtr = actor.add(0xF8C).readU32();
                    logBoth('[ACTION START #' + actionStartCount + '] slot=' + slot +
                        ' "' + name + '" df0=' + curSt.df0 + ' dfb=' + curSt.dfb +
                        (prior ? ' (prev df0=' + prior.df0 + ' dfb=' + prior.dfb + ')' : '') +
                        ' scriptPtr=0x' + scriptPtr.toString(16));
                    baseline[slot] = snapshot(actor);
                }

                if (wasActing && !nowActing) {
                    actionEndCount++;
                    const name = readActorName(slot) || ('actor#' + slot);
                    logBoth('[ACTION END #' + actionEndCount + '] slot=' + slot +
                        ' "' + name + '" df0=' + curSt.df0 + ' dfb=' + curSt.dfb);
                    const after = snapshot(actor);
                    const diffs = diff(baseline[slot], after);
                    logDiff(diffs, actor);
                    baseline[slot] = null;
                }
            }
        }
    });
    logBoth('[*] hooked dispatcher @ 0x' + RVA_DISPATCHER.toString(16));
} catch (e) { logBoth('[!] dispatcher hook FAILED: ' + e.message); }

// ── Damage hook: cross-reference caster ──
try {
    Interceptor.attach(rva(RVA_DAMAGE), {
        onEnter: function (args) {
            if (!inBattle()) return;
            damageCount++;
            const chrId = args[0].toInt32() & 0xFF;
            const dmgType = args[1].toInt32();
            const value = args[2].toInt32();
            // Find who's currently acting (any state byte predicate).
            const actors = [];
            for (let slot = 0; slot < NUM_ACTORS; slot++) {
                const a = getActor(slot);
                if (!a) continue;
                const s = readState(a);
                if (isActing(s)) {
                    const nm = readActorName(slot) || ('actor#' + slot);
                    actors.push(slot + ':' + nm + '(df0=' + s.df0 + ',dfb=' + s.dfb + ')');
                }
            }
            logBoth('[DMG #' + damageCount + '] target=' + chrId +
                ' type=' + dmgType + ' value=' + value +
                '  acting=[' + actors.join(', ') + ']');
        }
    });
    logBoth('[*] hooked damage @ 0x' + RVA_DAMAGE.toString(16));
} catch (e) { logBoth('[!] damage hook FAILED: ' + e.message); }

// ── Frame sanity heartbeat ──
let frameFires = 0;
try {
    Interceptor.attach(rva(RVA_FRAME), {
        onLeave: function () {
            frameFires++;
            if (frameFires % 300 === 0) {
                logFile('[HEARTBEAT] frames=' + frameFires +
                    '  starts=' + actionStartCount +
                    '  ends=' + actionEndCount +
                    '  damage=' + damageCount +
                    '  inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked frame (sanity)');
} catch (e) { logBoth('[!] frame hook FAILED: ' + e.message); }

logBoth('[*] Ready. Pre-battle save → battle → mix of actions → Ctrl+C.');
