#include "message_text.hpp"
#include "events.hpp"
#include "text_plan.hpp"
namespace ffx {
namespace {
void instruction(TextPlan& p,uint8_t kind,const uint8_t* bytes,unsigned count) {
    if(!p.valid || count>4 || p.used+count+2>=p.bytes.size()) {p.valid=false;return;}
    p.bytes[p.used++]=kind;p.bytes[p.used++]=static_cast<uint8_t>(count);
    for(unsigned i=0;i<count;++i) p.bytes[p.used++]=bytes[i];
    p.bytes[p.used]=0;
}
void captureMessage(unsigned slot,uintptr_t source,TextPlan& p,unsigned depth,unsigned& budget,bool& selector) {
    if(!source || !p.valid) return;
    if(depth>=8) {p.valid=false;return;}
    unsigned offset=0;
    for(;offset<0x1000 && budget && p.valid;) {
        uint8_t code=0,payload=0;const auto token=source+offset;
        if(!copy(token,&code,1)) {p.valid=false;return;}
        if(code<=2) return;
        unsigned width=1;
        if(code==4 || (code>=6 && code<=14) || code==16 || (code>=18 && code<=47)) {
            if(!copy(token+1,&payload,1)) {p.valid=false;return;}
            width=code==4 && payload<0x30?3:2;
        }
        uint8_t bytes[3]{};
        if(width>budget || offset+width>0x1000 || !copy(token,bytes,width)) {p.valid=false;return;}
        budget-=width;
        if(code==3) instruction(p,0,nullptr,0);
        else if(code==11) instruction(p,1,&payload,1);
        else if(code==16) {instruction(p,2,&payload,1);if(payload>=0x30 && payload<0x50) selector=true;}
        else if(code>=18 && code<=35 && code!=20 && code!=21 && code!=24) {
            using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t);
            const auto resolved=reinterpret_cast<Getter>(address(0x4b6610))(slot,token);
            if(resolved && resolved!=token) {
                instruction(p,5,nullptr,0);captureMessage(slot,resolved,p,depth+1,budget,selector);
                uint8_t next=0;copy(token+width,&next,1);instruction(p,6,&next,1);
            }
        } else if(code>=0x2b) {
            const auto window=read<uintptr_t>(address(0x14676b0)+slot*4);
            if(read<uint16_t>(window+0x10)==0x4000) {
                uint8_t converted[4]{};using Convert=int(__cdecl*)(uintptr_t,uint8_t*);
                reinterpret_cast<Convert>(address(0x506780))(token,converted);
                instruction(p,4,converted,sizeof(converted));
            } else instruction(p,3,bytes,width);
        }
        offset+=width;
    }
    // The whole page must fit the documented token/depth bounds. A natural
    // terminator exactly at the boundary is valid; a partial prefix is not.
    if(offset>=0x1000) {p.valid=false;return;}
    uint8_t end=0;
    if(p.valid && (!copy(source+offset,&end,1) || end>2)) p.valid=false;
}
}
unsigned planMessage(TextPlan& p,unsigned slot,uintptr_t source,bool rows) {
    if(!p.valid || slot>=8 || p.count==p.nodes.size()) {p.valid=false;return 0;}
    TextPlanNode n;n.op=rows?TextPlanOp::MessageRows:TextPlanOp::MessagePrompt;n.offset=p.used;
    unsigned budget=0x4000;bool selector=false;captureMessage(slot,source,p,0,budget,selector);
    n.length=p.used-n.offset;n.value=selector;
    if(!p.valid) return 0;
    p.nodes[p.count++]=n;return p.count;
}
}
