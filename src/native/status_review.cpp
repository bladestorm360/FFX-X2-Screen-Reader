#include "events.hpp"
#include "settings.hpp"
#include "sphere_grid.hpp"
#include "status_review.hpp"
#include "ready_game_text.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include "esc_details.hpp"
#include <atomic>
#include <algorithm>
namespace ffx {
void resetStatusReview();
bool helpMenuContext();
namespace {
constexpr unsigned Receiver=1,RowLimit=512;
enum Command : unsigned {Begin=1,Row,Finish,Move,Close,ReadControls};
enum Flags : unsigned {Grid=1,ResetCursor=2,SkipEmpty=4,Invalid=8,AbilityPage=16,Present=32,HelpPage=64,BattlePage=128,HelpMenu=256,HelpOpening=512};
struct View {unsigned epoch=0,serial=0;int character=-1,page=-1;bool visible=false,grid=false,aeon=false,assembled=false,help=false;unsigned nativeArrows=0;bool battle=false;unsigned detailSerial=0;bool helpMenu=false;};
async::AtomicSnapshot<View> published;
View nativeView; // Serialized originating native owner only; worker reads published POD.
unsigned nextSerial=0,nextBatch=0;
std::atomic<unsigned> activeBatch{0};
struct StatusRow {std::wstring text,detail;unsigned ordinal=0;int x=0,y=0;bool positioned=false;unsigned role=HelpContent;bool mandatory=false;};
struct State {
    unsigned epoch=0,serial=0,batch=0,pendingSerial=0,received=0;
    bool pendingValid=false;
    int row=0;
    std::vector<StatusRow> rows,pending;
    std::wstring controls;
};
ProcessStorage<State> storage;
thread_local bool collecting=false;
thread_local StatusBatch drawn;
bool available() {
    return !escMenuActive() && nativeView.visible && nativeView.epoch==generation() &&
        (nativeView.battle?battleResultsReviewActive():nativeView.help?helpMenuContext():nativeView.grid?sphereGridOpen():scene==(nativeView.aeon?7:6));
}
bool current(unsigned epoch,unsigned serial) {
    View view;return published.tryRead(view) && view.visible && view.epoch==epoch &&
        view.serial==serial && epoch==generation();
}
void retireWorker() {
    auto& s=storage.get();
    if(s.epoch!=generation() || (!speechq::capturesPending() && !current(s.epoch,s.serial))) {
        std::vector<StatusRow>{}.swap(s.rows);std::wstring{}.swap(s.controls);s.row=0;s.serial=0;
    }
    // A begun batch may precede publication of its context. Finish owns adoption;
    // native page changes do not erase accepted automatic output mid-assembly.
    if(s.epoch!=generation()) {std::vector<StatusRow>{}.swap(s.pending);s.pendingValid=false;s.batch=0;}
    else if(!speechq::capturesPending() && activeBatch.load(std::memory_order_acquire)!=s.batch) {
        std::vector<StatusRow>{}.swap(s.pending);s.pendingValid=false;s.batch=0;
    }
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring text,std::wstring detail) {
    if(p.epoch!=generation()) return;
    auto& s=storage.get();
    if(p.command==Begin) {
        s.epoch=p.epoch;s.batch=p.batch;s.pendingSerial=p.serial;s.received=0;s.pendingValid=true;
        s.pending.clear();s.pending.reserve(RowLimit);return;
    }
    if(p.command==Row) {
        if(s.batch!=p.batch || s.pendingSerial!=p.serial || !s.pendingValid) return;
        if(p.ordinal!=s.received || s.received>=RowLimit) {s.pendingValid=false;return;}
        ++s.received;
        if(!(p.flags&SkipEmpty) || text.find_first_not_of(L" \r\n\t")!=std::wstring::npos)
            s.pending.push_back({std::move(text),std::move(detail),p.ordinal,static_cast<int>(p.context[2]),static_cast<int>(p.context[3]),p.context[4]!=0,static_cast<unsigned>(p.context[5]),p.context[6]!=0});
        return;
    }
    if(p.command==Finish) {
        if(s.batch!=p.batch || s.pendingSerial!=p.serial) return;
        if(!s.pendingValid || (p.flags&Invalid) || p.ordinal!=s.received) {
            s.pending.clear();s.pendingValid=false;return;
        }
        if(p.flags&HelpPage) {
            const auto token=currentSelectionDetailToken();
            View view;
            // An accepted arrow can replace this page's detail token before
            // its queued rows assemble. Keep that same-page transition while
            // rejecting a token owned by a popup, close or another page.
            if(!current(p.epoch,p.serial) || !published.tryRead(view) ||
               token.epoch!=view.epoch || token.serial!=view.detailSerial) {
                s.pending.clear();s.pendingValid=false;return;
            }
            std::stable_sort(s.pending.begin(),s.pending.end(),[](const StatusRow& a,const StatusRow& b) {
                if(a.positioned!=b.positioned)return a.positioned;
                return a.positioned && (a.y!=b.y?a.y<b.y:a.x<b.x);
            });
            std::vector<StatusRow> lines;lines.reserve(RowLimit);std::vector<std::wstring> controls;
            for(auto& row:s.pending) {
                if(row.role==HelpControls) {if(!row.text.empty()) controls.push_back(row.text);continue;}
                // Split only after ordering native text blocks: a multiline
                // block keeps its own original line order without guessed y offsets.
                size_t begin=0;
                for(size_t end=0;end<=row.text.size();++end) if(end==row.text.size() || row.text[end]==L'\n') {
                    auto line=row.text.substr(begin,end-begin);begin=end+1;
                    if(!line.empty() && line.back()==L'\r') line.pop_back();
                    if(line.find_first_not_of(L" \t")==std::wstring::npos) continue;
                    if(lines.size()==RowLimit) {s.pending.clear();s.pendingValid=false;return;}
                    if(row.role==HelpCharacter && line.find_first_not_of(L"? \t")==std::wstring::npos) line=L"Unknown character";
                    lines.push_back({std::move(line),{},row.ordinal,row.x,row.y,row.positioned,row.role});
                }
            }
            s.controls=join(controls,L"\n");s.pending.swap(lines);
        }
        const bool noAbilities=(p.flags&AbilityPage) && s.pending.size()==static_cast<size_t>(p.context[0]);
        if(noAbilities)
            s.pending.push_back({text,{}});
        size_t focus=0;
        bool focused=false;
        if(p.flags&HelpPage) for(size_t i=0;i<s.pending.size();++i)
            if(s.pending[i].ordinal==static_cast<unsigned>(p.context[1])) {focus=i;focused=true;break;}
        if(p.flags&(Grid|Present|BattlePage)) {
            if(s.pending.empty()) {
                if(p.flags&HelpPage) {s.rows.clear();s.row=0;s.serial=p.serial;emitSelectionDetailOwned({},p.detail,false,header);}
                s.pendingValid=false;return;
            }
            if((p.flags&Grid) && s.pending.size()==1) s.pending.front().text+=text;
            std::vector<std::wstring> lines;lines.reserve(s.pending.size());
            for(const auto& row:s.pending) if(!row.text.empty()) lines.push_back(row.text);
            const auto separator=(p.flags&HelpPage)?L"\n":L". ";
            const auto buffer=join(lines,separator);s.pending.front().detail=buffer;
            emitSelectionDetailOwned(buffer,p.detail,false,header);
            const auto headline=lines.empty()?std::wstring{}:lines[focus]+(noAbilities && !p.automatic?L". "+text:L"");
            if(p.flags&HelpMenu) {
                if(p.flags&HelpOpening) for(const auto& row:s.pending) if(row.role==HelpPrompt)
                    speechq::emitOwned(row.text,header,"help-menu-prompt",false,false);
                if(focused) speechq::emitOwned(headline,header,"help-menu-selection",false,false);
            } else if(p.flags&(Grid|Present)) speechq::emitOwned(headline,header,"status-page",false,false);
            if(!(p.flags&HelpMenu) && (p.flags&(Grid|Present)) && lines.size()>1) {
                std::vector<std::wstring> details;details.reserve(lines.size()-1);
                for(size_t i=0;i<s.pending.size();++i) if(i!=focus && !s.pending[i].text.empty() &&
                    (p.automatic || ((p.flags&BattlePage) && s.pending[i].mandatory))) details.push_back(s.pending[i].text);
                if(!details.empty()) speechq::emitOwned(join(details,separator),header,"status-page-details",false,false);
            }
        }
        // Adopt at the ordered event point even if Native has already opened
        // another page. Accepted earlier arrow events still own their output;
        // current native availability and detail tokens reject stale new reads.
        s.rows.swap(s.pending);s.serial=p.serial;
        if((p.flags&ResetCursor) || s.row>=static_cast<int>(s.rows.size()))
            s.row=static_cast<int>(focus);
        s.pending.clear();s.pendingValid=false;return;
    }
    if(p.command==ReadControls) {
        if(current(p.epoch,p.serial) && s.serial==p.serial)
            speechq::emitOwned(s.controls.empty()?L"No controls displayed.":s.controls,header,"help-controls",false,true);
        return;
    }
    if(p.command==Move) {
        if(s.serial!=p.serial || s.rows.empty()) return;
        const int direction=(p.flags&1)?1:-1;
        s.row=(s.row+direction+static_cast<int>(s.rows.size()))%static_cast<int>(s.rows.size());
        const auto& selected=s.rows[s.row];
        speechq::emitOwned(selected.text,header,"status-review",false,true);
        const bool help=(p.flags&HelpPage)!=0;
        emitSelectionDetailOwned(help?s.rows.front().detail:selected.detail,p.detail,!help && p.automatic,header);return;
    }
    if(p.command==Close) retireWorker();
}
void prepareCommand(TextPlan& p,unsigned command,unsigned serial,unsigned batch=0) {
    prepareTextPlan(p,"status-buffer",false);p.receiver=Receiver;p.command=command;p.serial=serial;p.batch=batch;
}
using Value=unsigned(__cdecl*)(unsigned);
unsigned value(unsigned getter,unsigned id) {return reinterpret_cast<Value>(address(getter))(id);}
TextRef numberLabel(TextPlan& p,const wchar_t* label,int64_t number) {
    return planConcat(p,planLiteral(p,label),planInteger(p,number));
}
void header(StatusBatch& batch) {
    const auto id=batch.character;
    TextPlan name;prepareTextPlan(name,"status-buffer",false);
    auto title=planPlayerName(name,id);
    if(batch.page>0) {
        constexpr const wchar_t* headings[]={L"Auto-Abilities",L"Skills",L"Special",L"White Magic",L"Black Magic"};
        const auto index=batch.aeon?batch.page:batch.page-1;
        if(index<0 || index>=5) {batch.valid=false;return;}
        title=planConcat(name,title,planConcat(name,planLiteral(name,L". "),planLiteral(name,headings[index])));
    }
    addStatusRow(batch,name,title);
    for(unsigned mp=0;mp<2;++mp) {
        TextPlan p;prepareTextPlan(p,"status-buffer",false);
        const auto currentValue=value(mp?0x4a9920:0x4a9870,id),maximum=value(mp?0x4a9960:0x4a9940,id);
        const auto text=planConcat(p,numberLabel(p,mp?L"MP: ":L"HP: ",currentValue),
            planConcat(p,planLiteral(p,L"/"),planInteger(p,maximum)));
        addStatusRow(batch,p,text);
    }
    if(batch.aeon) return; // Aeon pages display neither S.Lv nor Next AP.
    TextPlan levels;prepareTextPlan(levels,"status-buffer",false);
    const auto level=reinterpret_cast<uint8_t(__cdecl*)(unsigned)>(address(0x4a9b30))(id);
    addStatusRow(batch,levels,numberLabel(levels,L"Sphere Level: ",level));
    TextPlan ap;prepareTextPlan(ap,"status-buffer",false);
    const auto needed=value(0x384f50,id),earned=value(0x385370,id);
    addStatusRow(batch,ap,numberLabel(ap,L"Next AP: ",static_cast<int>(needed-earned)));
}
void standard(StatusBatch& batch) {
    const auto id=batch.character;
    using Saved=uintptr_t(__cdecl*)(unsigned);
    const auto saved=reinterpret_cast<Saved>(address(0x3853f0))(id);
    if(!saved) {batch.valid=false;return;}
    if(batch.aeon) {
        const unsigned special=read<uint16_t>(address(0x146a9e4)),overdrives=read<uint16_t>(address(0x146a9e8));
        if(special>16 || overdrives>16-special) {batch.valid=false;return;}
        for(unsigned i=0;i<special+overdrives;++i) {
            TextPlan p;prepareTextPlan(p,"aeon-status",false);
            const auto name=planRecordName(p,0x39a4c0,read<uint16_t>(address(0x146a9c4)+i*2));
            addStatusRow(batch,p,planPrefix(p,planLiteral(p,i<special?L"Special Attack":L"Overdrive"),name,L": "),0,true);
        }
    } else {
    const auto mode=read<uint8_t>(saved+0x38);
    TextPlan overdrive;prepareTextPlan(overdrive,"status-buffer",false);
    const auto title=planConcat(overdrive,planLiteral(overdrive,L"Overdrive mode: "),planMenuText(overdrive,1,mode+0x1033));
    addStatusRow(batch,overdrive,title);
    for(unsigned armor=0;armor<2;++armor) {
        TextPlan p;prepareTextPlan(p,"status-buffer",false);
        const auto text=planConcat(p,planConcat(p,planMenuText(p,11,armor?6:5),planLiteral(p,L": ")),planEquippedItemName(p,id,armor!=0));
        addStatusRow(batch,p,text);
    }
    }
    for(unsigned i=0;i<8;++i) {
        const auto entry=address(0x85a790)+i*8;
        const int label=read<int>(entry,-1),offset=read<int>(entry+4,-1);
        if(label<0 || offset<0 || offset>=0x94) break;
        TextPlan p;prepareTextPlan(p,"status-buffer",false);
        const auto text=planConcat(p,planConcat(p,planMenuText(p,11,label),planLiteral(p,L": ")),planInteger(p,read<uint8_t>(saved+offset)));
        addStatusRow(batch,p,text);
    }
}
void review(int direction) {
    if(!available()) return;
    if(nativeView.battle) refreshBattleResultReview();
    TextPlan p;prepareCommand(p,Move,nativeView.serial);p.flags=(direction>0?1:0)|(nativeView.help?HelpPage:0);p.detail=beginSelectionDetail();
    if(nativeView.help) {nativeView.detailSerial=p.detail.serial;published.publish(nativeView);}
    commitTextPlan(p);
}
void previous(bool) {review(-1);}
void next(bool) {review(1);}
bool reviewable() {return available() && !(nativeView.help && nativeView.helpMenu);}
bool verticalBuffer() {return reviewable() && !(nativeView.help && (nativeView.nativeArrows&1));}
bool horizontalBuffer() {return reviewable() && !(nativeView.help && (nativeView.nativeArrows&2));}
void controller(Event& e) {
    if(escMenuActive()) return;
    const int phase=read<int>(e.args[0]+0x1c,-1);
    if(scene!=6) {if(!nativeView.grid && !nativeView.aeon && !nativeView.help && !nativeView.battle) resetStatusReview();return;}
    if(phase<4 || phase>=0x11) {resetStatusReview();return;}
    using Character=uint8_t(__cdecl*)();
    const auto id=reinterpret_cast<Character>(address(0x4a9810))();
    const int page=phase==4?0:(phase>=6 && phase<=0xe && !(phase&1))?phase/2-2:-1;
    if(id>=18 || page<0) {nativeView.visible=false;published.publish(nativeView);return;}
    if(!available() || nativeView.character!=static_cast<int>(id) || nativeView.page!=page) {
        auto batch=beginStatusRows(id,page,false,true);header(batch);if(page==0) {standard(batch);batch.present=true;}
        finishStatusRows(batch);
    }
}
void drawBefore(Event& e) {
    e.before[0]=collecting;
    collecting=!nativeView.grid && available() && nativeView.page>0 &&
        read<int>(e.args[0]+0x64,-1)==(nativeView.aeon?1:0) &&
        read<int>(e.args[0]+0x60,-1)==nativeView.page-(nativeView.aeon?0:1);
    if(collecting) {
        drawn=beginStatusRows(nativeView.character,nativeView.page,false,false,false);
        drawn.aeon=nativeView.aeon;drawn.ability=true;drawn.present=!nativeView.assembled;
        header(drawn);drawn.fixedRows=drawn.count;
    }
}
void ability(Event& e) {
    if(!collecting || e.parentRva!=0x4d2760 || !e.args[0]) return;
    TextPlan p;prepareTextPlan(p,"status-buffer",false);
    const unsigned getter=e.rva==0x4f4f10?0x3909c0:0x39a4c0;
    const auto id=static_cast<unsigned>(e.args[0]);
    addStatusRow(drawn,p,planRecordName(p,getter,id),0,true);
}
void drawAfter(Event& e) {
    if(collecting && available() && finishStatusRows(drawn)) nativeView.assembled=true;
    collecting=e.before[0]!=0;
}
}
StatusBatch beginStatusRows(unsigned id,int page,bool grid,bool resetCursor,bool replaceContext) {
    if(replaceContext) resetSelectionDetail();
    StatusBatch batch;batch.character=id;batch.page=page;batch.grid=grid;batch.reset=resetCursor;batch.replaceContext=replaceContext;
    batch.serial=replaceContext?++nextSerial:nativeView.serial;batch.batch=++nextBatch;
    activeBatch.store(batch.batch,std::memory_order_release);
    TextPlan p;prepareCommand(p,Begin,batch.serial,batch.batch);batch.valid=commitTextPlan(p);return batch;
}
void addStatusRow(StatusBatch& batch,TextPlan& p,TextRef text,TextRef detail,bool skipEmpty) {
    if(batch.count>=(batch.help?RowLimit:65)) {batch.valid=false;return;}
    if(!batch.valid) {++batch.count;return;}
    p.receiver=Receiver;p.command=Row;p.serial=batch.serial;p.batch=batch.batch;p.ordinal=batch.count++;
    p.headline=text;p.description=detail;p.detail={};p.flags=skipEmpty?SkipEmpty:0;
    if(!commitTextPlan(p)) batch.valid=false;
}
bool finishStatusRows(StatusBatch& batch) {
    TextPlan p;prepareCommand(p,Finish,batch.serial,batch.batch);p.ordinal=batch.count;
    p.flags=(batch.grid?Grid:0)|(batch.reset?ResetCursor:0)|(batch.valid?0:Invalid)|
        (batch.ability?AbilityPage:0)|(batch.present?Present:0)|(batch.help?HelpPage:0)|(batch.battle?BattlePage:0)|
        (batch.helpMenu?HelpMenu:0)|(batch.helpOpening?HelpOpening:0);
    p.context[0]=batch.fixedRows;p.context[1]=batch.focusRow;
    if((batch.grid || batch.present || batch.battle) && batch.valid) p.detail=beginSelectionDetail();
    if((batch.grid || batch.ability) && batch.valid) {
        p.headline=planLiteral(p,batch.grid?(batch.page==1?L". No displayed attributes":L". No learned abilities in this category"):L"No learned abilities");
    }
    if(batch.valid && batch.replaceContext) {nativeView={generation(),batch.serial,static_cast<int>(batch.character),batch.page,true,batch.grid,batch.aeon,false,batch.help,batch.nativeArrows,batch.battle,p.detail.serial,batch.helpMenu};published.publish(nativeView);}
    const bool complete=commitTextPlan(p) && batch.valid;
    auto active=batch.batch;activeBatch.compare_exchange_strong(active,0,std::memory_order_acq_rel);
    if(!complete && batch.replaceContext) {nativeView.visible=false;published.publish(nativeView);}
    return complete;
}
void installStatusReview() {
    storage.prepare();registerTextPlanReceiver(Receiver,receive);
    registerTextPlanMaintenance(Receiver,retireWorker);
    installGridStatus();hook(0x4d1dd0,nullptr,controller);hook(0x4d2760,drawBefore,drawAfter);
    hook(0x4f4f10,ability,nullptr);hook(0x4f5000,ability,nullptr);
    hotkeyWhen(VK_UP,previous,verticalBuffer);hotkeyWhen(VK_DOWN,next,verticalBuffer);
    hotkeyWhen(VK_LEFT,previous,horizontalBuffer);hotkeyWhen(VK_RIGHT,next,horizontalBuffer);
    hotkeyWhen(VK_OEM_COMMA,previous,reviewable);hotkeyWhen(VK_OEM_PERIOD,next,reviewable);
}
void resetStatusReview() {
    activeBatch.store(0,std::memory_order_release);
    nativeView={generation(),++nextSerial,-1,-1,false,false};published.publish(nativeView);
    TextPlan p;prepareCommand(p,Close,nativeView.serial);commitTextPlan(p);
}
bool statusReviewAvailable() {return available();}
bool readHelpControls() {
    if(!nativeView.help || !available()) return false;
    TextPlan p;prepareCommand(p,ReadControls,nativeView.serial);commitTextPlan(p);return true;
}
void closeGridStatus(int page) {if(nativeView.grid && nativeView.page==page) resetStatusReview();}
void closeHelpReview() {if(nativeView.help) resetStatusReview();}
void closeBattleReview() {if(nativeView.battle) {resetStatusReview();resetSelectionDetail();}}
void showAeonStatus(unsigned character,int page) {
    if(escMenuActive()) return;
    if(scene!=7 || character<8 || character>=18 || page<0 || page>4) {
        if(nativeView.aeon) resetStatusReview();return;
    }
    if(available() && nativeView.aeon && nativeView.character==static_cast<int>(character) && nativeView.page==page) return;
    auto batch=beginStatusRows(character,page,false,true);batch.aeon=true;header(batch);
    if(!page) {standard(batch);batch.present=true;}
    finishStatusRows(batch);
}
}
