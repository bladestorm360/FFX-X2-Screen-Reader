#include "external_fixture.hpp"
#include "../jecht.hpp"
#include "../lightning.hpp"
#include "../jecht_data.hpp"
#include "../jecht_voice.hpp"
#include "../jecht_caption.hpp"
#include <thread>
#include "../events.hpp"
#include "../atel_resources.hpp"
#include "../native_worker_index.hpp"
#include "../settings.hpp"
#include "../text_plan.hpp"
#include "native_heap_counter.hpp"
#include <map>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <algorithm>
namespace {
using namespace ffx;
struct Listener {Handler before,after;};
std::map<unsigned,std::vector<Listener>> listeners;
std::vector<uint8_t> resource,worker(0x600),signal(0x20),controller(0x80),saved(0x1000);
unsigned epoch=1,which=0;size_t start=0;
bool owner=true,draining=false,focused=true,battle=false,menu=false,console=false,automatic=true,cues=true;
unsigned sounds=0,plans=0,lastMask=0,getters=0,ticks=10000;bool lastInterrupt=false;
std::atomic<unsigned> audioOwner{0};
struct AudioRecord {uintptr_t event=0;unsigned playing=0;};
struct AudioGroup {unsigned count=0,capacity=0;uintptr_t records=0;unsigned padding[7]{};};
AudioRecord audioRecords[2][16]{};AudioGroup audioGroups[2]{};unsigned nextAudioEvent=0x6000000;
constexpr unsigned callIds[]={0x4c,0x4d,0x50,0x51,0x122};
constexpr auto& callRvas=jecht_rva::queries;
uintptr_t ptr(std::vector<uint8_t>& b) {return reinterpret_cast<uintptr_t>(b.data());}
uintptr_t chunk() {return ptr(resource)+start;}
void check(bool value,const char* text) {if(!value){fprintf(stderr,"FAIL: %s\n",text);exit(1);}}
template<class T> void put(std::vector<uint8_t>& b,unsigned offset,T v) {check(offset+sizeof(v)<=b.size(),"host extent");memcpy(b.data()+offset,&v,sizeof(v));}
uintptr_t __cdecl getWorker(int global) {check(owner && !draining,"engine worker getter owner");++getters;return global==0?ptr(worker):0;}
uintptr_t __cdecl variable(uintptr_t w,int index,int) {
    check(owner && !draining,"engine variable getter owner");++getters;
    const auto low=read<unsigned>(read<uintptr_t>(w+4)+read<unsigned>(read<uintptr_t>(w)+0x14)+index*8);
    return ((low>>25&7)==0?ptr(saved)+0x1ec:chunk()+read<unsigned>(chunk()+0x20))+(low&0xffffff);
}
void value(unsigned index,int v) {const auto low=read<unsigned>(chunk()+read<unsigned>(read<uintptr_t>(ptr(worker))+0x14)+index*8);
    if(low>>28==5)write<int>(variable(ptr(worker),index,0),v);else write<uint8_t>(variable(ptr(worker),index,0),static_cast<uint8_t>(v));}
void location(unsigned local,unsigned entry,unsigned pc) {
    if(read<uint16_t>(ptr(worker)+0x2e,0xffff)!=local) invalidateNativeWorkerIndex();
    put(worker,0,chunk()+read<unsigned>(chunk()+0x38+local*4));put(worker,4,chunk());put(worker,0x2e,static_cast<uint16_t>(local));
    put(worker,0x90,ptr(signal));put(worker,0x32,uint8_t{1});put(signal,8,static_cast<uint16_t>(entry));put(signal,0xe,uint8_t{1});
    put(worker,0x12c+0x4c+0x18,chunk()+read<unsigned>(chunk()+0x30)+pc);
}
void load(unsigned variant,bool prepare=true,bool changed=false) {
    if(!resource.empty()){Event freed;freed.args[0]=ptr(resource);for(auto& l:listeners.at(0x48d490))if(l.before)l.before(freed);}
    resetJecht();++epoch;which=variant;
    const std::string name=jechtScripts[which].name;
    std::ifstream f((ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/sw").string()+"/"+name+"/"+name+".ebp",std::ios::binary);
    check(f.good(),"retained original Winno EBP");resource.assign(std::istreambuf_iterator<char>(f),{});start=read<unsigned>(ptr(resource)+4);
    check(start+0x38<resource.size(),"exact retained EBP extent");
    if(changed)resource[start+read<unsigned>(chunk()+0x30)]^=1;
    Event loaded;loaded.args[0]=12;loaded.nativeStack=0x910000;loaded.epoch=epoch;
    listeners.at(0x48d230).front().before(loaded);
    Event allocated;allocated.caller=address(0x48d230)+0x74;allocated.edi=static_cast<unsigned>(resource.size());
    allocated.args[0]=(allocated.edi+0x7ffu)&~0x7ffu;allocated.result=ptr(resource);allocated.epoch=epoch;
    listeners.at(0x2871c0).front().before(allocated);listeners.at(0x2871c0).front().after(allocated);
    loaded.result=ptr(resource);listeners.at(0x48d230).front().after(loaded);
    std::fill(worker.begin(),worker.end(),uint8_t{0});std::fill(signal.begin(),signal.end(),uint8_t{0});std::fill(controller.begin(),controller.end(),uint8_t{0});
    put(controller,0xc,uint16_t{1});put(controller,0x1c,ptr(worker));
    focused=automatic=cues=owner=true;battle=menu=console=false;sounds=plans=lastMask=0;scene=0;
    const auto& s=jechtScripts[which];location(s.local,s.entry,s.entryPc);
    check(s.tutorialEntry==(which?89u:7u) && s.tutorialPc==(which?70289u:23099u),
        "practice entry matches independent archived callers/entry table in both original scripts");
    value(s.active[0],1);value(s.active[1],1);value(s.selector[0],0);value(s.selector[1],0);value(s.timer,1);value(s.score,0);

    if(prepare){draining=true;drainAtelResourcesForTest();draining=false;}
}
Event queryAt(const JechtQuery& q,uintptr_t physical=0,unsigned parent=0x464180,unsigned eventEpoch=0,bool settle=true) {
    location(q.local,q.entry,q.pc);
    auto invoke=[&]() {
        Event entry;entry.rva=0x464180;entry.args[0]=ptr(worker);entry.epoch=epoch;
        for(auto& l:listeners.at(entry.rva))if(l.before)l.before(entry);
        Event e;e.parentRva=parent;e.epoch=eventEpoch?eventEpoch:epoch;e.args[0]=ptr(worker);e.result=physical;
        for(unsigned i=0;i<std::size(callIds);++i)if(callIds[i]==q.call)e.rva=callRvas[i];
        for(auto& l:listeners.at(e.rva))if(l.after)l.after(e);
        for(auto& l:listeners.at(entry.rva))if(l.after)l.after(entry);
        return e;
    };
    auto result=invoke();
    // Replay a later original input tick after the documented silent-audio
    // fallback. Focused timing cases below do not use this settling helper.
    if(settle && automatic && q.slot<2 && !result.overrideResult) {ticks+=1000;result=invoke();}
    if(settle)ticks+=150;
    return result;
}

// Execute the original predicate bytecode, rather than reconstructing its
// boolean expression from the generated answer table. Dependencies are the
// production Jecht callbacks and controlled physical/analog input only.
bool predicate(unsigned pc,bool tutorial=false,bool confirm=false,uintptr_t axis=0) {
    std::vector<int> stack;const auto code=chunk()+read<unsigned>(chunk()+0x30);
    auto pop=[&](){check(!stack.empty(),"original predicate stack");int v=stack.back();stack.pop_back();return v;};
    for(unsigned count=0;count<96;++count){const auto op=read<uint8_t>(code+pc);const auto arg=read<uint16_t>(code+pc+1);
        if(op==0xae){stack.push_back(static_cast<int16_t>(arg));pc+=3;continue;}
        if(op==0xb5){const int input=pop();(void)input;const auto& s=jechtScripts[which];
            JechtQuery q{which,tutorial?s.tutorial:s.local,tutorial?s.tutorialEntry:s.entry,pc,arg,0,2,0,0};
            check(arg==0x4c||arg==0x4d||arg==0x50||arg==0x51||arg==0x122,"original input predicate call");
            const auto physical=arg==0x122?axis:(arg==0x4d||arg==0x51?uintptr_t{confirm}:0);
            stack.push_back(static_cast<int>(queryAt(q,physical,0x464180,0,false).result));pc+=3;continue;}
        if(op==0x19){const int v=pop();stack.push_back(!v);++pc;continue;}
        if(op==0xd7)return pop()!=0;
        int right=pop(),left=pop();switch(op){case 1:stack.push_back(left||right);break;case 2:stack.push_back(left&&right);break;
            case 8:case 10:stack.push_back(left>right);break;case 9:case 11:stack.push_back(left<right);break;
            default:fprintf(stderr,"Original predicate opcode=%x pc=%x\n",op,pc);check(false,"unexpected original predicate opcode");}++pc;
    }check(false,"original predicate work bound");return false;
}
void tutorialFixtures(unsigned variant) {
    load(variant);const auto& s=jechtScripts[variant];
    // Independent original block PCs: Up Right, Down, center, Down Right.
    constexpr unsigned blocks[2][4]={{23639,23773,23927,24094},{70835,70969,71123,71290}};
    for(const auto pc:blocks[variant])for(const auto physicalAxis:{uintptr_t{0},uintptr_t{255}})
        check(predicate(pc,true,false,physicalAxis),"all four original tutorial predicates auto-complete, including neutral center");
    automatic=false;
    check(!predicate(blocks[variant][2],true,false,128),"manual centered tutorial still requires Confirm");
    check(predicate(blocks[variant][2],true,true,128),"manual centered tutorial accepts physical Confirm with neutral axes");
    check(!predicate(blocks[variant][2],true,true,0),"manual centered tutorial preserves original directional/axis restrictions");
    automatic=true;
    // These are the two explanation/example advance loops, not practice.
    for(const auto pc:variant?std::initializer_list<unsigned>{70410,70501}:std::initializer_list<unsigned>{23220,23311}) {
        JechtQuery q{variant,s.tutorial,s.tutorialEntry,pc,0x4d,1,2,0,0x20};
        check(!queryAt(q,0).overrideResult && queryAt(q,1).result==1,"explanation advance remains physical even with assistance on");
    }
    printf("PASS: script%u all four original tutorial predicates, neutral-center assistance, manual Confirm/axis preservation and explanatory-page exclusion.\n",variant);
}
}
namespace ffx {
namespace speechq {void notifyPreparation() noexcept {}}
std::atomic<int> scene{0};
unsigned jechtTickForTest(){return ticks;}
unsigned generation(){return epoch;}void log(const char*,...){}bool nativeGameThread(){return owner;}
bool gameFocused(){return focused;}bool inBattle(){return battle;}bool modMenuActive(){return menu;}bool utilityConsoleActive(){return console;}
bool controllerOwned(){return false;}std::wstring nativeBindingName(unsigned,bool){return L"control";}
void navAudioLightningWarning(unsigned){check(false,"Winno cannot emit a Thunder Plains cue");}void navAudioCancelLightningWarning(){}
int setting(Setting s){return s==Setting::JechtAssist?automatic:s==Setting::JechtCues?cues:0;}
bool copy(uintptr_t p,void* d,size_t n){check(!draining,"preparation reads copied resources only");
    const auto groupStart=reinterpret_cast<uintptr_t>(audioGroups),recordStart=reinterpret_cast<uintptr_t>(audioRecords);
    if((p>=groupStart && p<groupStart+sizeof(audioGroups)) || (p>=recordStart && p<recordStart+sizeof(audioRecords)))
        check(GetCurrentThreadId()==audioOwner.load(),"audio objects are read only by the original audio owner");__try{memcpy(d,reinterpret_cast<void*>(p),n);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
uintptr_t address(unsigned rva){if(!rva)return 0;
    if(rva==0x46a7c0)return reinterpret_cast<uintptr_t>(getWorker);if(rva==0x46c500)return reinterpret_cast<uintptr_t>(variable);
    if(rva==0xf26ae8){static uintptr_t p;p=ptr(controller);return reinterpret_cast<uintptr_t>(&p);}return rva;}
bool hook(unsigned r,Handler a,Handler b){listeners[r].push_back({a,b});return true;}
bool hookSnapshot(unsigned r,Handler a,Handler b){return hook(r,a,b);}bool hookAddress(uintptr_t,unsigned id,Handler a,Handler b){return hook(id,a,b);}
void prepareTextPlan(TextPlan& p,const char*,bool interrupt){p.count=0;p.valid=true;p.interrupt=interrupt;}
TextRef planNativeButtonMask(TextPlan& p,uint16_t mask){check(mask && !(mask&(mask-1)),"renderer receives single configured controls");p.nodes[p.count].op=TextPlanOp::NativeButtonMask;p.nodes[p.count].value=mask;return ++p.count;}
TextRef planLiteral(TextPlan&,const wchar_t*){check(false,"Jecht cue contains no instructional filler or plus literals");return 0;}
TextRef planPair(TextPlan& p,TextPlanOp op,TextRef left,TextRef right){
    check(op==TextPlanOp::SpaceJoin || op==TextPlanOp::IfText,"Jecht cue joins control names with spaces only");
    auto& n=p.nodes[p.count];n.op=op;n.left=left;n.right=right;return ++p.count;
}
TextRef planConcat(TextPlan&,TextRef,TextRef){check(false,"Jecht cue contains no verbose concatenation");return 0;}
bool commitTextPlan(const TextPlan& p){
    ++plans;lastMask=0;lastInterrupt=p.interrupt;unsigned controls=0,joins=0;
    for(unsigned i=0;i<p.count;++i) {
        const auto& n=p.nodes[i];
        if(n.op==TextPlanOp::NativeButtonMask){lastMask|=static_cast<unsigned>(n.value);++controls;}
        else if(n.op==TextPlanOp::SpaceJoin)++joins;
    }
    check(controls && joins+1==controls,"all required cue controls form one space-separated sequence");return true;
}
void navAudioOverdriveBeep(float,unsigned e){check(e==epoch,"cue generation");++sounds;}
}
#include "jecht_audio_followups.inc"
#include "jecht_round_followups.inc"
int main(){using namespace ffx;
    check(startAtelResources(),"preparation startup");installAtelResources();installLightning();installJecht();
    unsigned count=0;double maxQueryUs=0,maxCueUs=0;
    for(unsigned variant=0;variant<2;++variant){tutorialFixtures(variant);fullRoundFixtures(variant);load(variant);const auto& s=jechtScripts[variant];
        for(const auto& q:jechtQueries)if(q.script==variant){if(q.slot<2)value(s.selector[q.slot],q.value);
            check(read<uint8_t>(chunk()+read<unsigned>(chunk()+0x30)+q.pc)==0xb5,"exact original CALL cursor");
            nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;const auto before=std::chrono::steady_clock::now();const auto result=queryAt(q);
            maxQueryUs=std::max(maxQueryUs,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-before).count());nativeHeapFixture::observing=false;
            check(result.overrideResult && result.result==q.expected,"practice/real original input answer");
            check(nativeHeapFixture::allocations==0 && nativeHeapFixture::frees==0,"Native heap0/0");++count;
        }
        // Execute all nine predicates from the original bytecode.
        unsigned predicates=0;for(const auto& q:jechtQueries)if(q.script==variant && q.slot<2 && q.call==0x50){
            const auto code=chunk()+read<unsigned>(chunk()+0x30);
            // Each predicate begins immediately after its prior JMP or starts
            // the exact original branch listed by its first CALL in PC order.
            bool first=true;for(const auto& previous:jechtQueries)if(previous.script==variant && previous.slot==q.slot && previous.value==q.value && previous.pc<q.pc)first=false;
            if(!first)continue;(void)code;value(s.selector[q.slot],q.value);
            resetJecht();const auto source=*std::find_if(std::begin(jechtCues),std::end(jechtCues),[&](const auto& c){return c.script==variant && c.local==s.local+q.slot+1 && c.entry==q.value+6;});
            soundSource(source);showCue(source);ticks+=750;
            if(!predicate(q.pc-3)) {fprintf(stderr,"SAME-TICK predicate failed script=%u slot=%u value=%u pc=%u\n",variant,q.slot,q.value,q.pc);check(false,"original same-tick direction predicate succeeds");}
            value(s.selector[q.slot],(q.value+1)%(q.slot?4:5));check(!predicate(q.pc-3),"wrong pending cue predicate stays false");++predicates;
        }check(predicates==9,"all nine original predicates");
        auto q=*std::find_if(std::begin(jechtQueries),std::end(jechtQueries),[&](const auto& x){return x.script==variant && x.slot<2;});
        value(s.selector[q.slot],q.value);value(s.timer,300);check(!queryAt(q).overrideResult,"expired real cue");value(s.timer,1);
        automatic=false;check(!queryAt(q,true).overrideResult && queryAt(q,true).result,"manual physical input preserved");automatic=true;
        focused=false;check(!queryAt(q).overrideResult,"focus lost");focused=true;menu=true;check(!queryAt(q).overrideResult,"mod menu");menu=false;
        owner=false;check(!queryAt(q).overrideResult,"foreign native owner");owner=true;
        check(!queryAt(q,false,0x123).overrideResult && !queryAt(q,false,0x464180,epoch-1).overrideResult,"wrong ancestry/generation");
        put(controller,0x1a,uint16_t{3});check(!queryAt(q).overrideResult,"segmented owner rejected");put(controller,0x1a,uint16_t{0});
        automatic=false;
        for(const auto& cue:jechtCues)if(cue.script==variant){
            value(s.selector[cue.local-s.local-1],static_cast<int>(cue.entry-6));
            const auto before=std::chrono::steady_clock::now();
            nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
            showCue(cue);
            maxCueUs=std::max(maxCueUs,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-before).count());nativeHeapFixture::observing=false;
            check(nativeHeapFixture::allocations==0 && nativeHeapFixture::frees==0,"Native cue heap0/0");
            check(lastMask==cue.mask,"actual visible cue input mask");}
        check(plans==9 && sounds==9,"nine distinct original visible cues");
        const auto cue=*std::find_if(std::begin(jechtCues),std::end(jechtCues),[&](const auto& x){return x.script==variant;});
        showCue(cue);showCue(cue);
        check(plans==11 && sounds==11,"equal consecutive gameplay cues both speak and beep without deduplication");
        const auto prior=plans;
        value(s.active[0],0);value(s.selector[0],4);value(s.timer,300);showCue(cue);
        check(plans==prior+1 && sounds==prior+1,"manual cue has no tutorial/pending filter");
        value(s.active[0],1);value(s.selector[0],0);value(s.timer,1);
        pacingFixtures(variant);
        if(variant==1) {
            auto wrong=*std::find_if(std::begin(jechtQueries),std::end(jechtQueries),[](const auto& x){return x.script==1 && x.slot==2;});
            automatic=true;check(queryAt(wrong).overrideResult,"actual second-script entry89 practice is assisted");
            wrong.entry=7;check(!queryAt(wrong).overrideResult,"second-script dialogue entry7 cannot claim entry89 practice input");automatic=false;
        }
        load(variant,false);check(!queryAt(q).overrideResult,"pending resource cannot solve");draining=true;drainAtelResourcesForTest();draining=false;
        value(s.selector[q.slot],q.value);check(queryAt(q).overrideResult,"qualification readiness recovers without negative cache");
        load(variant,true,true);check(!queryAt(q).overrideResult,"modified original bytecode refused");
    }
    printf("PASS: %u exact Jecht queries, original nine-position predicates in both scripts, ungated manual cues, 750ms reaction/spacing and original score11 cutoff, SFX caption classification/failure/cancellation, consecutive equal speech/beeps, lifecycle/focus/owner/age guards and Native heap0/0. Maximum controlled Native query %.1fus, cue capture %.1fus. Gameplay/backend timing unconfirmed.\n",count,maxQueryUs,maxCueUs);
}
