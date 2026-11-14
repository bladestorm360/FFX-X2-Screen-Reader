#pragma once
#include "copied_controls.hpp"
#include "selection_details.hpp"
namespace ffx {
using TextRef=unsigned;
constexpr unsigned TextPlanNodeLimit=128,TextPlanByteLimit=98304+16384;
enum class TextPlanOp : unsigned {Literal,Integer,First,Lines,Description,Concat,Join,SpaceJoin,Periods,IfText,Or,Hp,Mp,EquipmentName,MessagePrompt,MessageRows,BattleTarget,SettingValue,NativeButtonMask,PrimaryDodgeName,LineBreaks,OrderedRows};
struct TextPlanNode {
    TextPlanOp op=TextPlanOp::Literal;
    unsigned left=0,right=0,offset=0,length=0;
    OwnedTextMode mode=OwnedTextMode::Terminated;
    int64_t value=0;
};
struct TextPlanOutput {TextRef root=0;char producer[64]{};bool interrupt=false;};
// Native builders only copy bounded ready data and describe formatting. All
// text decoding, emptiness decisions, joins and numeric formatting are worker work.
struct TextPlan {
    CopiedControls controls;
    std::array<TextPlanNode,TextPlanNodeLimit> nodes{};
    SelectionDetailToken detail;
    char producer[64]{};
    unsigned epoch=0,count=0,used=0,headline=0,description=0,gate=0;
    unsigned receiver=0,command=0,serial=0,batch=0,ordinal=0,flags=0;
    // Receiver-specific copied scalars/opaque identities; never worker-dereferenced engine pointers.
    std::array<intptr_t,64> context{};
    std::array<TextPlanOutput,2> following{};
    unsigned followingCount=0;
    bool valid=true,interrupt=true,automatic=false,append=false;
    // Variable payload is written up to used; avoid zeroing large unused storage on Native.
    std::array<uint8_t,TextPlanByteLimit> bytes;
};
void prepareTextPlan(TextPlan&,const char* producer,bool interrupt=true);
void addTextPlanOutput(TextPlan&,TextRef,const char* producer,bool interrupt=false);
TextRef planLiteral(TextPlan&,const wchar_t*);
TextRef planInteger(TextPlan&,int64_t);
TextRef planNativeButtonMask(TextPlan&,uint16_t);
TextRef planPrimaryDodgeName(TextPlan&);
TextRef planSettingValue(TextPlan&,int value,unsigned style); // 0=toggle,1=voiced,2=combat,3=Polish,4=steps,5=percent.
TextRef planBattleTarget(TextPlan&,TextRef name,int current,int maximum,unsigned kind);
TextRef planVitals(TextPlan&,int64_t current,int64_t maximum,bool mp,bool showMaximumHp);
TextRef planEquipmentFallback(TextPlan&,TextRef name,unsigned character,bool armor,unsigned id);
TextRef planEncoded(TextPlan&,uintptr_t,unsigned limit=256,TextPlanOp=TextPlanOp::First,
    OwnedTextMode=OwnedTextMode::Terminated);
TextRef planPair(TextPlan&,TextPlanOp,TextRef,TextRef);
TextRef planConcat(TextPlan&,TextRef,TextRef);
TextRef planJoin(TextPlan&,TextRef,TextRef); // Comma-separated, skipping empty values.
TextRef planOrderedRow(TextPlan&,TextRef previous,TextRef text,int64_t order);
TextRef planPrefix(TextPlan&,TextRef prefix,TextRef text,const wchar_t* separator);
TextRef planSuffix(TextPlan&,TextRef text,TextRef suffix,const wchar_t* separator);
bool commitTextPlan(const TextPlan&);
void installTextPlans();
using TextPlanReceiver=void(*)(const speechq::Header&,const TextPlan&,std::wstring,std::wstring);
void registerTextPlanMaintenance(unsigned,speechq::DomainMaintenance);
void registerTextPlanReceiver(unsigned,TextPlanReceiver); // Bootstrap publication; receiver owns worker buffers.
// Preparation only, also used by focused synthetic fixtures.
std::wstring renderTextPlan(const TextPlan&,TextRef,const OwnedTextContext&);
}
