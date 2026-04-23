// A* pathfinding on the navmesh + greedy waypoint smoothing.

import { buildWalkable, hasLineOfSight } from './navmesh.js';

export function astar(nav, start, end) {
    if (start === end) return [start];
    if (start < 0 || end < 0) return null;
    const ex = nav.tris[end].cx, ez = nav.tris[end].cz;
    const open = {}, closed = {}, gS = {}, fS = {}, from = {};
    gS[start] = 0;
    fS[start] = Math.sqrt((nav.tris[start].cx-ex)*(nav.tris[start].cx-ex)+(nav.tris[start].cz-ez)*(nav.tris[start].cz-ez));
    open[start] = true;
    for (let iter = 0; iter < 10000; iter++) {
        let cur = -1, bestF = 1e18;
        for (const k in open) {
            const ki = parseInt(k);
            if (fS[ki] < bestF) { bestF = fS[ki]; cur = ki; }
        }
        if (cur === -1) return null;
        if (cur === end) {
            const path = [cur];
            while (from[cur] !== undefined) { cur = from[cur]; path.unshift(cur); }
            return path;
        }
        delete open[cur]; closed[cur] = true;
        const tri = nav.tris[cur];
        for (let ni = 0; ni < 3; ni++) {
            const nb = tri.neighbours[ni];
            if (nb < 0 || nb >= nav.triCount || closed[nb]) continue;
            if (nav.tris[nb].coll !== 0 && nav.tris[nb].coll !== 2) continue;
            const tg = gS[cur] + Math.sqrt(
                (tri.cx-nav.tris[nb].cx)*(tri.cx-nav.tris[nb].cx) +
                (tri.cz-nav.tris[nb].cz)*(tri.cz-nav.tris[nb].cz));
            if (gS[nb] === undefined || tg < gS[nb]) {
                from[nb] = cur; gS[nb] = tg;
                fS[nb] = tg + Math.sqrt(
                    (nav.tris[nb].cx-ex)*(nav.tris[nb].cx-ex) +
                    (nav.tris[nb].cz-ez)*(nav.tris[nb].cz-ez));
                open[nb] = true;
            }
        }
    }
    return null;
}

// Greedy: collapse consecutive waypoints by jumping to the furthest one
// reachable via straight line of sight on walkable tris.
export function smoothWaypoints(nav, wps) {
    if (wps.length <= 2) return wps;
    const walkable = buildWalkable(nav);
    const out = [wps[0]];
    let i = 0;
    while (i < wps.length - 1) {
        let next = i + 1;
        for (let j = wps.length - 1; j > i + 1; j--) {
            if (hasLineOfSight(walkable, wps[i].x, wps[i].z, wps[j].x, wps[j].z)) {
                next = j;
                break;
            }
        }
        out.push(wps[next]);
        i = next;
    }
    return out;
}
