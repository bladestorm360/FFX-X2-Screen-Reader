// @desc Discovery probe for ally/enemy action announcement (actor + ability).
//
// Goal: announce "<actor name>: <command name>" when a combatant performs
// an action in battle (e.g. "Gold Element: Thundara", "Tidus: Attack").
// Damage is already announced after the fact — we need the action name
// BEFORE it resolves.
//
// Candidate hooks (from Ghidra decompile layer2_game_logic.txt):
//   0x391230  FUN_00791230 — ally dispatcher (loop, iVar4<0x1f). Fires
//                            every frame per ally, NOT hooked here (too
//                            noisy). We rely on its callee instead.
//   0x393420  FUN_00793420(actor_id, action_type_flag=0xC) — ACTION EXECUTE.
//                            Both ally (case 6 of dispatcher) and enemy
//                            actions route through this. Primary target.
//   0x394030  FUN_00794030(slot) — actor-object lookup. Replicated inline
//                            below (no NativeFunction call) — safer across
//                            calling-convention assumptions.
//   0x396DD0  FUN_00796DD0(slot, actor_ptr, flags) — teardown helper
//                            called inside FUN_00793420. Kept for logs.
//
// Actor arrays (from FUN_00794030 decompile + FUN_00793540 allocations):
//   Allies : [ptr at RVA 0xD334CC] + slot * 0xF90,  31 slots (0..30)
//   Enemies: [ptr at RVA 0xD34464] + (slot-0x1F) * 0x390, 62 slots (31..92)
//
// What the probe does:
//   1. Resolves RVAs, validates each against an AOB pattern built from the
//      first 20 bytes at the RVA. If pattern match != RVA or finds >1
//      match, logs a warning but still tries the RVA.
//   2. Hooks 0x393420 onEnter: logs (actor_id, action_type, actor_ptr,
//      64-byte dump around offset 0xDE0, and tries to resolve every u16
//      in that window through the command/item ExcelFile walk). Any
//      "hit" (u16 that resolves to a plausible name string) is flagged.
//   3. Hooks 0x396DD0 onEnter + onLeave: logs args and return.
//   4. Console output is capped at 20 unique "fires" to keep the screen
//      reader safe. Full detail goes to the file log.
//
// Test protocol:
//   1. Load a save just before a mandatory battle (quicksave inside
//      battle works too — the probe attaches mid-battle).
//   2. run_frida.bat -> 1 (FFX) -> action_announce_probe.
//   3. Perform: Tidus Attack. Enemy Attack. Tidus Magic (Fire). Use item.
//   4. Ctrl+C to stop. Paste first ~50 lines of the log back in chat.
//
// Output: Docs/Scripts/Frida/output/FFX/action_announce_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_announce_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }

let consoleBudget = 20;
function logBoth(msg) {
    if (consoleBudget > 0) { console.log(msg); consoleBudget--; }
    logFile(msg);
}

// ── Module resolution (explicit by name; enumerateModules order is not guaranteed) ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(off) { return base.add(off); }

logBoth('[*] action_announce_probe  module=' + gameModule.name +
        '  base=0x' + base.toString(16));

// ── RVAs of interest ──
const RVA_ACTION_EXEC = 0x393420;  // FUN_00793420
const RVA_CLEANUP     = 0x396DD0;  // FUN_00796DD0
const RVA_ALLY_PTR    = 0xD334CC;  // DAT_011334cc (ptr-to-ally-array)
const RVA_ENEMY_PTR   = 0xD34464;  // DAT_01134464 (ptr-to-enemy-array)
const RVA_BTL_STATE   = 0xD2A8E0;  // btlBase + 0x10
const RVA_CMD_BIN     = 0xD2A92C;  // btlBase + 0x5C
const RVA_ITEM_BIN    = 0xD2A940;  // btlBase + 0x70
const ALLY_STRIDE     = 0xF90;
const ENEMY_STRIDE    = 0x390;

// ── AOB pattern validation ──
// Per project rule: hooks MUST include an AOB fallback. For a probe we
// compute the pattern from the first ~20 bytes at the config RVA, then
// Memory.scanSync the module text to verify exactly one match at the
// expected address. If mismatch, still try the RVA but log loud.
function patternAt(r, nBytes) {
    try {
        const bytes = rva(r).readByteArray(nBytes);
        if (!bytes) return null;
        const view = new Uint8Array(bytes);
        const parts = [];
        for (let i = 0; i < view.length; i++) {
            let h = view[i].toString(16);
            if (h.length === 1) h = '0' + h;
            parts.push(h);
        }
        return parts.join(' ');
    } catch (e) { return null; }
}
function verifyAOB(label, r, nBytes) {
    const pat = patternAt(r, nBytes);
    if (!pat) { logFile('[AOB ' + label + '] could not read at RVA 0x' + r.toString(16)); return; }
    logFile('[AOB ' + label + '] rva=0x' + r.toString(16) + '  pattern=' + pat);
    try {
        const matches = Memory.scanSync(base, gameModule.size, pat);
        if (matches.length === 1) {
            const found = matches[0].address.sub(base).toInt32();
            if (found === r) logFile('[AOB ' + label + '] OK, unique match at expected RVA');
            else logFile('[AOB ' + label + '] ! unique match at 0x' + found.toString(16) + ' (expected 0x' + r.toString(16) + ')');
        } else {
            logFile('[AOB ' + label + '] ! found ' + matches.length + ' matches; RVA may be stale');
        }
    } catch (e) { logFile('[AOB ' + label + '] scan failed: ' + e.message); }
}
verifyAOB('action_exec', RVA_ACTION_EXEC, 20);
verifyAOB('cleanup',     RVA_CLEANUP,     20);

// ── FFX charset decoder (subset of src/core/charset.js; probes can't import) ──
const CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz{|}~';
function safeReadBytes(addr, len) {
    try {
        if (addr.isNull() || addr.compare(ptr(0x10000)) < 0) return null;
        return addr.readByteArray(len);
    } catch (e) { return null; }
}
function decodeFFXFirstSegment(addr, maxLen) {
    const raw = safeReadBytes(addr, maxLen);
    if (!raw) return '';
    const bytes = new Uint8Array(raw);
    let result = '';
    let i = 0;
    while (i < maxLen) {
        let b = bytes[i];
        if (b === 0x00 || b === 0x03 || b === 0x0A) {
            if (result.length > 0) return result.trim();
            i++; continue;
        }
        if (b === 0x09 || b === 0x10 || b === 0x13) { i += 2; continue; }
        if (b < 0x2B) { i++; continue; }
        let idx = 0;
        if (b < 0x30) { idx = (b - 0x2B) * 0xD0; i++; if (i >= maxLen) break; b = bytes[i]; }
        idx += (b - 0x30);
        if (idx >= 0 && idx < CHARSET.length) result += CHARSET[idx];
        i++;
    }
    return result.trim();
}

// ── ExcelFile command/item text resolver (matches src/core/charset.js:92) ──
function resolveCommandText(commandId, binAddrRva) {
    try {
        const binAddr = rva(binAddrRva);
        const excelPtr = binAddr.readPointer();
        if (excelPtr.isNull() || excelPtr.compare(ptr(0x10000)) < 0) return null;
        const hdr = excelPtr.add(0x8);
        const firstIdx = hdr.readU16();
        const lastIdx = hdr.add(2).readU16();
        const elementSize = hdr.add(4).readU16();
        const dataLength = hdr.add(6).readU16();
        const dataStart = hdr.add(8).readU32();
        if (dataStart === 0 || dataStart > 0x100000) return null;
        if (elementSize === 0 || elementSize > 0x1000) return null;
        const excelIdx = commandId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;
        const elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        const nameOffset = elementAddr.readU16();
        const textAddr = excelPtr.add(dataStart + dataLength + nameOffset);
        const s = decodeFFXFirstSegment(textAddr, 64);
        return (s && s.length > 0) ? s : null;
    } catch (e) { return null; }
}

// Resolve an arbitrary u16 "candidate" to a name string — tries command
// bin then item bin. Returns { name, source } or null. Used to sniff out
// which offset in the actor object holds the queued ability ID.
function resolveCandidate(cmdId) {
    if (cmdId === 0 || cmdId === 0xFFFF) return null;
    let n = resolveCommandText(cmdId, RVA_CMD_BIN);
    if (n) return { name: n, source: 'cmd' };
    n = resolveCommandText(cmdId, RVA_ITEM_BIN);
    if (n) return { name: n, source: 'item' };
    return null;
}

// ── Actor-object lookup (inline replication of FUN_00794030) ──
function getActor(slot) {
    try {
        slot = slot & 0xFF;
        if (slot < 0x1f) {
            const p = rva(RVA_ALLY_PTR).readPointer();
            if (p.isNull() || p.compare(ptr(0x10000)) < 0) return null;
            return { ptr: p.add(slot * ALLY_STRIDE), team: 'ally', stride: ALLY_STRIDE };
        }
        if (slot < 0x5d) {
            const p = rva(RVA_ENEMY_PTR).readPointer();
            if (p.isNull() || p.compare(ptr(0x10000)) < 0) return null;
            return { ptr: p.add((slot - 0x1f) * ENEMY_STRIDE), team: 'enemy', stride: ENEMY_STRIDE };
        }
    } catch (e) {}
    return null;
}

// Ally actor name lives at +0x540 (CHR_RAM_OFFSET) in the 0xF90 struct.
// Enemy-struct layout at 0x390 bytes is different and undocumented; the
// existing mod reads enemy names via a DIFFERENT array (`monsterChrsAddr`
// at RVA 0xD34460, stride 0xF90). For probe purposes, we try both.
function readActorName(slot) {
    try {
        if (slot < 0x1f) {
            const p = rva(RVA_ALLY_PTR).readPointer();
            if (p.isNull()) return null;
            const namePtr = p.add(slot * ALLY_STRIDE + 0x540);
            const s = decodeFFXFirstSegment(namePtr, 40);
            if (s && s.length > 0) return s;
        } else if (slot < 0x5d) {
            // Try the display array used by the existing mod:
            //   monsterChrsAddr at RVA 0xD34460, stride 0xF90, slot-relative.
            const p = rva(0xD34460).readPointer();
            if (p.isNull()) return null;
            const monIdx = slot - 0x1f;
            const namePtr = p.add(monIdx * ALLY_STRIDE + 0x540);
            const s = decodeFFXFirstSegment(namePtr, 40);
            if (s && s.length > 0) return s;
        }
    } catch (e) {}
    return null;
}

// ── Action-executor hook (primary target) ──
// Scan a window of u16 values in the actor struct looking for plausible
// command/item IDs. This is the discovery step — once we know which
// offset holds the queued action, the production port uses that offset
// directly.
function scanActorForCommandId(actorPtr) {
    const hits = [];
    try {
        const scanFrom = 0xD80;
        const scanTo = 0xE80;
        for (let off = scanFrom; off < scanTo; off += 2) {
            const v = actorPtr.add(off).readU16();
            const r = resolveCandidate(v);
            if (r) hits.push({ off: off, id: v, name: r.name, source: r.source });
        }
    } catch (e) {}
    return hits;
}

// Hex-dump a byte range from the actor object, for the log.
function dumpBytes(actorPtr, from, len) {
    try {
        const raw = actorPtr.add(from).readByteArray(len);
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

let fireCount = 0;
try {
    Interceptor.attach(rva(RVA_ACTION_EXEC), {
        onEnter: function (args) {
            fireCount++;
            const btlState = (function () {
                try { return rva(RVA_BTL_STATE).readU8(); } catch (e) { return 0; }
            })();
            const actorId = args[0].toInt32() & 0xFF;
            const actionFlag = args[1].toInt32() & 0xFF;
            const actor = getActor(actorId);
            const actorPtr = actor ? actor.ptr : null;
            const team = actor ? actor.team : '?';
            const name = readActorName(actorId) || ('actor#' + actorId);

            // First pass (console + file): compact summary line
            const hits = actorPtr ? scanActorForCommandId(actorPtr) : [];
            let hitStr = '';
            if (hits.length > 0) {
                const first = hits[0];
                hitStr = '  cmd=0x' + first.id.toString(16) +
                    ' "' + first.name + '" (' + first.source +
                    ' @+0x' + first.off.toString(16) + ')';
                if (hits.length > 1) hitStr += ' (+' + (hits.length - 1) + ' more)';
            }
            logBoth('[ACT #' + fireCount + '] ' + team + ' slot=' + actorId +
                ' name="' + name + '" flag=0x' + actionFlag.toString(16) + hitStr +
                (btlState === 0 ? '  (battle_state=0)' : ''));

            // Detailed (file only): all hits + hex dump of the interesting window
            if (actorPtr) {
                for (let i = 0; i < hits.length; i++) {
                    const h = hits[i];
                    logFile('    hit: +0x' + h.off.toString(16) + ' = 0x' +
                        h.id.toString(16) + ' "' + h.name + '" (' + h.source + ')');
                }
                logFile('    dump @+0xDE0..+0xE20 : ' + dumpBytes(actorPtr, 0xDE0, 0x40));
            }
        }
    });
    logBoth('[*] hooked action_exec @ RVA 0x' + RVA_ACTION_EXEC.toString(16));
} catch (e) {
    logBoth('[!] action_exec hook FAILED: ' + e.message);
}

// ── Cleanup helper hook (supplementary) ──
let cleanupFires = 0;
try {
    Interceptor.attach(rva(RVA_CLEANUP), {
        onEnter: function (args) {
            cleanupFires++;
            this.slot = args[0].toInt32() & 0xFF;
            this.actorArg = args[1];
            this.flags = args[2].toInt32();
            // File-only — this fires often and isn't the announce source.
            if (cleanupFires <= 80) {
                logFile('[CLEANUP #' + cleanupFires + '] slot=' + this.slot +
                    ' actor=0x' + this.actorArg.toString(16) +
                    ' flags=0x' + this.flags.toString(16));
            }
        },
        onLeave: function (retval) {
            if (cleanupFires <= 80) {
                logFile('    ret=0x' + retval.toInt32().toString(16));
            }
        }
    });
    logBoth('[*] hooked cleanup @ RVA 0x' + RVA_CLEANUP.toString(16));
} catch (e) {
    logBoth('[!] cleanup hook FAILED: ' + e.message);
}

logBoth('[*] Ready. Reach battle, perform ally + enemy actions, then Ctrl+C.');
