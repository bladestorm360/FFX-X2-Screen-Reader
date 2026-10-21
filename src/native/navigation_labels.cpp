#include "events.hpp"
#include "navigation_targets.hpp"
#include "navigation_models_data.hpp"
#include <algorithm>
#include <cstring>
#include <cwchar>
namespace ffx {
namespace {
const NavigationModel* modelInfo(unsigned model) {
    const auto first=std::begin(navigationModels),last=std::end(navigationModels);
    const auto found=std::lower_bound(first,last,model,[](const NavigationModel& item,unsigned id) { return item.model<id; });
    return found!=last && found->model==model?found:nullptr;
}
}
bool navigationSaveSphereModel(unsigned model) { return model==0x5001 || model==0x50ab; }
bool navigationActorPresence(const NavigationScript* script,unsigned model,bool actor) {
    // Dedicated CID257 model, independently identified by native NPC metadata
    // and original airship loaders. A present Cid stays identifiable even when
    // story-dependent Talk is a no-op. Never invent a missing/disabled actor.
    return actor && model==0x2101 && script && !std::strncmp(script->name,"hiku",4);
}
unsigned navigationEntityCategory(const NavigationTarget* target,unsigned model,bool actor) {
    const auto info=actor?modelInfo(model):nullptr;
    const bool character=info && info->kind==1;
    if(actor && navigationSaveSphereModel(model)) return 3;
    if(actor && model==0x5002) return 4;
    if(actor && model==0x2101) return 5;
    if(target) {
        if(target->category==5 && !character) return 6;
        return target->category;
    }
    return actor?(character?5:6):2;
}
std::wstring navigationEntityName(const NavigationTarget* target,unsigned model,bool actor) {
    return navigationEntityName(target,model,actor,[](int index) {return playerName(index);});
}
std::wstring navigationEntityName(const NavigationTarget* target,unsigned model,bool actor,const std::function<std::wstring(int)>& names) {
    if(actor && navigationSaveSphereModel(model)) return L"Save Sphere";
    const auto info=actor?modelInfo(model):nullptr;
    const bool nonCharacter=actor && (!info || info->kind!=1);
    // A catalog's stale actor/NPC identity cannot turn an actual book/prop into
    // a person. Keep confirmed functional object/commerce labels, not NPC names.
    if(nonCharacter && (!target || target->category==5 || target->identity<2))
        return info?info->label:L"Object";
    // Some shared airship dialogue contains another character's speaker name.
    // Dedicated, currently bound character models outrank that static inference.
    // Generic villagers still use verified actor-local speaker/service labels.
    if(info && info->kind==1 && model==0x2101) return L"Cid";
    if(info && info->kind==1 && target && (target->properName>=0 || target->identity>=2)) {
        const wchar_t* party[]={L"Tidus",L"Yuna",L"Auron",L"Kimahri",L"Wakka",L"Lulu",L"Rikku"};
        for(unsigned i=0;i<_countof(party);++i) if(!std::wcscmp(info->label,party[i])) {
            const auto current=names(static_cast<int>(i));return current.empty()?info->label:current;
        }
    }
    if(target && target->properName>=0) {
        const auto name=names(target->properName);
        if(!name.empty()) return name;
    }
    // Script speaker/service/role evidence has precedence over a generic body.
    // These are concise identities, never synthetic on-screen talk prompts.
    if(target && target->identity>=2) return target->label;
    if(info) return info->label;
    if(target && target->label[0]) return target->label;
    return actor?L"Object":L"Event";
}
}
