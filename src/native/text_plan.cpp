#include "text_plan.hpp"
#include "events.hpp"
#include "settings.hpp"
#include "owned_message_decoder.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>
namespace ffx {
namespace {
std::array<std::atomic<TextPlanReceiver>,9> receivers{};
std::array<std::atomic<speechq::DomainMaintenance>,9> maintenance{};
void maintain() {for(auto& entry:maintenance) if(const auto fn=entry.load(std::memory_order_acquire)) fn();}
TextRef node(TextPlan& p,TextPlanNode value) {
    if(!p.valid || p.count==p.nodes.size()) {p.valid=false;return 0;}
    p.nodes[p.count]=value;return ++p.count;
}
class Renderer {
    const TextPlan& p;
    const OwnedTextContext& context;
    std::array<std::wstring,TextPlanNodeLimit> values;
    const std::wstring empty;
    unsigned completed=0;
public:
    Renderer(const TextPlan& plan,const OwnedTextContext& controls) : p(plan),context(controls) {}
std::wstring render(TextRef root) {
    if(!root || root>p.count || p.count>p.nodes.size()) return {};
    if(root<=completed) return values[root-1];
    for(unsigned i=completed;i<root;++i) {
        const auto& n=p.nodes[i];auto& out=values[i];
        if(n.left>i || n.right>i) return {}; // Earlier nodes only; no recursive cycles.
        const auto& left=n.left?values[n.left-1]:empty;const auto& right=n.right?values[n.right-1]:empty;
        if(n.op==TextPlanOp::SettingValue) {
            const auto value=static_cast<int>(n.value);
            if(n.offset>5 || value<0 || (n.offset<4 && value>(n.offset==1?2:1))) return {};
            if(n.offset==1) {static const wchar_t* names[]={L"Suppress",L"Braille only",L"Speak anyway"};out=names[value];}
            else if(n.offset==2) out=value?L"Detailed":L"Normal";
            else if(n.offset==3) out=value?L"Polish":L"Auto";
            else if(n.offset==0) out=value?L"On":L"Off";
            else out=std::to_wstring(value)+(n.offset==4?L" steps":L" percent");
            continue;
        }
        if(n.op==TextPlanOp::PrimaryDodgeName) {out=context.primaryDodge;continue;}
        if(n.op==TextPlanOp::NativeButtonMask) {
            constexpr uint16_t masks[]={0x10,0x20,0x40,0x80,4,8,1,2,0x800,0x100,0x1000,0x2000,0x4000,0x8000};
            constexpr unsigned glyphs[]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x41,0x42,0x44,0x48};
            for(unsigned bit=0;bit<_countof(masks);++bit) if(n.value==masks[bit]) {const auto& name=context.glyphs[glyphs[bit]];if(name!=L"unassigned control") out=name;break;}
            continue;
        }
        if(n.op==TextPlanOp::Integer) {out=std::to_wstring(n.value);continue;}
        if(n.op==TextPlanOp::OrderedRows) {
            // Sort copied presentation coordinates on preparation, never on
            // the native render/input owner. Repeated turns stay separate.
            std::array<std::pair<int64_t,TextRef>,32> rows{};unsigned count=0,at=i+1;
            while(at && count<rows.size()) {
                const auto& row=p.nodes[at-1];
                if(row.op!=TextPlanOp::OrderedRows || row.right>=at || row.left>=at) return {};
                rows[count++]={row.value,row.right};at=row.left;
            }
            if(at) return {};
            std::reverse(rows.begin(),rows.begin()+count);
            std::stable_sort(rows.begin(),rows.begin()+count,[](const auto& a,const auto& b){return a.first<b.first;});
            for(unsigned j=0;j<count;++j) if(rows[j].second && !values[rows[j].second-1].empty()) {
                if(!out.empty())out+=L", ";out+=values[rows[j].second-1];
            }
            continue;
        }
        if(n.op==TextPlanOp::Concat) {out=left+right;continue;}
        if(n.op==TextPlanOp::Join || n.op==TextPlanOp::SpaceJoin || n.op==TextPlanOp::Periods) {
            out=left;if(!right.empty()) {if(!out.empty()) out+=n.op==TextPlanOp::Periods?L". ":n.op==TextPlanOp::SpaceJoin?L" ":L", ";out+=right;}continue;
        }
        if(n.op==TextPlanOp::IfText) {if(!left.empty()) out=right;continue;}
        if(n.op==TextPlanOp::Or) {out=left.empty()?right:left;continue;}
        if(n.op==TextPlanOp::Hp || n.op==TextPlanOp::Mp) {
            out=(n.op==TextPlanOp::Mp?L"MP ":L"HP ")+std::to_wstring(n.value);
            const auto maximum=static_cast<int>(n.offset);
            if(n.op==TextPlanOp::Mp || (maximum>0 && n.length)) out+=L"/"+std::to_wstring(n.op==TextPlanOp::Mp?static_cast<int64_t>(n.offset):maximum);
            else if(maximum>0) out+=L" ("+std::to_wstring(std::clamp<int64_t>((n.value*100+maximum/2)/maximum,0,100))+L"%)";
            continue;
        }
        if(n.op==TextPlanOp::BattleTarget) {
            out=left+L": ";const auto maximum=static_cast<int>(n.offset);
            if(!n.length) out+=std::to_wstring(maximum>0?static_cast<int>(std::floor(static_cast<double>(n.value)/maximum*100+.5)):0)+L"%";
            else out+=(n.length==2?L"MP ":L"HP ")+std::to_wstring(n.value)+L"/"+std::to_wstring(maximum);
            continue;
        }
        if(n.op==TextPlanOp::EquipmentName) {
            out=left;
            if(out.empty()) {recordLog("equipment name unavailable character=%u armor=%d index=%u",static_cast<unsigned>(n.value),n.length!=0,n.offset);out=L"Equipment name unavailable";}
            continue;
        }
        if(n.offset>p.used || n.length>p.used-n.offset) return {};
        if(n.op==TextPlanOp::Literal) {
            if(n.length%sizeof(wchar_t)) return {};
            out.resize(n.length/sizeof(wchar_t));std::memcpy(out.data(),p.bytes.data()+n.offset,n.length);continue;
        }
        if(n.op==TextPlanOp::MessagePrompt || n.op==TextPlanOp::MessageRows) {
            const auto page=decodeOwnedMessage(p.bytes.data()+n.offset,n.length,context);
            out=n.op==TextPlanOp::MessageRows?join(page.lines,L"\n"):page.prompt;continue;
        }
        const auto parts=decodeOwnedBytes(p.bytes.data()+n.offset,n.length,n.mode,context);
        if(n.op==TextPlanOp::First) {if(!parts.empty()) out=parts.front();}
        else if(n.op==TextPlanOp::Lines) out=join(parts,L" ");
        else if(n.op==TextPlanOp::LineBreaks) out=join(parts,L"\n");
        else if(n.op==TextPlanOp::Description && parts.size()>2 && parts[1]==L"-")
            out=join(std::vector<std::wstring>(parts.begin()+2,parts.end()),L" ");
    }
    completed=root;return values[root-1];
}
};
void process(const speechq::Header& header,const std::wstring& owned) {
    constexpr auto start=offsetof(TextPlan,bytes);
    if(owned.size()*sizeof(wchar_t)<start) return;
    TextPlan p;std::memcpy(&p,owned.data(),start);
    if(!p.valid || p.epoch!=generation() || p.used>p.bytes.size() || p.count>p.nodes.size() || p.followingCount>p.following.size() ||
        owned.size()*sizeof(wchar_t)<start+p.used) return;
    std::memcpy(p.bytes.data(),reinterpret_cast<const uint8_t*>(owned.data())+start,p.used);
    const auto context=copiedControlContext(p.controls);
    Renderer renderer(p,context);if(p.gate && renderer.render(p.gate).empty()) return;auto line=renderer.render(p.headline);
    if(p.epoch!=generation()) return;
    if(p.receiver) {
        if(p.receiver>=receivers.size()) return;
        const auto receiver=receivers[p.receiver].load(std::memory_order_acquire);
        if(receiver) receiver(header,p,std::move(line),renderer.render(p.description));
        return;
    }
    speechq::emitOwned(std::move(line),header,p.producer,false,p.interrupt);
    for(unsigned i=0;i<p.followingCount;++i) {
        const auto& output=p.following[i];
        if(p.epoch!=generation()) return;
        speechq::emitOwned(renderer.render(output.root),header,output.producer,false,output.interrupt);
    }
    if(p.detail.serial && p.epoch==generation()) {
        auto detail=renderer.render(p.description);
        if(p.append) appendSelectionDetailOwned(std::move(detail),p.detail);
        else emitSelectionDetailOwned(std::move(detail),p.detail,p.automatic,header);
    }
}
}
std::wstring renderTextPlan(const TextPlan& p,TextRef root,const OwnedTextContext& context) {
    return Renderer(p,context).render(root);
}
void prepareTextPlan(TextPlan& p,const char* producer,bool interrupt) {
    p.epoch=generation();p.interrupt=interrupt;prepareCopiedControlSnapshot(p.controls);
    p.automatic=setting(Setting::AutoDetail)!=0;
    unsigned i=0;if(producer) while(i+1<sizeof(p.producer) && producer[i]) {p.producer[i]=producer[i];++i;}p.producer[i]=0;
}
void addTextPlanOutput(TextPlan& p,TextRef root,const char* producer,bool interrupt) {
    if(p.followingCount==p.following.size()) {p.valid=false;return;}
    auto& output=p.following[p.followingCount++];output.root=root;output.interrupt=interrupt;
    unsigned i=0;if(producer) while(i+1<sizeof(output.producer) && producer[i]) {output.producer[i]=producer[i];++i;}output.producer[i]=0;
}
TextRef planLiteral(TextPlan& p,const wchar_t* value) {
    TextPlanNode n;n.offset=p.used;
    unsigned i=0;
    if(value) for(;i<2048 && value[i];++i) {
        if(p.used+sizeof(wchar_t)>=p.bytes.size()) {p.valid=false;return 0;}
        std::memcpy(p.bytes.data()+p.used,value+i,sizeof(wchar_t));p.used+=sizeof(wchar_t);n.length+=sizeof(wchar_t);
    }
    if(value && i==2048 && value[i]) {p.valid=false;return 0;}
    p.bytes[p.used]=0;return node(p,n);
}
TextRef planPrimaryDodgeName(TextPlan& p) {TextPlanNode n;n.op=TextPlanOp::PrimaryDodgeName;return node(p,n);}
TextRef planNativeButtonMask(TextPlan& p,uint16_t value) {TextPlanNode n;n.op=TextPlanOp::NativeButtonMask;n.value=value;return node(p,n);}
TextRef planInteger(TextPlan& p,int64_t value) {TextPlanNode n;n.op=TextPlanOp::Integer;n.value=value;return node(p,n);}
TextRef planSettingValue(TextPlan& p,int value,unsigned style) {TextPlanNode n;n.op=TextPlanOp::SettingValue;n.value=value;n.offset=style;return node(p,n);}
TextRef planBattleTarget(TextPlan& p,TextRef name,int current,int maximum,unsigned kind) {
    TextPlanNode n;n.op=TextPlanOp::BattleTarget;n.left=name;n.value=current;n.offset=static_cast<unsigned>(maximum);n.length=kind;return node(p,n);
}
TextRef planVitals(TextPlan& p,int64_t current,int64_t maximum,bool mp,bool showMaximumHp) {
    TextPlanNode n;n.op=mp?TextPlanOp::Mp:TextPlanOp::Hp;n.value=current;n.offset=static_cast<unsigned>(maximum);n.length=showMaximumHp;return node(p,n);
}
TextRef planEquipmentFallback(TextPlan& p,TextRef name,unsigned character,bool armor,unsigned id) {
    TextPlanNode n;n.op=TextPlanOp::EquipmentName;n.left=name;n.value=character;n.length=armor;n.offset=id;return node(p,n);
}
TextRef planEncoded(TextPlan& p,uintptr_t source,unsigned limit,TextPlanOp op,OwnedTextMode mode) {
    if(limit>0x10000 || limit>=p.bytes.size()-p.used) {p.valid=false;return 0;}
    TextPlanNode n;n.op=op;n.mode=mode;n.offset=p.used;
    if(mode==OwnedTextMode::Terminated || mode==OwnedTextMode::Literal)
        n.length=copyReadyGameText(source,p.bytes.data()+p.used,limit);
    else while(n.length<limit) {
        uint8_t byte=0;if(!copy(source+n.length,&byte,1)) break;p.bytes[p.used+n.length++]=byte;
    }
    p.used+=n.length;p.bytes[p.used]=0;return node(p,n);
}
TextRef planPair(TextPlan& p,TextPlanOp op,TextRef left,TextRef right) {
    TextPlanNode n;n.op=op;n.left=left;n.right=right;return node(p,n);
}
TextRef planConcat(TextPlan& p,TextRef left,TextRef right) {return planPair(p,TextPlanOp::Concat,left,right);}
TextRef planJoin(TextPlan& p,TextRef left,TextRef right) {return planPair(p,TextPlanOp::Join,left,right);}
TextRef planOrderedRow(TextPlan& p,TextRef previous,TextRef text,int64_t order) {
    TextPlanNode n;n.op=TextPlanOp::OrderedRows;n.left=previous;n.right=text;n.value=order;return node(p,n);
}
TextRef planPrefix(TextPlan& p,TextRef prefix,TextRef text,const wchar_t* separator) {
    const auto line=planConcat(p,planConcat(p,prefix,planLiteral(p,separator)),text);
    return planPair(p,TextPlanOp::Or,planPair(p,TextPlanOp::IfText,prefix,line),text);
}
TextRef planSuffix(TextPlan& p,TextRef text,TextRef suffix,const wchar_t* separator) {
    const auto line=planConcat(p,text,planConcat(p,planLiteral(p,separator),suffix));
    return planPair(p,TextPlanOp::Or,planPair(p,TextPlanOp::IfText,suffix,line),text);
}
bool commitTextPlan(const TextPlan& p) {
    if(!p.valid || p.used>p.bytes.size() || p.count>p.nodes.size()) {recordLog("native text plan capacity refused producer=%s",p.producer);return false;}
    // Transmit only initialized ready bytes. The rounded padding byte is owned
    // zero-initialized storage; callbacks never allocate a variable packet.
    const auto bytes=static_cast<unsigned>(offsetof(TextPlan,bytes))+(p.used+1u)/2u*2u;
    return speechq::captureDomain(speechq::TextPlanDomain,&p,bytes,nullptr,0,p.epoch);
}
void installTextPlans() {
    speechq::registerDomain(speechq::TextPlanDomain,process,nullptr);
    speechq::registerDomainMaintenance(speechq::TextPlanDomain,maintain);
}
void registerTextPlanMaintenance(unsigned id,speechq::DomainMaintenance fn) {
    if(id && id<maintenance.size()) maintenance[id].store(fn,std::memory_order_release);
}
void registerTextPlanReceiver(unsigned id,TextPlanReceiver receiver) {
    if(id && id<receivers.size()) receivers[id].store(receiver,std::memory_order_release);
}
}
