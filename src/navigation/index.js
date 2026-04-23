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
let navScanPending = false;
let navScanRetries = 0;

// 0xcdcdcdcd reinterpreted as float; indicates an uninit actor slot.
const ACTOR_UNINIT_FLOAT = -431602080;

// Is the actor array populated enough to scan? Player slot (actor 0) at
// +0x0C holds x; non-zero and non-garbage means the game finished loading
// per-actor runtime state. px=0 alone isn't a safe gate — some maps place
// the player legitimately at origin — but px=garbage is an uninit slot.
function actorsReady() {
    const ab = getActorsBase();
    if (!ab) return false;
    try {
        const px = ab.add(0x0C).readFloat();
        if (isNaN(px)) return false;
        if (px === ACTOR_UNINIT_FLOAT) return false;
        if (px === 0) return false;
        return true;
    } catch (e) { return false; }
}

function classify(id) {
    if (id === 0x5001) return 'Save Sphere';
    if (id === 0x5002) return 'Treasure Chest';
    if (id >= 0x2000 && id < 0x5000) return 'NPC';
    if (id <= 0x0012) return 'Player';
    return 'Object';
}

function label(e) {
    return (e.name !== '?' && e.name !== e.category) ? e.category + ' "' + e.name + '"' : e.category;
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
            const cat = classify(chrId);
            if (cat === 'Player') continue;
            const pos = { x: actor.add(0x0C).readFloat(), y: actor.add(0x10).readFloat(), z: actor.add(0x14).readFloat() };
            if (isNaN(pos.x) || isNaN(pos.z)) continue;
            const dist = distXZ(pp, pos);
            if (isNaN(dist)) continue;

            if (nav && !isNearWalkable(nav, pos, NEAR_THRESHOLD_SQ)) { filtered++; continue; }

            let name = '?';
            try {
                const np = actor.add(0x04).readPointer();
                const s = safeReadCString(np, 64);
                if (s) name = s;
            } catch (e) {}

            try {
                logFile('[ACTOR ' + i + '] chrId=0x' + chrId.toString(16) + ' en=' + en +
                    ' name=' + name +
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

// Narrow gated polling: active ONLY between JUMP2 and the first successful
// scan after a transition. Empirically FUN_009093B0 returns before actor
// positions and trigger worker classification stabilize (latency 40ms–1s+
// past return), so we need to wait for actors to be ready before scanning.
// The map-load-complete hook below tries a fast-path first; if actors
// aren't ready yet, this tick retries for up to 150 frames.
function onTick() {
    if (!navScanPending) return;
    try {
        navScanRetries++;
        if (actorsReady()) {
            logFile('[NAV] Actors ready after ' + navScanRetries + ' frames');
            scan();
            navScanPending = false;
            return;
        }
        if (navScanRetries > 150) {
            logFile('[NAV] Scan timed out — no actors after 150 frames');
            navScanPending = false;
        }
    } catch (e) { navScanPending = false; }
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
            if (btlStateAddr.readU8() === 0) {
                navScanPending = true;
                navScanRetries = 0;
            } else {
                logFile('[NAV] Skipping autoscan arm — battle state active');
            }
        }
    }, 'AtelSetEventJump2');

    // Map-load complete (FUN_009093B0, RVA 0x5093B0) — orchestrates the
    // full map-load sequence: state reset, layer loads, NavMesh install,
    // finalize. On its simplest transitions this returns AFTER actor state
    // is populated; on others actors stabilize up to a second later.
    // Use it as a fast path: if actorsReady() is true on return, scan
    // immediately; otherwise let onTick poll until actors are ready.
    attachRva(0x5093B0, {
        onLeave: function() {
            try {
                if (btlStateAddr.readU8() !== 0) {
                    logFile('[NAV] Skipping autoscan — battle state active');
                    navScanPending = false;
                    return;
                }
                if (actorsReady()) {
                    logFile('[NAV] Map load complete — fast-path scan');
                    scan();
                    navScanPending = false;
                } else {
                    logFile('[NAV] Map load complete — deferring to tick retry');
                }
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
