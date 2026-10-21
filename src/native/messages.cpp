#include "events.hpp"
#include "settings.hpp"
#include "dialogue_voice.hpp"
#include "movie_dialogue.hpp"
#include "message_text.hpp"
#include "owned_message_decoder.hpp"
#include "blitz_review.hpp"
#include "field_controls.hpp"
#include "sphere_grid.hpp"
#include "text_plan.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include "jecht_caption.hpp"
#include <atomic>
#include <cstring>
#include <deque>
namespace ffx {
namespace {
constexpr unsigned Receiver=4;
enum Command : unsigned {Page=1,Selector,Npc,Subtitle,Repeat};
struct TkState {uintptr_t window=0,cursor=0,text=0;int visible=0,row=0,count=0;};
struct Review {unsigned epoch=0,revision=0;uintptr_t window=0,source=0;int slot=-1;bool event=false;};
struct Context {TkState tk;Review review;FieldControlsSnapshot field;BlitzSnapshot blitz;JechtCaption jecht;unsigned slot=8;bool opening=false,changed=false,moved=false,title=false,tutorial=false,eventActive=false;};
struct Announced {uintptr_t window=0,text=0;};
struct PendingPage {speechq::Header header;Context context;MessagePage page;SelectionDetailToken detail;
    unsigned command=0;bool automatic=false;};
struct PendingCaption {speechq::Header header;JechtCaption token;std::wstring text;};
struct State {unsigned revision=0;std::wstring lastDialogue,reviewText;Review review;std::array<Announced,8> announced{};
    std::deque<PendingPage> pending;std::deque<PendingCaption> captions;unsigned captionCharacters=0;};
ProcessStorage<State> storage;
std::atomic<unsigned> stateRevision{1},reviewRevision{1};
Review nativeReview;async::AtomicSnapshot<Review> currentReview;
BlitzSnapshot nativeReviewBlitz;
thread_local unsigned installing=0;
TkState tk(unsigned slot) {
    if(slot>=8) return {};
    TkState s;s.window=read<uintptr_t>(address(0x14676b0)+slot*4);s.cursor=read<uintptr_t>(address(0x1468a50)+slot*4);
    if(!s.window || !s.cursor) return {};
    s.visible=read<uint8_t>(s.cursor+1);s.row=read<uint8_t>(s.cursor+2);s.count=read<uint8_t>(s.cursor+5);s.text=read<uintptr_t>(s.window+0x94);return s;
}
bool eventActive() {const auto controller=read<uintptr_t>(address(0xf26ae8));return scene!=0 || (controller && !(read<uint8_t>(controller)&2));}
bool sameReview(const Review& a,const Review& b) {
    return a.epoch==b.epoch && a.revision==b.revision && a.window==b.window && a.source==b.source && a.slot==b.slot && a.event==b.event;
}
void processPage(const PendingPage&);
void maintain() {
    auto& s=storage.get();const auto revision=stateRevision.load(std::memory_order_acquire);
    if(s.revision!=revision) {for(const auto& item:s.pending) releaseBlitzSnapshot(item.context.blitz);s={};s.revision=revision;}
    if(s.review.epoch && (s.review.epoch!=generation() || s.review.revision!=reviewRevision.load(std::memory_order_acquire))) {s.review={};s.reviewText.clear();}
    for(auto it=s.captions.begin();it!=s.captions.end();) {
        const auto state=jechtCaptionState(it->token);
        if(!state.ready) {++it;continue;}
        auto item=std::move(*it);it=s.captions.erase(it);
        s.captionCharacters-=static_cast<unsigned>(item.text.size());
        speechq::emitOwned(std::move(item.text),item.header,"jecht-subtitle",state.voiced,false);
    }
    while(!s.pending.empty()) {
        auto& item=s.pending.front();const auto status=blitzSnapshotPreparation(item.context.blitz);
        if(status==AtelPreparation::Pending) break;
        if(status!=AtelPreparation::Ready) item.context.blitz.valid=item.context.blitz.lobby=false;
        try {processPage(item);} catch(...) {releaseBlitzSnapshot(item.context.blitz);s.pending.pop_front();throw;}
        releaseBlitzSnapshot(item.context.blitz);s.pending.pop_front();
    }
}
void remember(const std::wstring& text,const Review& review,bool tutorial) {
    Review latest;
    if(review.revision!=reviewRevision.load(std::memory_order_acquire) || !currentReview.tryRead(latest) || !sameReview(latest,review)) return;
    auto& s=storage.get();s.review=review;s.reviewText=text;
    if(tutorial) protectTutorialSpeech(true);
}
void emit(const std::wstring& text,const speechq::Header& header,const char* producer,bool interrupt=false) {speechq::emitOwned(text,header,producer,false,interrupt);}
void jechtSubtitle(const std::wstring& text,const speechq::Header& header,const JechtCaption& token) {
    auto& state=storage.get();const auto voice=jechtCaptionState(token);
    if(!voice.ready && state.captions.size()<16 && text.size()<=speechq::CharacterLimit-state.captionCharacters) {
        state.captions.push_back({header,token,text});state.captionCharacters+=static_cast<unsigned>(text.size());
    } else speechq::emitOwned(text,header,"jecht-subtitle",voice.voiced,false);
}
void selector(const speechq::Header& header,SelectionDetailToken detail,bool automatic,const Context& c,const MessagePage& page) {
    if(c.tk.visible!=1 || !c.tk.window || (!c.opening && !c.changed && !c.moved)) return;
    auto& state=storage.get();
    if(page.selector && state.review.window==c.tk.window) clearDialogueReviewOwned();
    std::wstring focus;
    if(c.title) {static const wchar_t* labels[]={L"New Game",L"Load"};focus=c.tk.row<2?labels[c.tk.row]:L"?";}
    else if(c.tk.row>=0 && c.tk.row<static_cast<int>(page.choices.size())) focus=page.choices[c.tk.row];
    if(c.opening && c.title) {
        emit(L"Title Screen",header,"title",true);emit(focus,header,"title");
        if(detail.serial) emitSelectionDetailOwned({},detail,false,header);return;
    }
    auto& announced=state.announced[c.slot];
    if(c.opening || c.changed) {
        if(announced.window!=c.tk.window || announced.text!=c.tk.text) {emit(page.prompt,header,"window-prompt",true);announced={c.tk.window,c.tk.text};}
    }
    const bool moving=!c.opening && !c.changed;
    emit(focus,header,"window-focus",moving);
    if(detail.serial) emitSelectionDetailOwned(page.prompt,detail,moving && automatic,header);
}
void processPage(const PendingPage& item) {
    const auto& header=item.header;const auto& c=item.context;const auto& page=item.page;auto& state=storage.get();
    if(header.generation!=generation() || c.slot>=8) return;
    if(observeFieldControlsOwned(page,c.field)) {emit(page.prompt,header,"field-controls");return;}
    if(observeBlitzMessageOwned(c.slot,page,c.blitz)) return;
    // These exact original memory-show producers are plain taunt captions,
    // regardless of a TK cursor left visible by another window operation.
    if(c.jecht.epoch && !page.selector) {
        if(page.prompt.empty())return;
        remember(page.prompt,c.review,false);
        if(page.prompt!=state.lastDialogue) {
            state.lastDialogue=page.prompt;jechtSubtitle(page.prompt,header,c.jecht);
        }
        state.announced[c.slot]={c.tk.window,c.tk.text};return;
    }
    if(item.command==Selector || (item.command==Npc && c.tk.visible==1)) {selector(header,item.detail,item.automatic,c,page);return;}
    if(page.selector && state.review.window==c.tk.window) clearDialogueReviewOwned();
    if(page.prompt.empty()) return;
    if(!page.selector) remember(page.prompt,c.review,c.tutorial);
    if(page.selector) emit(page.prompt,header,"window-prompt");
    else if(page.prompt!=state.lastDialogue) {
        state.lastDialogue=page.prompt;
        submitDialogueOwned(page.prompt,header,"npc-dialogue");
    }
    state.announced[c.slot]={c.tk.window,c.tk.text};
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring line,std::wstring) {
    if(p.batch!=stateRevision.load(std::memory_order_acquire)) return;
    maintain();auto& state=storage.get();Context c;static_assert(sizeof(c)<=sizeof(p.context));std::memcpy(&c,p.context.data(),sizeof(c));
    if(p.command==Repeat) {if(sameReview(c.review,state.review)) emit(state.reviewText,header,"dialogue-repeat",true);return;}
    if(p.command==Subtitle) {
        if(!line.empty() && line!=state.lastDialogue) {
            if(c.eventActive) remember(line,c.review,false);
            state.lastDialogue=line;
            if(c.jecht.epoch) jechtSubtitle(line,header,c.jecht);
            else submitDialogueOwned(std::move(line),header,"event-dialogue");
        }return;
    }
    if(c.slot>=8 || !p.ordinal || p.ordinal>p.count) return;
    const auto& n=p.nodes[p.ordinal-1];if(n.offset>p.used || n.length>p.used-n.offset) return;
    PendingPage item;item.header=header;item.context=c;item.detail=p.detail;item.command=p.command;item.automatic=p.automatic;
    try {
        item.page=decodeOwnedMessage(p.bytes.data()+n.offset,n.length,copiedControlContext(p.controls));
        if(c.blitz.valid && blitzSnapshotPreparation(c.blitz)==AtelPreparation::Pending) {
            state.pending.push_back(std::move(item));return;
        }
        processPage(item);
    } catch(...) {releaseBlitzSnapshot(c.blitz);throw;}
    releaseBlitzSnapshot(c.blitz);
}
void candidate(Review& review,int slot,uintptr_t window,uintptr_t source,bool event) {
    review={generation(),reviewRevision.load(std::memory_order_acquire),window,source,slot,event};
    nativeReview=review;nativeReviewBlitz={};currentReview.publish(review);
}
bool reviewAvailable() {
    const auto& r=nativeReview;
    if(r.epoch!=generation() || r.revision!=reviewRevision.load(std::memory_order_acquire)) return false;
    if(nativeReviewBlitz.valid) {
        const auto state=blitzSnapshotPreparation(nativeReviewBlitz);
        if(state==AtelPreparation::Pending || state==AtelPreparation::Ready) return false;
    }
    if(r.slot<0) return r.event && eventActive();
    const auto s=tk(static_cast<unsigned>(r.slot));const auto state=read<uint16_t>(s.window+0x2c);
    return s.window && s.window==r.window && s.text==r.source && state>0 && state<3;
}
void repeatDialogue(bool) {
    if(!reviewAvailable()) return;
    TextPlan p;prepareTextPlan(p,"dialogue-repeat");p.receiver=Receiver;p.command=Repeat;p.batch=stateRevision.load();
    Context c;c.review=nativeReview;std::memcpy(p.context.data(),&c,sizeof(c));commitTextPlan(p);
}
void tkBefore(Event& e) {
    const auto s=tk(static_cast<unsigned>(e.args[0]));e.before[0]=s.visible;e.before[1]=s.row;e.before[2]=s.text;e.before[3]=s.window;
}
void capturePage(unsigned slot,Command command,const Event* e=nullptr) {
    const auto s=tk(slot);if(slot>=8 || !s.window) return;
    Context c;c.slot=slot;c.tk=s;c.tutorial=sphereGridOpen();
    c.jecht=captureJechtCaption(slot);
    c.opening=e && e->before[0]!=1;c.changed=e && (s.text!=static_cast<uintptr_t>(e->before[2]) || s.window!=static_cast<uintptr_t>(e->before[3]));c.moved=e && e->before[1]!=s.row;
    if(command==Selector && s.visible!=1) return;
    if(command==Selector && !c.opening && !c.changed && !c.moved) return;
    c.title=read<int>(address(0xefbbf8))==0x17 && slot==2;
    TextPlan p;prepareTextPlan(p,"message-page",false);p.receiver=Receiver;p.command=command;p.batch=stateRevision.load();
    p.ordinal=planMessage(p,slot,s.text);
    const bool selectorPage=p.ordinal && p.nodes[p.ordinal-1].value;
    if(!selectorPage) c.field=captureFieldControls(slot,s.text);
    else invalidateFieldControlsSlot(slot);
    if((selectorPage && nativeReview.window==s.window) || c.field.candidate) resetDialogueReview();
    c.blitz=captureBlitzSnapshot();if(command==Npc && c.blitz.valid) c.changed=true;
    const bool pendingName=c.blitz.lobby && slot==2 && blitzSnapshotPreparation(c.blitz)==AtelPreparation::Pending;
    if(!selectorPage && !c.field.candidate && (!(c.blitz.lobby && slot==2) || pendingName) && (command==Page || s.visible!=1)) {
        candidate(c.review,static_cast<int>(slot),s.window,s.text,false);
        if(pendingName) nativeReviewBlitz=c.blitz;
    }
    if(s.visible==1 && !c.field.candidate && !(c.blitz.lobby && slot==2)) p.detail=beginSelectionDetail();
    std::memcpy(p.context.data(),&c,sizeof(c));
    if(!commitTextPlan(p)) {if(c.field.candidate) rejectFieldControlsCapture(c.field);releaseBlitzSnapshot(c.blitz);}
}
void closed(Event& e) {if(nativeReview.slot==static_cast<int>(e.args[0])) resetDialogueReview();}
void eventEnded(Event&) {if(nativeReview.event) resetDialogueReview();}
void tkAfter(Event& e) {capturePage(static_cast<unsigned>(e.args[0]),Selector,&e);}
void npcBefore(Event& e) {tkBefore(e);++installing;}
void npcAfter(Event& e) {if(installing) --installing;capturePage(static_cast<unsigned>(e.args[0]),Npc,&e);}
void pageBefore(Event& e) {e.before[0]=read<uintptr_t>(e.args[0]+0x94);}
void pageAfter(Event& e) {
    if(installing || e.before[0]==static_cast<intptr_t>(read<uintptr_t>(e.args[0]+0x94))) return;
    const auto slot=read<uint16_t>(e.args[0]+4);if(slot<8 && tk(slot).window==e.args[0]) capturePage(slot,Page);
}
void subtitleAfter(Event& e) {
    if(scene==0xb || read<int>(address(0x8416b0))==0xb) return;
    TextPlan p;prepareTextPlan(p,"event-dialogue",false);p.receiver=Receiver;p.command=Subtitle;p.batch=stateRevision.load();
    p.headline=planEncoded(p,e.args[0],256,TextPlanOp::Lines,OwnedTextMode::Records);
    Context c;c.eventActive=eventActive();if(c.eventActive) candidate(c.review,-1,0,0,true);
    c.jecht=captureJechtCaption(0);if(!c.jecht.epoch)c.jecht=captureJechtCaption(1);
    std::memcpy(p.context.data(),&c,sizeof(c));commitTextPlan(p);
}
}
void installHelp();void installButtonGlyphs();void resetHelp();
void clearDialogueReviewOwned() {auto& s=storage.get();s.review={};s.reviewText.clear();protectTutorialSpeech(false);}
void resetDialogueReview() {reviewRevision.fetch_add(1,std::memory_order_acq_rel);protectTutorialSpeech(false);}
void resetMessageViews() {resetDialogueReview();stateRevision.fetch_add(1,std::memory_order_release);resetFieldControls();resetBlitzReview();resetHelp();}
void resetMessages() {resetMessageViews();resetDialogueVoice();resetMovieDialogue();}
void installMessages() {
    storage.prepare();registerTextPlanReceiver(Receiver,receive);registerTextPlanMaintenance(Receiver,maintain);
    installFieldControls();installBlitzReview();installButtonGlyphs();installDialogueVoice();installMovieDialogue();
    hook(0x376a50,nullptr,subtitleAfter);hook(0x4b8980,npcBefore,npcAfter);
    hook(0x4de350,pageBefore,pageAfter);hook(0x4de5b0,pageBefore,pageAfter);
    hook(0x4b5f60,tkBefore,tkAfter);hook(0x4b6260,tkBefore,tkAfter);hook(0x4b6490,tkBefore,tkAfter);
    hook(0x4b8510,nullptr,closed);hook(0x471a50,nullptr,eventEnded);
    hotkeyChordWhen('T',false,repeatDialogue,reviewAvailable);installHelp();
}
}
