// Navmesh reading + point-in-triangle queries. Pure geometry — no hooks.

import { rva } from '../core/memory.js';
import {
    RVA_TRIS, RVA_TRICOUNT, RVA_VERTICES, RVA_SCALE,
    RVA_MS_CAMERA_NOW, CAM_EYE_OFFSET, CAM_TARGET_OFFSET,
    RVA_ACTOR_ARRAY
} from '../core/addresses.js';
import { isValidPtr } from '../core/safeRead.js';

// Uninitialized / freed-heap sentinels (MSVC debug fill + common allocator
// patterns). Values above these are garbage regardless of < 0x10000 check.
function looksLikeSentinel(u32) {
    if (u32 >= 0xFEEEFEE0) return true;  // freed heap
    if (u32 >= 0xDDDDDDD0) return true;  // delete'd
    if (u32 >= 0xCDCDCDC0) return true;  // uninit heap
    if (u32 >= 0xCCCCCCC0) return true;  // uninit stack
    if (u32 >= 0xFFFFFFF0) return true;
    return false;
}

function pageCommitted(p) {
    try {
        const range = Process.findRangeByAddress(p);
        return !!range;
    } catch (e) { return false; }
}

export function readNavmesh() {
    const triCount = rva(RVA_TRICOUNT).readS32();
    const trisRaw = rva(RVA_TRIS).readU32();
    const vertsRaw = rva(RVA_VERTICES).readU32();
    const scale = rva(RVA_SCALE).readFloat();
    if (triCount <= 0 || triCount > 100000) return null;
    if (trisRaw < 0x10000 || vertsRaw < 0x10000 || scale <= 0) return null;
    if (looksLikeSentinel(trisRaw) || looksLikeSentinel(vertsRaw)) return null;
    const tB = ptr(trisRaw), vB = ptr(vertsRaw);
    if (!pageCommitted(tB) || !pageCommitted(vB)) return null;
    const tris = [];
    for (let i = 0; i < triCount; i++) {
        try {
            const t = tB.add(i * 0x10);
            const vis = [t.readS16(), t.add(2).readS16(), t.add(4).readS16()];
            const nbs = [t.add(6).readS16(), t.add(8).readS16(), t.add(10).readS16()];
            const coll = (t.add(12).readS32() >>> 0) & 0x7F;
            const vxs = [], vzs = [];
            let cx = 0, cz = 0;
            let badVertex = false;
            for (let j = 0; j < 3; j++) {
                // Vertex indices are S16; treat negative or wildly out-of-range
                // values as bad data and skip the triangle rather than reading
                // into arbitrary memory.
                if (vis[j] < 0 || vis[j] > 0x10000) { badVertex = true; break; }
                const va = vB.add(vis[j] * 8);
                const wx = va.readS16() / scale, wz = va.add(4).readS16() / scale;
                vxs.push(wx); vzs.push(wz); cx += wx; cz += wz;
            }
            if (badVertex) continue;
            tris.push({ verts: vis, neighbours: nbs, coll: coll, cx: cx/3, cz: cz/3, vxs: vxs, vzs: vzs });
        } catch (e) {
            // Single-tri read failure should skip, not abort the whole navmesh read.
        }
    }
    return { tris: tris, triCount: tris.length };
}

export function findTri(nav, px, pz) {
    for (let i = 0; i < nav.triCount; i++) {
        const t = nav.tris[i];
        const v0x=t.vxs[2]-t.vxs[0],v0z=t.vzs[2]-t.vzs[0];
        const v1x=t.vxs[1]-t.vxs[0],v1z=t.vzs[1]-t.vzs[0];
        const v2x=px-t.vxs[0],v2z=pz-t.vzs[0];
        const d00=v0x*v0x+v0z*v0z,d01=v0x*v1x+v0z*v1z,d02=v0x*v2x+v0z*v2z;
        const d11=v1x*v1x+v1z*v1z,d12=v1x*v2x+v1z*v2z;
        let inv=d00*d11-d01*d01; if (Math.abs(inv)<0.0001) continue; inv=1/inv;
        const u=(d11*d02-d01*d12)*inv, v=(d00*d12-d01*d02)*inv;
        if (u>=-0.05&&v>=-0.05&&(u+v)<=1.05) return i;
    }
    let best=999999, bi=-1;
    for (let i=0;i<nav.triCount;i++) {
        const t=nav.tris[i];
        const d=(px-t.cx)*(px-t.cx)+(pz-t.cz)*(pz-t.cz);
        if (d<best) { best=d; bi=i; }
    }
    return bi;
}

export function buildWalkable(nav) {
    const walkable = [];
    for (let i = 0; i < nav.triCount; i++) {
        const t = nav.tris[i];
        if (t.coll !== 0 && t.coll !== 2) continue;
        walkable.push({
            tri: t,
            minX: Math.min(t.vxs[0], t.vxs[1], t.vxs[2]),
            maxX: Math.max(t.vxs[0], t.vxs[1], t.vxs[2]),
            minZ: Math.min(t.vzs[0], t.vzs[1], t.vzs[2]),
            maxZ: Math.max(t.vzs[0], t.vzs[1], t.vzs[2])
        });
    }
    return walkable;
}

function findTriStrictFast(walkable, px, pz) {
    for (let wi = 0; wi < walkable.length; wi++) {
        const w = walkable[wi];
        if (px < w.minX || px > w.maxX || pz < w.minZ || pz > w.maxZ) continue;
        const t = w.tri;
        const v0x=t.vxs[2]-t.vxs[0],v0z=t.vzs[2]-t.vzs[0];
        const v1x=t.vxs[1]-t.vxs[0],v1z=t.vzs[1]-t.vzs[0];
        const v2x=px-t.vxs[0],v2z=pz-t.vzs[0];
        const d00=v0x*v0x+v0z*v0z,d01=v0x*v1x+v0z*v1z,d02=v0x*v2x+v0z*v2z;
        const d11=v1x*v1x+v1z*v1z,d12=v1x*v2x+v1z*v2z;
        let inv=d00*d11-d01*d01; if (Math.abs(inv)<0.0001) continue; inv=1/inv;
        const u=(d11*d02-d01*d12)*inv, v=(d00*d12-d01*d02)*inv;
        if (u>=-0.02&&v>=-0.02&&(u+v)<=1.02) return true;
    }
    return false;
}

export function hasLineOfSight(walkable, x1, z1, x2, z2) {
    const dist = Math.sqrt((x2-x1)*(x2-x1) + (z2-z1)*(z2-z1));
    const steps = Math.max(3, Math.ceil(dist / 5));
    for (let i = 0; i <= steps; i++) {
        const t = i / steps;
        if (!findTriStrictFast(walkable, x1 + (x2-x1) * t, z1 + (z2-z1) * t)) return false;
    }
    return true;
}

// Entity navmesh proximity: does the given point sit on or within NEAR units of
// any walkable triangle? Catches NPCs standing on floor AND props just off it.
export function isNearWalkable(nav, pos, nearThresholdSq) {
    for (let ti = 0; ti < nav.triCount; ti++) {
        const t = nav.tris[ti];
        if (t.coll !== 0 && t.coll !== 2) continue;
        const minX = Math.min(t.vxs[0], t.vxs[1], t.vxs[2]) - 3;
        const maxX = Math.max(t.vxs[0], t.vxs[1], t.vxs[2]) + 3;
        const minZ = Math.min(t.vzs[0], t.vzs[1], t.vzs[2]) - 3;
        const maxZ = Math.max(t.vzs[0], t.vzs[1], t.vzs[2]) + 3;
        if (pos.x < minX || pos.x > maxX || pos.z < minZ || pos.z > maxZ) continue;
        // Strict point-in-triangle
        const v0x=t.vxs[2]-t.vxs[0], v0z=t.vzs[2]-t.vzs[0];
        const v1x=t.vxs[1]-t.vxs[0], v1z=t.vzs[1]-t.vzs[0];
        const v2x=pos.x-t.vxs[0],  v2z=pos.z-t.vzs[0];
        const d00=v0x*v0x+v0z*v0z, d01=v0x*v1x+v0z*v1z;
        const d02=v0x*v2x+v0z*v2z, d11=v1x*v1x+v1z*v1z;
        const d12=v1x*v2x+v1z*v2z;
        const inv = d00*d11 - d01*d01;
        if (Math.abs(inv) >= 0.0001) {
            const invN = 1/inv;
            const u = (d11*d02 - d01*d12) * invN;
            const v = (d00*d12 - d01*d02) * invN;
            if (u >= -0.02 && v >= -0.02 && (u + v) <= 1.02) return true;
        }
        // Edge distance check
        for (let ei = 0; ei < 3; ei++) {
            const ax = t.vxs[ei], az = t.vzs[ei];
            const bx = t.vxs[(ei+1)%3], bz = t.vzs[(ei+1)%3];
            const ex = bx - ax, ez = bz - az;
            const el = ex*ex + ez*ez;
            if (el < 0.0001) continue;
            let tt = ((pos.x-ax)*ex + (pos.z-az)*ez) / el;
            if (tt < 0) tt = 0; else if (tt > 1) tt = 1;
            const cx = ax + ex*tt, cz = az + ez*tt;
            const ddx = pos.x-cx, ddz = pos.z-cz;
            if (ddx*ddx + ddz*ddz <= nearThresholdSq) return true;
        }
    }
    return false;
}

export function getActorsBase() {
    const p = rva(RVA_ACTOR_ARRAY).readPointer();
    return p.isNull() ? null : p;
}

export function getPlayerPos() {
    const ab = getActorsBase();
    if (!ab) return null;
    return { x: ab.add(0x0C).readFloat(), y: ab.add(0x10).readFloat(), z: ab.add(0x14).readFloat() };
}

export function distXZ(a, b) {
    const dx = a.x - b.x, dz = a.z - b.z;
    return Math.sqrt(dx * dx + dz * dz);
}

// Camera-relative "screen-up is north" angle. See original decompile notes in
// layer2_game_logic.txt ~line 347242. Both axes are intentionally flipped —
// the forward vector is (eye - target), not (target - eye).
export function getScreenNorthAngle() {
    try {
        const camPtr = rva(RVA_MS_CAMERA_NOW).readPointer();
        if (camPtr.isNull() || camPtr.compare(ptr(0x10000)) <= 0) return null;
        const ex = camPtr.add(CAM_EYE_OFFSET).readFloat();
        const ez = camPtr.add(CAM_EYE_OFFSET + 8).readFloat();
        const tx = camPtr.add(CAM_TARGET_OFFSET).readFloat();
        const tz = camPtr.add(CAM_TARGET_OFFSET + 8).readFloat();
        if (isNaN(ex) || isNaN(ez) || isNaN(tx) || isNaN(tz)) return null;
        const fx = ex - tx, fz = ez - tz;
        if (Math.abs(fx) < 0.001 && Math.abs(fz) < 0.001) return null;
        return Math.atan2(fz, fx);
    } catch (e) { return null; }
}

export function relativeDir(dx, dz) {
    if (Math.abs(dx) < 0.01 && Math.abs(dz) < 0.01) return 'here';
    const northAngle = getScreenNorthAngle();
    if (northAngle === null) return 'unknown';
    const worldAngle = Math.atan2(dz, dx);
    let rel = worldAngle - northAngle;
    while (rel > Math.PI) rel -= 2 * Math.PI;
    while (rel < -Math.PI) rel += 2 * Math.PI;
    const deg = rel * 180 / Math.PI;
    if (deg >= -22.5 && deg < 22.5) return 'north';
    if (deg >= 22.5 && deg < 67.5) return 'northwest';
    if (deg >= 67.5 && deg < 112.5) return 'west';
    if (deg >= 112.5 && deg < 157.5) return 'southwest';
    if (deg >= -67.5 && deg < -22.5) return 'northeast';
    if (deg >= -112.5 && deg < -67.5) return 'east';
    if (deg >= -157.5 && deg < -112.5) return 'southeast';
    return 'south';
}
