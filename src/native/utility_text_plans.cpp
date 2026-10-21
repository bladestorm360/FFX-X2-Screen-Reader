#include "utility_text.hpp"
#include "text_plan.hpp"
#include "utilities.hpp"
#include <cstring>
namespace ffx {
unsigned appendUtilityTextPlan(TextPlan& plan,const UtilityText& value) {
    if(!value.valid || value.count>value.parts.size() || value.used>=value.text.size()) {plan.valid=false;return 0;}
    TextRef root=0;
    for(unsigned i=0;i<value.count;++i) {const auto& p=value.parts[i];TextRef part=0;
        if(p.kind==1) part=planInteger(plan,p.value);
        else {if(p.kind>2 || p.offset>value.used || p.length>value.used-p.offset) {plan.valid=false;return 0;}
            // Each part is copied separately: following parts need not have a
            // terminator in the carrier. No engine pointers enter the recipe.
            wchar_t ready[2048];if(p.length>=_countof(ready)) {plan.valid=false;return 0;}
            std::memcpy(ready,value.text.data()+p.offset,p.length*sizeof(wchar_t));ready[p.length]=0;part=planLiteral(plan,ready);}
        root=planConcat(plan,root,part);
    }
    return root;
}
namespace {
constexpr unsigned Receiver=8;
void receive(const speechq::Header& header,const TextPlan& plan,std::wstring line,std::wstring) {
    if(plan.epoch!=generation() || line.empty()) return;
    consoleReplyReady(line.data(),line.size());speechq::emitOwned(std::move(line),header,plan.producer,false,true);
}
}
void emitUtilityText(const UtilityText& text,const char* producer) {
    if(text.empty()) return;TextPlan plan;prepareTextPlan(plan,producer);plan.receiver=Receiver;plan.headline=appendUtilityTextPlan(plan,text);commitTextPlan(plan);
}
void installUtilityText() {registerTextPlanReceiver(Receiver,receive);}
}
