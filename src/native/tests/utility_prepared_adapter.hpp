#pragma once

// Fixture-only prepared-resource service. Snapshots are built explicitly by
// setup code; the APIs below only query owned fixture data.
#include "../atel_resources.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace ffx { namespace utility_fixture_resources {
inline bool pending=false;
inline unsigned nextSerial=0;
inline std::vector<AtelPreparedHandler> preparedHandlers;
inline void finishPreparation() {pending=false;for(const auto handler:preparedHandlers) handler();}
struct Header {
    unsigned count=0,offset=0;
    std::vector<std::pair<unsigned,unsigned>> rows;
    std::map<std::pair<unsigned,unsigned>,unsigned> firstIndex;
};
struct Resource {
    AtelSpan span;
    AtelIdentity identity;
    std::map<unsigned,unsigned> workers;
    std::map<unsigned,Header> headers;
};
inline std::map<uintptr_t,Resource>& resources() {
    static std::map<uintptr_t,Resource> value;
    return value;
}
inline void prepareUtilityResources(uintptr_t chunk,const uint8_t* bytes,size_t size) {
    if(!chunk || !bytes || size<0x38) return;
    Resource ready;
    ready.span={chunk,generation(),++nextSerial,static_cast<unsigned>(size)};
    ready.identity.span=ready.span;
    ready.identity.length=0; std::memcpy(&ready.identity.length,bytes,4);
    std::memcpy(&ready.identity.codeOffset,bytes+0x30,4);
    uint16_t workers=0;std::memcpy(&workers,bytes+0x34,2);ready.identity.workers=workers;
    unsigned nameOffset=0;std::memcpy(&nameOffset,bytes+0xc,4);
    if(nameOffset<=size && size-nameOffset>=8) std::memcpy(ready.identity.name,bytes+nameOffset,8);
    if(ready.identity.codeOffset<=size && ready.identity.length<=size-ready.identity.codeOffset) {
        unsigned hash=2166136261u;
        for(unsigned i=0;i<ready.identity.length;++i) hash=(hash^bytes[ready.identity.codeOffset+i])*16777619u;
        ready.identity.checksum=hash;
    }
    for(unsigned local=0;local<workers && local<1024;++local) {
        const uint64_t table=0x38ull+static_cast<uint64_t>(local)*4;
        if(table+4>size) break;
        unsigned header=0;std::memcpy(&header,bytes+table,4);
        if(!header || header>size || size-header<0x28) continue;
        ready.workers.emplace(local,header);
        Header declarations;
        uint16_t count=0;std::memcpy(&count,bytes+header+2,2);declarations.count=count;
        std::memcpy(&declarations.offset,bytes+header+0x14,4);
        if(declarations.offset<=size && count<=(size-declarations.offset)/8) {
            declarations.rows.reserve(count);
            for(unsigned i=0;i<count;++i) {
                unsigned row[2]{};std::memcpy(row,bytes+declarations.offset+i*8,8);
                declarations.rows.emplace_back(row[0],row[1]);
                declarations.firstIndex.emplace(std::make_pair(row[0],row[1]),i);
            }
        } else {declarations.count=0;declarations.offset=0;}
        ready.headers.emplace(header,std::move(declarations));
    }
    resources()[chunk]=std::move(ready);
    finishPreparation();
}
inline Resource* find(uintptr_t chunk) {
    auto& all=resources();auto at=all.find(chunk);return at==all.end()?nullptr:&at->second;
}
} // namespace utility_fixture_resources

bool nativeAtelSpan(uintptr_t chunk,AtelSpan& span) {
    auto* resource=utility_fixture_resources::find(chunk);if(!resource) return false;
    span=resource->span;span.epoch=generation();return true;
}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& identity) {
    if(utility_fixture_resources::pending) return false;
    auto* resource=utility_fixture_resources::find(chunk);if(!resource) return false;
    identity=resource->identity;identity.span.epoch=generation();return true;
}
bool readyAtelWorkerHeader(uintptr_t chunk,unsigned local,unsigned& header) {
    if(utility_fixture_resources::pending) return false;
    auto* resource=utility_fixture_resources::find(chunk);if(!resource) return false;
    auto at=resource->workers.find(local);if(at==resource->workers.end()) return false;
    header=at->second;return true;
}
bool readyAtelDeclaration(uintptr_t chunk,unsigned header,unsigned low,unsigned high,unsigned& index) {
    if(utility_fixture_resources::pending) return false;
    auto* resource=utility_fixture_resources::find(chunk);if(!resource) return false;
    auto owner=resource->headers.find(header);if(owner==resource->headers.end()) return false;
    auto at=owner->second.firstIndex.find({low,high});if(at==owner->second.firstIndex.end()) return false;
    index=at->second;return true;
}
bool readyAtelDeclarationAt(uintptr_t chunk,unsigned header,unsigned index,unsigned& low,unsigned& high) {
    if(utility_fixture_resources::pending) return false;
    auto* resource=utility_fixture_resources::find(chunk);if(!resource) return false;
    auto owner=resource->headers.find(header);
    if(owner==resource->headers.end() || index>=owner->second.rows.size()) return false;
    low=owner->second.rows[index].first;high=owner->second.rows[index].second;return true;
}
AtelPreparation atelPreparation(uintptr_t chunk) {
    if(!utility_fixture_resources::find(chunk)) return AtelPreparation::Missing;
    return utility_fixture_resources::pending?AtelPreparation::Pending:AtelPreparation::Ready;
}
bool onAtelPrepared(AtelPreparedHandler handler) {
    if(!handler || utility_fixture_resources::preparedHandlers.size()>=8) return false;
    utility_fixture_resources::preparedHandlers.push_back(handler);return true;
}
} // namespace ffx
