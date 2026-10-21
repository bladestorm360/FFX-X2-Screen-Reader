#pragma once

#include "navigation.hpp"
#include "navigation_cancel.hpp"
#include "collision_rules.hpp"
#include "cross_water_data.hpp"
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace ffx {

struct RawNavigationTriangle {
    std::int16_t vertices[3];
    std::int16_t neighbours[3];
    std::uint32_t collision;
};
struct RawNavigationVertex {
    std::int16_t coordinates[4];
};
static_assert(sizeof(RawNavigationTriangle) == 16, "native triangle layout");
static_assert(sizeof(RawNavigationVertex) == 8, "native vertex layout");
static_assert(std::is_trivial<RawNavigationTriangle>::value, "owned POD triangle");
static_assert(std::is_trivial<RawNavigationVertex>::value, "owned POD vertex");

inline Mesh decodeOwnedNavigationMesh(
    const std::vector<RawNavigationTriangle>& rawTriangles,
    const std::vector<RawNavigationVertex>& rawVertices, float scale,
    const nav::CollisionSnapshot& collision, bool swimming,
    int playerTriangle, Point player,unsigned waterBake=0,unsigned waterCrossings=0,const RouteEvents& events={}) {
    Mesh mesh;mesh.events=events;
    // A missing/partial Sin floor must stay unresolved, including the generic
    // empty swimming-plane fallback below. Validate shape before early returns.
    if(events.kind==1 && (rawTriangles.size()!=1220 || rawVertices.size()!=1179 ||
        !std::isfinite(scale) || std::abs(scale-27.899267578125)>.00001)) mesh.events.ready=false;
    if (rawTriangles.size() > 100000 || rawVertices.size() > 32768) return mesh;
    if (rawTriangles.empty()) {
        if (swimming && std::isfinite(player.y)) {
            mesh.openPlane = true; mesh.fixedHeight = true; mesh.planeY = player.y;
        }
        return mesh;
    }
    if (!std::isfinite(scale) || scale <= 0) return mesh;
    if(events.kind==1) {
        // The script event overlay is valid only on the original physical
        // floor. Hashing runs on this owned decoder, never native processing.
        const auto hash=[](const void* p,size_t n) {
            unsigned h=2166136261u;const auto* bytes=static_cast<const uint8_t*>(p);
            for(size_t i=0;i<n;++i) {if((i&255)==0) nav::checkComputation();h=(h^bytes[i])*16777619u;}return h;
        };
        if(rawTriangles.size()!=1220 || rawVertices.size()!=1179 || std::abs(scale-27.899267578125)>.00001 ||
            hash(rawTriangles.data(),rawTriangles.size()*sizeof(RawNavigationTriangle))!=0x4024376f ||
            hash(rawVertices.data(),rawVertices.size()*sizeof(RawNavigationVertex))!=0xa8173459) mesh.events.ready=false;
    }
    mesh.triangles.resize(rawTriangles.size());
    for (std::size_t i = 0; i < rawTriangles.size(); ++i) {
        nav::checkComputation();
        const auto& src = rawTriangles[i];
        auto& dst = mesh.triangles[i];
        dst.collisionGroup = src.collision & 127u;
        dst.collision = nav::blockedForGroup(src.collision, collision) ? 1u : 0u;
        bool valid = true;
        for (unsigned j = 0; j < 3; ++j) {
            dst.vertices[j] = src.vertices[j];
            dst.neighbours[j] = src.neighbours[j];
            const int index = src.vertices[j];
            if (index < 0 || static_cast<std::size_t>(index) >= rawVertices.size()) {
                valid = false; break;
            }
            const auto& c = rawVertices[static_cast<std::size_t>(index)].coordinates;
            dst.points[j] = {c[0] / static_cast<double>(scale),
                             c[1] / static_cast<double>(scale),
                             c[2] / static_cast<double>(scale)};
            dst.center.x += dst.points[j].x / 3;
            dst.center.y += dst.points[j].y / 3;
            dst.center.z += dst.points[j].z / 3;
        }
        dst.valid = valid;
    }
    if(waterBake==1 && rawTriangles.size()==1406 && rawVertices.size()==1039 && std::abs(scale-18.835050964355467)<.00001) {
        const auto hash=[](const void* source,size_t bytes) {
            unsigned value=2166136261u;const auto* p=static_cast<const uint8_t*>(source);
            for(size_t i=0;i<bytes;++i) {if((i&255)==0) nav::checkComputation();value=(value^p[i])*16777619u;}return value;
        };
        if(hash(rawTriangles.data(),rawTriangles.size()*sizeof(RawNavigationTriangle))==nav::gagazetCrossWater.triangleHash &&
            hash(rawVertices.data(),rawVertices.size()*sizeof(RawNavigationVertex))==nav::gagazetCrossWater.vertexHash) {
            mesh.crossWater=&nav::gagazetCrossWater;
            mesh.waterCrossings=waterCrossings&7;
            if(swimming) mesh.terrain=mesh.triangles; // Original heights and current native gates, owned by this worker.
        }
    }
    if (swimming) {
        mesh.swimmingFloor = true;
        mesh.floorY.resize(mesh.triangles.size());
        for (std::size_t i = 0; i < mesh.triangles.size(); ++i)
            for (unsigned j = 0; j < 3; ++j)
                mesh.floorY[i][j] = mesh.triangles[i].points[j].y;
        if (playerTriangle < 0 || static_cast<std::size_t>(playerTriangle) >= mesh.triangles.size() ||
            !mesh.triangles[static_cast<std::size_t>(playerTriangle)].valid || !std::isfinite(player.y))
            return Mesh{};
        std::vector<bool> reached(mesh.triangles.size(), false);
        std::vector<int> pending{playerTriangle};
        reached[static_cast<std::size_t>(playerTriangle)] = true;
        while (!pending.empty()) {
            nav::checkComputation();
            const int at = pending.back(); pending.pop_back();
            for (int next : mesh.triangles[static_cast<std::size_t>(at)].neighbours) {
                if (next >= 0 && static_cast<std::size_t>(next) < mesh.triangles.size() &&
                    !reached[static_cast<std::size_t>(next)] && mesh.triangles[static_cast<std::size_t>(next)].valid &&
                    mesh.triangles[static_cast<std::size_t>(next)].collision == 0) {
                    reached[static_cast<std::size_t>(next)] = true; pending.push_back(next);
                }
            }
        }
        mesh.fixedHeight = true; mesh.planeY = player.y;
        for (std::size_t i = 0; i < mesh.triangles.size(); ++i) {
            nav::checkComputation();
            auto& triangle = mesh.triangles[i];
            triangle.valid = triangle.valid && reached[i];
            for (auto& point : triangle.points) point.y = player.y;
            triangle.center.y = player.y;
        }
    }
    mesh.playerTriangle = playerTriangle;
    mesh.player = player;
    return mesh;
}

} // namespace ffx
