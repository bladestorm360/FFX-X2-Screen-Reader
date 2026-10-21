#pragma once

#include "navigation.hpp"
#include <algorithm>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <utility>

namespace ffx {

// Fixed-size facts safe to publish from the native capture path. Conversion
// back to Entity belongs to the worker, where strings and vectors may allocate.
struct OwnedNavTarget {
    NativeRouteTarget native;
    std::array<wchar_t,129> category;
    double distance;
    EntryMask usefulEntries;
    unsigned butterflyEpoch;
    bool unreachable,authoredTrialControl,butterflyStart,hazardous;
    bool swimApproach,swimReady;
    double swimClearance;
};

static_assert(std::is_trivially_copyable_v<OwnedNavTarget>);
static_assert(std::is_trivially_destructible_v<OwnedNavTarget>);

inline void copyInteractionShape(const InteractionShape& from, InteractionShape& to) noexcept {
    to.center=from.center; to.end=from.end; to.angle=from.angle;
    to.width=from.width; to.depth=from.depth; to.range=from.range;
    to.height=from.height; to.viewAngle=from.viewAngle; to.shape=from.shape;
    to.worker=from.worker; to.valid=from.valid; to.crossing=from.crossing;
    to.talk=from.talk;
}

inline bool captureOwnedNavTarget(const Entity& source, OwnedNavTarget& destination) noexcept {
    if(source.name.size()>2048 || source.category.size()>128 ||
       source.zone.competing.size()>512) return false;

    auto& captured=destination;
    if(!source.name.empty())
        std::memcpy(captured.native.name.data(),source.name.data(),source.name.size()*sizeof(wchar_t));
    captured.native.name[source.name.size()]=L'\0';
    if(!source.category.empty())
        std::memcpy(captured.category.data(),source.category.data(),source.category.size()*sizeof(wchar_t));
    captured.category[source.category.size()]=L'\0';

    auto& native=captured.native;
    native.id=source.id; native.actor=source.actor; native.worker=source.worker;
    native.position=source.position; native.pushControl=source.pushControl;
    native.pushLocal=source.pushLocal; native.inactive=source.inactive;
    native.butterfly=source.butterfly; native.swimDepth=source.swimDepth;
    native.swimChunk=source.swimChunk;
    copyInteractionShape(static_cast<const InteractionShape&>(source.zone),
                         static_cast<InteractionShape&>(native.zone));
    native.zone.competing.count=static_cast<unsigned>(source.zone.competing.size());
    for(unsigned i=0;i<native.zone.competing.count;++i)
        copyInteractionShape(source.zone.competing[i],native.zone.competing.rows[i]);
    native.zone.contactRadius=source.zone.contactRadius;
    native.zone.contactMinY=source.zone.contactMinY;
    native.zone.contactMaxY=source.zone.contactMaxY;
    native.zone.approach=source.zone.approach; native.zone.obstacle=source.zone.obstacle;
    native.zone.obstacleRadius=source.zone.obstacleRadius;

    captured.distance=source.distance; captured.usefulEntries=source.usefulEntries;
    captured.butterflyEpoch=source.butterflyEpoch;
    captured.unreachable=source.unreachable;
    captured.authoredTrialControl=source.authoredTrialControl;
    captured.butterflyStart=source.butterflyStart; captured.hazardous=source.hazardous;
    captured.swimApproach=source.swimApproach; captured.swimReady=source.swimReady;
    captured.swimClearance=source.swimClearance;
    return true;
}

inline bool restoreOwnedNavTarget(const OwnedNavTarget& source, Entity& destination) {
    const auto nameEnd=std::find(source.native.name.begin(),source.native.name.end(),L'\0');
    const auto categoryEnd=std::find(source.category.begin(),source.category.end(),L'\0');
    if(nameEnd==source.native.name.end() || categoryEnd==source.category.end() ||
       source.native.zone.competing.count>source.native.zone.competing.rows.size()) return false;

    Entity restored;
    restored.name.assign(source.native.name.begin(),nameEnd);
    restored.category.assign(source.category.begin(),categoryEnd);
    restored.id=source.native.id; restored.actor=source.native.actor;
    restored.worker=source.native.worker; restored.position=source.native.position;
    restored.distance=source.distance;
    copyInteractionShape(static_cast<const InteractionShape&>(source.native.zone),
                         static_cast<InteractionShape&>(restored.zone));
    restored.zone.competing.reserve(source.native.zone.competing.count);
    for(unsigned i=0;i<source.native.zone.competing.count;++i)
        restored.zone.competing.push_back(source.native.zone.competing.rows[i]);
    restored.zone.contactRadius=source.native.zone.contactRadius;
    restored.zone.contactMinY=source.native.zone.contactMinY;
    restored.zone.contactMaxY=source.native.zone.contactMaxY;
    restored.zone.approach=source.native.zone.approach;
    restored.zone.obstacle=source.native.zone.obstacle;
    restored.zone.obstacleRadius=source.native.zone.obstacleRadius;
    restored.unreachable=source.unreachable; restored.inactive=source.native.inactive;
    restored.pushControl=source.native.pushControl; restored.pushLocal=source.native.pushLocal;
    restored.usefulEntries=source.usefulEntries;
    restored.authoredTrialControl=source.authoredTrialControl;
    restored.butterfly=source.native.butterfly;
    restored.hazardous=source.hazardous; restored.butterflyStart=source.butterflyStart;
    restored.butterflyEpoch=source.butterflyEpoch;
    restored.swimApproach=source.swimApproach; restored.swimDepth=source.native.swimDepth;
    restored.swimReady=source.swimReady; restored.swimClearance=source.swimClearance;
    restored.swimChunk=source.native.swimChunk;
    destination=std::move(restored);
    return true;
}

inline Entity restoreOwnedNavTarget(const OwnedNavTarget& source) {
    Entity restored;
    if(!restoreOwnedNavTarget(source,restored)) throw std::invalid_argument("invalid owned navigation target");
    return restored;
}

} // namespace ffx
