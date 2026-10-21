#include "sphere_owned.hpp"
#include "events.hpp"
#include "sphere_grid.hpp"
#include <map>
namespace ffx {
namespace {
using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
using Definition=uintptr_t(__cdecl*)(unsigned,uintptr_t,uintptr_t*);
unsigned language() { return reinterpret_cast<unsigned(__cdecl*)()>(address(0x3852b0))()&1; }
uintptr_t engine() { return read<uintptr_t>(address(0x1f05834)); }
uintptr_t definition(int index,uintptr_t* strings) {
    const auto grid=engine(),table=read<uintptr_t>(address(0x16860e0));
    const int count=read<int16_t>(grid+2);
    if(!grid || !table || index<0 || index>=count || count>1024) return 0;
    const int type=read<int16_t>(grid+0x80e+index*0x28,-1);
    return type<0?0:reinterpret_cast<Definition>(address(0x3ab890))(type,table,strings);
}
std::map<unsigned,unsigned> activationSpheres() {
    std::map<unsigned,unsigned> result;
    const auto table=read<uintptr_t>(address(0x16860e4));
    if(!table) return result;
    // The native item getter masks IDs to 12 bits. Resolve catalog records,
    // including items not owned, rather than deriving labels from inventory.
    for(unsigned id=0;id<256;++id) {
        uintptr_t strings=0;
        const auto item=reinterpret_cast<Getter>(address(0x390a40))(id,&strings);
        if(!item) continue;
        const int type=read<int8_t>(item+0x5d,-1);
        if(type<0) continue;
        const auto sphere=reinterpret_cast<Definition>(address(0x3ab890))(type,table,nullptr);
        if(!sphere) continue;
        const auto special=read<uint8_t>(sphere+0xd);
        if(special>=1 && special<=4) result.emplace(0x10000u+special,0x2000u+id);
        else if(!special) {
            const auto flags=read<uint16_t>(sphere+0xa);
            for(unsigned bit=1;bit<=0x4000;bit<<=1) if(flags&bit) result.emplace(bit,0x2000u+id);
        }
    }
    return result;
}
}
bool sphereGridOpen() { if(!nativeGameThread()) {bool open=false;sphereCopiedOpen(open);return open;} return read<uint8_t>(address(0x1685f70))!=0; }
std::wstring sphereItemDescription(unsigned id) {
    const auto table=read<uintptr_t>(address(0x16860e4));
    const auto item=reinterpret_cast<Getter>(address(0x390a40))(id,nullptr);
    const int type=item?read<int8_t>(item+0x5d,-1):-1;
    uintptr_t strings=0;
    const auto record=table && type>=0?reinterpret_cast<Definition>(address(0x3ab890))(type,table,&strings):0;
    return record && strings?join(decode(strings+read<uint16_t>(record+language()*4),1024),L" "):recordDescription(0x390a40,id);
}
namespace {
GridNode copyNode(int index,const std::map<unsigned,unsigned>& spheres) {
    GridNode result;
    if(index<0) return result;
    const auto node=engine()+0x808+index*0x28;
    result.type=read<int16_t>(node+6,-1); result.x=read<int16_t>(node); result.y=read<int16_t>(node+2);
    uintptr_t strings=0; const auto record=definition(index,&strings);
    if(!record || !strings) return result;
    result.valid=true; result.type=read<int16_t>(node+6,-1);
    result.x=read<int16_t>(node); result.y=read<int16_t>(node+2);
    result.flags=read<uint16_t>(record+0x10); result.locked=(result.flags&0x8000)!=0;
    result.ability=read<uint16_t>(record+0x12);
    result.activated=read<uint8_t>(node+0x21);
    const auto lang=language();
    result.name=first(strings+read<uint16_t>(record+lang*4));
    result.description=join(decode(strings+read<uint16_t>(record+8+lang*4),1024),L" ");
    if(result.ability) {
        const auto command=0x3000u|(result.ability&0xfff);
        const auto name=recordName(0x39a4c0,command);
        if(!name.empty()) result.name=name;
        const auto description=recordDescription(0x39a4c0,command);
        if(!description.empty()) result.description=description;
    }
    unsigned key=0;
    if(result.locked) {
        // Native availability cases 1..4 match node definition +16.
        const unsigned type=read<uint8_t>(record+0x16,255);
        key=type==0x12?0x10001:type==0x11?0x10002:type==0?0x10003:type==0x10?0x10004:0;
    } else if(result.flags) key=result.flags&(~result.flags+1u);
    const auto found=spheres.find(key); if(found!=spheres.end()) result.sphere=found->second;
    return result;
}
}
GridNode sphereNode(int index,bool materials) {
    const auto spheres=materials?activationSpheres():std::map<unsigned,unsigned>{};
    return copyNode(index,spheres);
}
bool sphereSnapshot(GridSnapshot& result,bool materials) {
    if(!nativeGameThread()) return sphereCopiedSnapshot(result);
    result={}; const auto grid=engine();
    if(!sphereGridOpen() || !grid) return false;
    const int nodes=read<int16_t>(grid+2,-1),links=read<int16_t>(grid+4,-1);
    if(nodes<1 || nodes>1024 || links<0 || links>1024) return false;
    result.selected=read<int16_t>(grid+0x112fc,-1); result.character=read<uint8_t>(grid+0x115bc,255);
    if(result.selected<0 || result.selected>=nodes || result.character>=7) return false;
    const auto spheres=materials?activationSpheres():std::map<unsigned,unsigned>{};
    for(int i=0;i<nodes;++i) result.nodes.push_back(copyNode(i,spheres));
    for(int i=0;i<links;++i) {
        const auto link=grid+0xa808+i*0x14;
        GridLink value{read<int16_t>(link,-1),read<int16_t>(link+2,-1),read<uint8_t>(link+0xc)};
        if(value.a<0 || value.b<0 || value.a>=nodes || value.b>=nodes || value.a==value.b) return false;
        if(result.nodes[value.a].valid && result.nodes[value.b].valid) result.links.push_back(value);
    }
    using Saved=uintptr_t(__cdecl*)(unsigned);
    for(unsigned i=0;i<7;++i) {
        result.positions[i]=read<int16_t>(grid+0x110cc+i*0x50,-1);
        const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(i);
        result.levels[i]=saved?read<uint8_t>(saved+0x3b):0;
    }
    return result.nodes[result.selected].valid;
}
}
