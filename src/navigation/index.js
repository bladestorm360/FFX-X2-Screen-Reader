// Navigation feature: map-transition detection, entity scanning, hotkey
// handling (scan / cycle / announce / pathfind), A* path announcements.

import { rva, base } from '../core/memory.js';
import { logFile, speak } from '../core/speech.js';
import { attachRva } from '../core/hooks.js';
import { register as onFrame, registerHotkey } from '../core/frame.js';
import {
    keyDown,
    VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_P, VK_SHIFT
} from '../core/input.js';
import {
    RVA_CURRENT_WORKER,
    NAV_ACTOR_SIZE, NAV_MAX_ACTORS, NAV_UNITS_PER_STEP,
    btlStateAddr
} from '../core/addresses.js';
import { isValidPtr, safeReadCString } from '../core/safeRead.js';
import {
    readNavmesh, findTri, distXZ, relativeDir,
    getActorsBase, getPlayerPos, isNearWalkable
} from './navmesh.js';
import { astar, smoothWaypoints } from './astar.js';
import { scanExits } from './exits.js';

const NAV_CATEGORIES = ['All', 'Exit', 'Event', 'Save Sphere', 'Treasure Chest', 'NPC', 'Object'];

let navEntities = [];
let navSelectedIdx = 0;
let navCategoryIdx = 0;
// Last count of fully-written actors observed in onTick. -1 sentinel
// means "no observation yet" (or "just reset by JUMP2"). When the live
// count differs from this, a new actor just landed (or the table was
// just cleared on transition) and we re-scan.
let navLastActorCount = -1;

// 0xcdcdcdcd reinterpreted as float; indicates an uninit actor slot.
const ACTOR_UNINIT_FLOAT = -431602080;

// Player-slot validity gate. Only checks slot 0 (player). Says "the engine
// has finished restoring the player position after a JUMP2/load" — does NOT
// say "all actors are loaded". Actor population continues for many frames
// after this returns true. The continuous-rescan loop in onTick() handles
// the progressive write window: it checks the actor count each frame and
// re-scans whenever the count changes. Event-driven (engine adds an actor
// → next frame the count differs → scan), no timing thresholds.
function playerSlotReady() {
    const ab = getActorsBase();
    if (!ab) return false;
    try {
        const px = ab.add(0x0C).readFloat();
        if (isNaN(px) || px === ACTOR_UNINIT_FLOAT) return false;
        const playerEn = ab.add(2).readU16();
        if (playerEn === 0 || playerEn === 0xCDCD || playerEn > 0xFF) return false;
        const playerVar1 = ab.add(0x824).readU32();
        if (playerVar1 === 0xCDCDCDCD) return false;
        return true;
    } catch (e) { return false; }
}

// Count actors whose slot is fully written (valid metadata AND non-zero
// position). Returns 0 during early load (positions still pending) and
// rises as the engine writes each slot. A delta in this count between
// frames IS the engine signal that another actor just landed — that's
// what drives the rescan in onTick().
function countLoadedActors() {
    const ab = getActorsBase();
    if (!ab) return -1;
    try {
        let count = 0;
        for (let i = 1; i < NAV_MAX_ACTORS; i++) {
            const actor = ab.add(i * NAV_ACTOR_SIZE);
            const chrId = actor.readU16();
            const en = actor.add(2).readU16();
            if (chrId === 0 || chrId === 0xCDCD || en === 0) continue;
            if (en === 0xCDCD || en > 0xFF) continue;
            const var1 = actor.add(0x824).readU32();
            if (var1 === 0xCDCDCDCD) continue;
            const x = actor.add(0x0C).readFloat();
            const z = actor.add(0x14).readFloat();
            if (isNaN(x) || isNaN(z)) continue;
            if (x === 0 && z === 0) continue;  // metadata in, position pending
            count++;
        }
        return count;
    } catch (e) { return -1; }
}

// Read the developer's debug string from chr_name (actor+0x04). FFX devs label
// each actor with an asset-name-style identifier (e.g. "chr_wakka_a", "barrel_01");
// most NPCs have no player-facing name, so the dev string is the best handle we
// have. Returns the cleaned string when printable, null otherwise. Cleanup is
// minimal: strip leading directory components and known asset extensions, but
// preserve the dev's underscores and version suffixes — the user learns the
// vocabulary, we don't translate it.
function readDevString(actor) {
    let np;
    try { np = actor.add(0x04).readPointer(); } catch (e) { return null; }
    if (!isValidPtr(np)) return null;
    const s = safeReadCString(np, 96);
    if (!s || s.length === 0) return null;
    let printable = 0;
    for (let i = 0; i < s.length; i++) {
        const c = s.charCodeAt(i);
        if (c >= 0x20 && c <= 0x7E) printable++;
    }
    if (printable / s.length < 0.85) return null;
    let cleaned = s;
    if (/[\/\\]/.test(cleaned)) cleaned = cleaned.split(/[\/\\]/).pop();
    cleaned = cleaned.replace(/\.(bin|dds|tex|ftc|mdl|tim)$/i, '').trim();
    return cleaned.length > 0 ? cleaned : null;
}

function classify(id) {
    if (id === 0x5001) return 'Save Sphere';
    if (id === 0x5002) return 'Treasure Chest';
    if (id >= 0x2000 && id < 0x5000) return 'NPC';
    if (id <= 0x0012) return 'Player';
    return 'Object';
}

function label(e) {
    // Exit/Event names already encode their category (e.g. "Exit to 0x1234");
    // don't double-prefix.
    if (e.category === 'Exit' || e.category === 'Event') return e.name || e.category;
    // Other entities: prefix the category to the dev string when readable, or
    // fall back to chrId hex so 30 unidentified entities don't all collide on
    // the bare category name.
    if (e.name) return e.category + ': ' + e.name;
    return e.category + ' 0x' + e.chrId.toString(16);
}

function getFiltered() {
    if (navCategoryIdx === 0) return navEntities;
    const cat = NAV_CATEGORIES[navCategoryIdx];
    return navEntities.filter(function(e) { return e.category === cat; });
}

function scan() {
    const ab = getActorsBase();
    if (!ab) { speak('No actors', true); return; }
    const pp = getPlayerPos();
    if (!pp || isNaN(pp.x)) return;

    const nav = readNavmesh();
    const NEAR_THRESHOLD_SQ = 9;
    navEntities = [];
    let filtered = 0;
    for (let i = 1; i < NAV_MAX_ACTORS; i++) {
        try {
            const actor = ab.add(i * NAV_ACTOR_SIZE);
            const chrId = actor.readU16();
            const en = actor.add(2).readU16();
            if (chrId === 0xCDCD || en === 0xCDCD || en === 0 || chrId === 0) continue;
            // Stricter uninit-memory filter. After a load (room=-1 JUMP2),
            // actorsReady() can return true while many actor slots are still
            // partially-initialized: chr_enabled holds garbage values like
            // 2939 / 2944 / 43981 / 65535 (all > 0xFF), and var1 at +0x824
            // contains MSVC's uninit-stack pattern 0xCDCDCDCD. Every legitimate
            // actor in field/world scans has en === 1 and var1 != 0xCDCDCDCD.
            // Filter both — without this, a load surfaces 14+ ghost entities
            // stacked at world origin (see FFX_SR_Latest.log 2026-04-27).
            if (en > 0xFF) continue;
            let var1 = 0;
            try { var1 = actor.add(0x824).readU32(); } catch (e) { continue; }
            if (var1 === 0xCDCDCDCD) continue;
            const cat = classify(chrId);
            if (cat === 'Player') continue;
            const pos = { x: actor.add(0x0C).readFloat(), y: actor.add(0x10).readFloat(), z: actor.add(0x14).readFloat() };
            if (isNaN(pos.x) || isNaN(pos.z)) continue;
            const dist = distXZ(pp, pos);
            if (isNaN(dist)) continue;

            const name = readDevString(actor);

            if (nav && !isNearWalkable(nav, pos, NEAR_THRESHOLD_SQ)) {
                // Log so we can see what's being dropped — interactables that
                // sit just off the walkable mesh (flint on a pedestal, flowers
                // in an alcove, etc.) come up here. Useful for tuning the
                // threshold or identifying entities the user can't reach.
                logFile('[ACTOR-OFFMESH ' + i + '] chrId=0x' + chrId.toString(16) +
                    ' en=' + en + ' name=' + (name || 'null') +
                    ' pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')');
                filtered++;
                continue;
            }

            try {
                logFile('[ACTOR ' + i + '] chrId=0x' + chrId.toString(16) + ' en=' + en +
                    ' name=' + (name || 'null') +
                    ' pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')' +
                    ' rot=' + actor.add(0x158).readFloat().toFixed(3) +
                    ' speed=' + actor.add(0x154).readFloat().toFixed(3) +
                    ' dir=' + actor.add(0x168).readFloat().toFixed(3) +
                    ' var1=0x' + actor.add(0x824).readU32().toString(16));
            } catch (e) {}

            navEntities.push({ name: name, category: cat, chrId: chrId, pos: pos, dist: dist, actorIdx: i });
        } catch (e) {}
    }

    scanExits(pp, navEntities);

    navEntities.sort(function(a, b) { return a.dist - b.dist; });
    navSelectedIdx = 0;
    logFile('[NAV] Scan: ' + navEntities.length + ' entities (' + filtered + ' filtered by navmesh)');
}

function announceSelected(interrupt) {
    if (interrupt === undefined) interrupt = true;
    const list = getFiltered();
    if (list.length === 0) return;
    if (navSelectedIdx >= list.length) navSelectedIdx = 0;
    const e = list[navSelectedIdx];
    const pp = getPlayerPos();
    if (!pp) return;
    const dist = distXZ(pp, e.pos);
    const steps = (dist / NAV_UNITS_PER_STEP).toFixed(1);
    const dir = relativeDir(e.pos.x - pp.x, e.pos.z - pp.z);
    speak((navSelectedIdx + 1) + '/' + list.length + ': ' + label(e) + ', ' + steps + ' steps ' + dir, interrupt);
}

function pathfind() {
    const list = getFiltered();
    if (list.length === 0 || navSelectedIdx >= list.length) { speak('No entity selected', true); return; }
    const target = list[navSelectedIdx];
    const pp = getPlayerPos(); if (!pp) return;
    const nav = readNavmesh();
    if (!nav) {
        speak(label(target) + ': ' + (distXZ(pp,target.pos)/NAV_UNITS_PER_STEP).toFixed(1) + ' steps ' + relativeDir(target.pos.x-pp.x,target.pos.z-pp.z), true);
        return;
    }
    const st = findTri(nav, pp.x, pp.z), en = findTri(nav, target.pos.x, target.pos.z);
    if (st < 0 || en < 0) {
        speak(label(target) + ': crow-flies ' + (distXZ(pp,target.pos)/NAV_UNITS_PER_STEP).toFixed(1) + ' steps', true);
        return;
    }
    const path = astar(nav, st, en);
    if (!path) { speak(label(target) + ': no path', true); return; }
    const rawWps = [{x:pp.x,z:pp.z}];
    for (let i = 0; i < path.length - 1; i++) {
        const a = nav.tris[path[i]], b = nav.tris[path[i+1]];
        const sh = [];
        for (let vi = 0; vi < 3; vi++) for (let vj = 0; vj < 3; vj++)
            if (a.verts[vi] === b.verts[vj]) sh.push(vi);
        if (sh.length === 2) rawWps.push({x:(a.vxs[sh[0]]+a.vxs[sh[1]])/2, z:(a.vzs[sh[0]]+a.vzs[sh[1]])/2});
    }
    rawWps.push({x:target.pos.x, z:target.pos.z});

    const wpsBefore = rawWps.length;
    const wps = smoothWaypoints(nav, rawWps);
    logFile('[NAV] Path: tris=' + path.length + ' wps ' + wpsBefore + '->' + wps.length +
        ' crow=' + relativeDir(target.pos.x - pp.x, target.pos.z - pp.z));
    const dirs = [];
    for (let i = 0; i < wps.length - 1; i++) {
        const dx = wps[i+1].x - wps[i].x, dz = wps[i+1].z - wps[i].z;
        const steps = Math.sqrt(dx*dx + dz*dz) / NAV_UNITS_PER_STEP;
        const dir = relativeDir(dx, dz);
        logFile('[NAV]   seg ' + i + ': dx=' + dx.toFixed(1) + ' dz=' + dz.toFixed(1) +
            ' steps=' + steps.toFixed(1) + ' dir=' + dir);
        if (steps > 0.5) dirs.push(Math.round(steps) + ' ' + dir);
    }
    const merged = [];
    for (let i = 0; i < dirs.length; i++) {
        const p = dirs[i].split(' ');
        if (merged.length > 0) {
            const prev = merged[merged.length-1].split(' ');
            if (prev.length >= 2 && p.length >= 2 && prev[prev.length-1] === p[p.length-1]) {
                merged[merged.length-1] = (parseInt(prev[0]) + parseInt(p[0])) + ' ' + p[p.length-1];
                continue;
            }
        }
        merged.push(dirs[i]);
    }
    speak(merged.length > 0 ? merged.join(', ') : 'you are here', true);
}

function hotkeyScan() {
    scan();
    const list = getFiltered();
    speak(list.length + ' ' + NAV_CATEGORIES[navCategoryIdx] + (navCategoryIdx === 0 ? '' : 's'), true);
    if (list.length > 0) announceSelected();
}

function hotkeyCycleNext() {
    if (keyDown(VK_SHIFT)) {
        navCategoryIdx = (navCategoryIdx + 1) % NAV_CATEGORIES.length;
        navSelectedIdx = 0;
        const list = getFiltered();
        speak('category: ' + NAV_CATEGORIES[navCategoryIdx] + ', ' + list.length, true);
        if (list.length > 0) announceSelected(false);
    } else {
        const list = getFiltered();
        if (list.length > 0) { navSelectedIdx = (navSelectedIdx + 1) % list.length; announceSelected(); }
    }
}

function hotkeyCyclePrev() {
    if (keyDown(VK_SHIFT)) {
        navCategoryIdx = (navCategoryIdx - 1 + NAV_CATEGORIES.length) % NAV_CATEGORIES.length;
        navSelectedIdx = 0;
        const list = getFiltered();
        speak('category: ' + NAV_CATEGORIES[navCategoryIdx] + ', ' + list.length, true);
        if (list.length > 0) announceSelected(false);
    } else {
        const list = getFiltered();
        if (list.length > 0) { navSelectedIdx = (navSelectedIdx - 1 + list.length) % list.length; announceSelected(); }
    }
}

function hotkeyAnnounceDistance() {
    const list = getFiltered();
    if (list.length > 0 && navSelectedIdx < list.length) {
        const e = list[navSelectedIdx];
        const pp = getPlayerPos();
        if (pp) {
            const s = (distXZ(pp,e.pos)/NAV_UNITS_PER_STEP).toFixed(1);
            speak(label(e) + ': ' + s + ' steps ' + relativeDir(e.pos.x-pp.x,e.pos.z-pp.z), true);
        }
    }
}

// Continuous event-driven rescan loop. Each frame: count fully-written
// actors and compare against the last observation. A delta means the
// engine just added (or removed) an actor — rescan. The trigger is the
// engine's own write activity, not a timing threshold. Steady state
// (count unchanged) is free: the array walk costs microseconds, and no
// scan() is invoked.
//
// We never stop watching. Late actor changes (scripted spawns after a
// cutscene, NPCs walking in/out, etc.) keep the entity list current
// without requiring manual rescan. The only gates are battle state
// (skip — battle uses a separate entity system) and player-slot
// readiness (skip during the few frames between JUMP2 and the engine
// restoring player position).
function onTick() {
    try {
        if (btlStateAddr.readU8() !== 0) return;
        if (!playerSlotReady()) return;
        const cur = countLoadedActors();
        if (cur < 0) return;
        if (cur !== navLastActorCount) {
            logFile('[NAV] Actor count: ' + navLastActorCount + ' -> ' + cur + ' — rescanning');
            navLastActorCount = cur;
            if (cur > 0) scan();
        }
    } catch (e) {}
}

export function install() {
    // AtelSetEventJump2 (RVA 0x46FED0) — fires at the START of a map
    // transition. Clears the entity cache and arms the scan-pending flag
    // so onTick begins polling for actor readiness.
    //
    // Do NOT add Thread.backtrace here. Accurate backtracer walks stack
    // frames via debug info and can AV during map transitions when frames
    // are partially unwound. retRVA + worker info is enough diagnostic.
    attachRva(0x46FED0, {
        onEnter: function(args) {
            const room = args[0].toInt32();
            const entrance = args[1].toInt32();
            const fade = args[2].toInt32();
            const retAddr = this.returnAddress;
            const retRva = (isValidPtr(retAddr) && retAddr.compare(base) > 0)
                ? retAddr.sub(base).toInt32() : -1;
            const pp = getPlayerPos();
            const posStr = pp ? ('(' + pp.x.toFixed(1) + ',' + pp.y.toFixed(1) + ',' + pp.z.toFixed(1) + ')') : 'none';
            let workerInfo = '';
            try {
                const worker = rva(RVA_CURRENT_WORKER).readPointer();
                if (isValidPtr(worker)) {
                    workerInfo = ' w_idx=' + worker.add(0x2E).readU16() + ' w_entry=' + worker.add(0x2C).readU16();
                }
            } catch (e) {}
            logFile('[JUMP2] room=0x' + room.toString(16) + ' entrance=' + entrance +
                ' fade=' + fade + ' playerPos=' + posStr +
                ' retRVA=0x' + retRva.toString(16) + workerInfo);
            navEntities = [];
            navSelectedIdx = 0;
            // Reset count tracker so the first onTick observation after the
            // new map starts loading is treated as a delta and triggers a
            // scan. Without this, a stale count from the previous map could
            // match the new table's first observation and skip the scan.
            navLastActorCount = -1;
        }
    }, 'AtelSetEventJump2');

    // Map-load complete (FUN_009093B0, RVA 0x5093B0) — diagnostic only.
    // onTick handles all scanning via count-change detection regardless
    // of whether this fires before or after actor data lands.
    attachRva(0x5093B0, {
        onLeave: function() {
            try {
                logFile('[NAV] Map load complete' +
                    (btlStateAddr.readU8() !== 0 ? ' (battle state)' : ''));
            } catch (e) {}
        }
    }, 'Map load complete');

    registerHotkey(VK_OEM_3, hotkeyScan, 'nav.scan');
    registerHotkey(VK_OEM_6, hotkeyCycleNext, 'nav.next');
    registerHotkey(VK_OEM_4, hotkeyCyclePrev, 'nav.prev');
    registerHotkey(VK_P,     hotkeyAnnounceDistance, 'nav.announce');
    registerHotkey(VK_OEM_5, pathfind, 'nav.pathfind');

    onFrame(onTick);
    logFile('Navigation feature installed');
}
