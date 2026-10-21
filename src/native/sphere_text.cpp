// Capture Grid labels/material requirements through native definition/getter
// APIs into TextPlans. The material cache is keyed by table, generation and
// explicit revision; callers must invalidate it when those resources change.
// See Docs/features/menus/index.md#sphere-grid.
#include "sphere_text.hpp"
#include "ready_game_text.hpp"
#include "events.hpp"
#include "process_storage.hpp"
namespace ffx {
namespace {
struct Materials {uintptr_t table=0;unsigned epoch=0,revision=0;bool ready=false;std::array<unsigned,19> ids{};};
ProcessStorage<Materials> storage;
std::atomic<unsigned> materialRevision{1};
using Definition=uintptr_t(__cdecl*)(unsigned,uintptr_t,uintptr_t*);
unsigned material(unsigned key) {
    auto& cache=storage.get();const auto table=read<uintptr_t>(address(0x16860e4));
    const auto revision=materialRevision.load(std::memory_order_acquire);
    if(!cache.ready || cache.epoch!=generation() || cache.table!=table || cache.revision!=revision) {
        cache.ready=false;cache.ids={};cache.table=table;cache.epoch=generation();cache.revision=revision;
        if(!table) return 0;
        using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
        const auto getter=reinterpret_cast<Getter>(address(0x390a40));const auto definition=reinterpret_cast<Definition>(address(0x3ab890));
        for(unsigned i=0;i<256;++i) {
            const auto item=getter(i,nullptr);const int type=item?read<int8_t>(item+0x5d,-1):-1;
            const auto native=type<0?0:definition(type,table,nullptr);if(!native) continue;
            const unsigned special=read<uint8_t>(native+0xd);
            if(special>=1 && special<=4) {auto& row=cache.ids[14+special];if(!row) row=0x2000u+i;}
            else if(!special) {const auto flags=read<uint16_t>(native+0xa);for(unsigned bit=0;bit<15;++bit) if((flags&(1u<<bit)) && !cache.ids[bit]) cache.ids[bit]=0x2000u+i;}
        }
        cache.ready=true;
    }
    if(key>=0x10001 && key<=0x10004) return cache.ids[14+key-0x10000];
    for(unsigned i=0;i<15;++i) if(key==(1u<<i)) return cache.ids[i];return 0;
}
}
void prepareSphereText() {storage.prepare();}
void invalidateSphereText() {materialRevision.fetch_add(1,std::memory_order_acq_rel);}
SphereNodePlan planSphereNode(TextPlan& p,int index,bool materials) {
    SphereNodePlan result;const auto engine=read<uintptr_t>(address(0x1f05834));const auto table=read<uintptr_t>(address(0x16860e0));
    const int count=read<int16_t>(engine+2,-1);if(!nativeGameThread() || !engine || !table || count<1 || count>1024 || index<0 || index>=count) return result;
    const auto node=engine+0x808+index*0x28;const int type=read<int16_t>(node+6,-1);uintptr_t strings=0;
    const auto native=type<0?0:reinterpret_cast<Definition>(address(0x3ab890))(type,table,&strings);if(!native || !strings) return result;
    result.valid=true;result.flags=read<uint16_t>(native+0x10);result.activated=read<uint8_t>(node+0x21);
    const auto language=reinterpret_cast<unsigned(__cdecl*)()>(address(0x3852b0))()&1;
    result.name=planEncoded(p,strings+read<uint16_t>(native+language*4));result.description=planEncoded(p,strings+read<uint16_t>(native+8+language*4),1024,TextPlanOp::Lines);
    const unsigned ability=read<uint16_t>(native+0x12);if(ability) {
        const auto id=0x3000u|(ability&0xfff);result.name=planPair(p,TextPlanOp::Or,planRecordName(p,0x39a4c0,id),result.name);
        result.description=planPair(p,TextPlanOp::Or,planRecordDescription(p,0x39a4c0,id),result.description);
    }
    if(materials && storage.ready()) {
        const auto lock=read<uint8_t>(native+0x16,255);const unsigned key=result.flags&0x8000?lock==0x12?0x10001:lock==0x11?0x10002:lock==0?0x10003:lock==0x10?0x10004:0:result.flags?result.flags&(~result.flags+1u):0;
        result.sphere=material(key);
    }
    return result;
}
TextRef planSphereItemDescription(TextPlan& p,unsigned id) {
    const auto table=read<uintptr_t>(address(0x16860e4));using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    const auto item=reinterpret_cast<Getter>(address(0x390a40))(id,nullptr);const int type=item?read<int8_t>(item+0x5d,-1):-1;uintptr_t strings=0;
    const auto native=table && type>=0?reinterpret_cast<Definition>(address(0x3ab890))(type,table,&strings):0;
    if(!native || !strings) return planRecordDescription(p,0x390a40,id);
    const auto language=reinterpret_cast<unsigned(__cdecl*)()>(address(0x3852b0))()&1;
    return planEncoded(p,strings+read<uint16_t>(native+language*4),1024,TextPlanOp::Lines);
}
}
