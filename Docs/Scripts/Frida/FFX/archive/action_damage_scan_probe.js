// @desc Find ability-ID offset by scanning actor struct at damage time.
//
// PRIOR PROBE RESULTS (session 24, action_diff_probe):
// Diff between ACTION START (dfb=4) and ACTION END (dfb=0) reliably
// shows the same ~6 bytes changing (state flags at +0xDE4, +0xDE9,
// +0xDF0, +0xDFB, +0xE0D, +0xE0F). No ability-specific field in the
// +0xD80..+0xF00 window. Conclusion: either the ability ID is outside
// that window, OR it's cleared before action end.
//
// NEW APPROACH — scan at DAMAGE TIME (dfb=5, execution peak):
//   1. On damage-display fire, find the acting actor (any dfb in {4,5}
//      or df0 == 1). This is the caster.
//   2. Scan the ENTIRE actor struct (+0x0..+0xF90) as u16 values.
//      For each value that resolves to a name via the command or item
//      ExcelFile, log the offset + value + name.
//   3. Also follow actor[+0xF8C] to the ATEL script object and scan
//      its first 256 bytes the same way.
//   4. Cross-reference: user tells me what ability was used in each
//      damage event, I look for the offset that consistently holds
//      the matching name across different abilities.
//
// Also logs the full acting-actor state byte panel (df0, dfa, dfb,
// dfc, dfd, dfe) — because DMG #2 and #5 in the prior probe had
// acting=[] (no actor matched our predicate); that data will show
// what state the "hidden" caster is in.
//
// Pre-flight:
//   1. Pre-battle save → battle.
//   2. run_frida.bat → 1 → action_damage_scan_probe.
//   3. Cast a mix of KNOWN abilities. WRITE DOWN what you cast in what
//      order so we can correlate. E.g.:
//         1) Tidus Attack
//         2) Lulu Fire
//         3) Yuna Cure
//         4) Rikku Steal
//         5) let enemy hit you
//   4. Ctrl+C. Paste the full [DMG ... SCAN] blocks + your ability
//      list.
//
// Output: Docs/Scripts/Frida/output/FFX/action_damage_scan_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_damage_scan_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
let consoleBudget = 15;
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

logBoth('[*] action_damage_scan_probe  base=0x' + base.toString(16));

const RVA_DAMAGE      = 0x39FA20;
const RVA_FRAME       = 0x471D10;
const RVA_ALLY_PTR    = 0xD334CC;
const RVA_BTL_STATE   = 0xD2A8E0;
const RVA_CMD_BIN     = 0xD2A92C;
const RVA_ITEM_BIN    = 0xD2A940;
const ALLY_STRIDE     = 0xF90;
const NUM_ACTORS      = 0x1f;

// ── AOB ──
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
            logFile('[AOB ' + label + '] OK unique');
        } else {
            logFile('[AOB ' + label + '] ! ' + m.length + ' matches');
        }
    } catch (e) { logFile('[AOB ' + label + '] scan err: ' + e.message); }
}
verifyAOB('damage', RVA_DAMAGE, 20);

// ── Helpers (same charset + ExcelFile resolver as prior probes) ──
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

// ── "Acting" predicate (wider than before — include more state values) ──
function actorState(actor) {
    try {
        return {
            df0: actor.add(0xDF0).readU8(),
            dfa: actor.add(0xDFA).readU8(),
            dfb: actor.add(0xDFB).readU8(),
            dfc: actor.add(0xDFC).readU8(),
            dfd: actor.add(0xDFD).readU8(),
            dfe: actor.add(0xDFE).readU8()
        };
    } catch (e) { return null; }
}
function isActingAny(s) {
    if (!s) return false;
    // Prior probe: dfb in {4,5} OR df0 == 1 was the known "acting" set.
    // Widen here in case DoT/counter paths use different values.
    return (s.dfb >= 3 && s.dfb <= 5) || s.df0 === 1 || s.df0 === 3;
}

// ── Scan actor struct + script object, return list of u16 hits ──
function scanForCmdHits(addr, from, to, label) {
    const hits = [];
    for (let off = from; off < to; off += 2) {
        try {
            const v = addr.add(off).readU16();
            const r = resolveCandidate(v);
            if (r) hits.push({ off: off, v: v, name: r.name, src: r.source, region: label });
        } catch (e) { break; }
    }
    return hits;
}

// ── Damage hook: the core scan ──
let damageCount = 0;
try {
    Interceptor.attach(rva(RVA_DAMAGE), {
        onEnter: function (args) {
            if (!inBattle()) return;
            damageCount++;
            const target = args[0].toInt32() & 0xFF;
            const dmgType = args[1].toInt32();
            const value = args[2].toInt32();

            // Find acting actor(s).
            const acting = [];
            for (let slot = 0; slot < NUM_ACTORS; slot++) {
                const a = getActor(slot);
                if (!a) continue;
                const s = actorState(a);
                if (isActingAny(s)) {
                    acting.push({ slot: slot, actor: a, state: s });
                }
            }

            const actingStr = acting.map(function (x) {
                const nm = readActorName(x.slot) || ('#' + x.slot);
                return x.slot + ':' + nm + '(df0=' + x.state.df0 + ' dfb=' + x.state.dfb + ')';
            }).join(', ');

            logBoth('[DMG #' + damageCount + '] target=' + target +
                ' type=' + dmgType + ' value=' + value +
                ' acting=[' + actingStr + ']');

            // If acting is empty, the DoT/counter path fired. Dump
            // state bytes for every actor so we can identify the path.
            if (acting.length === 0) {
                logFile('    (no actor in "acting" state — dumping all non-idle)');
                for (let slot = 0; slot < NUM_ACTORS; slot++) {
                    const a = getActor(slot);
                    if (!a) continue;
                    const s = actorState(a);
                    if (!s) continue;
                    if (s.df0 === 0 && s.dfa === 0 && s.dfb === 0 && s.dfc === 0 && s.dfd === 0 && s.dfe === 0) continue;
                    const nm = readActorName(slot) || ('#' + slot);
                    logFile('      slot=' + slot + ' ' + nm +
                        '  df0=' + s.df0 + ' dfa=' + s.dfa + ' dfb=' + s.dfb +
                        ' dfc=' + s.dfc + ' dfd=' + s.dfd + ' dfe=' + s.dfe);
                }
                return;
            }

            // For each acting actor, scan full struct + script object.
            for (const x of acting) {
                const hits = scanForCmdHits(x.actor, 0, 0xF90, 'actor');
                // De-dup: same (off, v) collapse. Actor structs often
                // contain menu lists where the same ability appears in
                // several slots — we log each once.
                const seen = {};
                const uniq = hits.filter(function (h) {
                    const k = h.off + ':' + h.v;
                    if (seen[k]) return false;
                    seen[k] = true;
                    return true;
                });

                logFile('    actor slot=' + x.slot + ' total u16 hits=' + uniq.length);
                // Sort by offset for readability.
                uniq.sort(function (a, b) { return a.off - b.off; });
                for (const h of uniq) {
                    logFile('      ' + h.region + '+0x' + h.off.toString(16) +
                        ' = 0x' + h.v.toString(16) + ' "' + h.name + '" (' + h.src + ')');
                }

                // Script object scan.
                try {
                    const scriptPtr = x.actor.add(0xF8C).readPointer();
                    if (!scriptPtr.isNull() && scriptPtr.compare(ptr(0x10000)) > 0) {
                        const sHits = scanForCmdHits(scriptPtr, 0, 0x100, 'scriptObj');
                        const sSeen = {};
                        const sUniq = sHits.filter(function (h) {
                            const k = h.off + ':' + h.v;
                            if (sSeen[k]) return false;
                            sSeen[k] = true;
                            return true;
                        });
                        sUniq.sort(function (a, b) { return a.off - b.off; });
                        logFile('      scriptPtr=0x' + scriptPtr.toString(16) + ' hits=' + sUniq.length);
                        for (const h of sUniq) {
                            logFile('        ' + h.region + '+0x' + h.off.toString(16) +
                                ' = 0x' + h.v.toString(16) + ' "' + h.name + '" (' + h.src + ')');
                        }
                    }
                } catch (e) {}
            }
        }
    });
    logBoth('[*] hooked damage @ 0x' + RVA_DAMAGE.toString(16));
} catch (e) { logBoth('[!] damage hook FAILED: ' + e.message); }

// ── Frame sanity ──
let frames = 0;
try {
    Interceptor.attach(rva(RVA_FRAME), {
        onLeave: function () {
            frames++;
            if (frames % 300 === 0) {
                logFile('[HEARTBEAT] frames=' + frames + ' damages=' + damageCount +
                    ' inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked frame (sanity)');
} catch (e) { logBoth('[!] frame hook FAILED: ' + e.message); }

logBoth('[*] Ready. WRITE DOWN each ability you cast, in order. Ctrl+C when done.');
