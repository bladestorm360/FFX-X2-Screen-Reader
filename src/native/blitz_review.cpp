// Context-owned Blitzball review assembled from copied minigame facts. Review
// selection must not become a second dialogue producer or retain an old match.
// Flow, controls and evidence: Docs/features/menus/index.md#dialogue-and-review.
#include "blitz_review.hpp"
#include "utility_memory.hpp"
#include "minigame_memory.hpp"
#include "events.hpp"
#include "process_storage.hpp"
#include "text_plan.hpp"
#include "speech_queue.hpp"
#include <atomic>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>
#include <vector>
#include <stdexcept>

namespace ffx {
namespace {
constexpr unsigned Receiver=5,PageLineLimit=0x4000;
enum Command : unsigned {Review=1,Position};
std::atomic<bool> active{false},positionFlag{false};
std::atomic<unsigned> activeEpoch{0},revision{1},positionRevision{0};
std::array<std::atomic<unsigned>,64> pendingTickets{};
unsigned ticketSerial=0,ticketHint=0,nativeSerial=0;
bool overloaded=false;
uintptr_t nativeChunk=0;
unsigned nativeEpoch=0;
bool nativeLobby=false;
ProcessStorage<UtilityScript> lobbyStorage;
struct State {
    unsigned epoch=0,revision=0;
    bool valid=false;
    unsigned row=0;
    std::array<std::vector<std::wstring>,8> pages;
    std::vector<std::wstring> rows;
};
ProcessStorage<State> storage;

struct Stat {const wchar_t* label;unsigned low;};
constexpr Stat stats[]={{L"HP",0x5d000040},{L"Speed",0x5d000300},
    {L"Endurance",0x5d000180},{L"Attack",0x5d000100},
    {L"Pass",0x5d0001c0},{L"Block",0x5d000240},
    {L"Shot",0x5d000200},{L"Catch",0x5d000280}};
bool identify(UtilityScript& script,bool& lobby,AtelSpan& span) {
    lobby=false;
    try {
        if(captureUtilityScript(script) && script.name=="bltz0200" && script.length==113266 && script.workers==7) {
            const auto state=atelPreparation(script.chunk);
            if(state!=AtelPreparation::Pending && state!=AtelPreparation::Ready) return false;
            if(!nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial) return false;
            if(state==AtelPreparation::Ready) {
                AtelIdentity identity;
                if(!readyAtelIdentity(script.chunk,identity) || identity.span.serial!=script.allocationSerial ||
                    identity.length!=script.length || identity.codeOffset!=script.codeOffset || identity.workers!=script.workers ||
                    script.name!=identity.name || identity.checksum!=0x58a5c0a5 || !captureUtilityWorker(script,1,script.worker)) return false;
                script.header=read<uintptr_t>(script.worker);
            }
            lobby=true;
            return true;
        }
    } catch(...) {}
    script.chunk=0;script.worker=0;script.header=0;lobby=false;
    const auto match=captureMinigameScript();span=match.span;
    return match.kind==MinigameKind::Blitz;
}
void retireWorker() {
    auto& s=storage.get();
    if(s.epoch!=generation() || (!speechq::capturesPending() &&
        (!active.load(std::memory_order_acquire) || activeEpoch.load(std::memory_order_acquire)!=s.epoch || revision.load(std::memory_order_acquire)!=s.revision))) {
        for(auto& page:s.pages) std::vector<std::wstring>{}.swap(page);
        std::vector<std::wstring>{}.swap(s.rows);s.row=0;s.valid=false;s.revision=0;
    }
}
void appendStats(std::vector<std::wstring>& result,const BlitzSnapshot& snapshot) {
    if(!snapshot.statsValid) return;
    if(snapshot.statsCount) result.push_back(L"Level: "+std::to_wstring(snapshot.stats[0]));
    for(unsigned i=0;i+1<snapshot.statsCount && i<std::size(stats);++i)
        result.push_back(std::wstring(stats[i].label)+L": "+std::to_wstring(snapshot.stats[i+1]));
}
void assemble(State& s,const BlitzSnapshot& snapshot) {
    std::vector<std::wstring> result;
    result.reserve(s.pages[3].size()+s.pages[4].size()+s.pages[5].size()+s.pages[6].size()+s.pages[0].size()+9);
    if(snapshot.lobby && !s.pages[2].empty()) {
        if(snapshot.statsValid) {result.push_back(s.pages[2].front());appendStats(result,snapshot);}
    }
    for(unsigned slot:{3u,4u,5u,6u,0u})
        result.insert(result.end(),s.pages[slot].begin(),s.pages[slot].end());
    s.rows=std::move(result);if(s.row>=s.rows.size()) s.row=0;
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring,std::wstring) {
    static_assert(sizeof(BlitzSnapshot)<=sizeof(p.context));
    BlitzSnapshot snapshot;std::memcpy(&snapshot,p.context.data(),sizeof(snapshot));
    if(p.command==Review) {
        if(!snapshot.valid || snapshot.epoch!=p.epoch || p.epoch!=generation()) return;
        auto& s=storage.get();
        if(s.epoch!=snapshot.epoch || s.revision!=snapshot.revision) {
            for(auto& page:s.pages) page.clear();s.rows.clear();s.row=0;
            s.epoch=snapshot.epoch;s.revision=snapshot.revision;s.valid=true;
        }
        assemble(s,snapshot);
        if(s.rows.empty()) {
            speechq::emitOwned(L"No Blitzball details are displayed.",header,"blitz-review",false,true);return;
        }
        if(p.flags==VK_UP && s.row) --s.row;
        if(p.flags==VK_DOWN && s.row+1<s.rows.size()) ++s.row;
        if(p.flags==VK_LEFT) s.row=0;
        if(p.flags==VK_RIGHT) {
            std::wstring joined;
            for(const auto& line:s.rows) {if(!joined.empty()) joined+=L". ";joined+=line;}
            speechq::emitOwned(std::move(joined),header,"blitz-review",false,true);
        } else speechq::emitOwned(s.rows[s.row],header,"blitz-review",false,true);
        return;
    }
    if(p.command==Position) {
        static const wchar_t* names[]={L"left forward",L"right forward",L"midfielder",L"left defense",L"right defense",L"goalkeeper"};
        std::wstring text;
        if(!snapshot.valid || snapshot.epoch!=p.epoch || !snapshot.lobby) return;
        if(snapshot.position==-2) text=L"The position assignment is unavailable.";
        else if(snapshot.position<0 || snapshot.position>=6) text=L"All positions assigned";
        else text=L"Assigning "+std::wstring(names[snapshot.position]);
        speechq::emitOwned(std::move(text),header,"blitz-position",false,true);
    }
}
bool identifyAndPublish(UtilityScript& script,BlitzSnapshot& snapshot) {
    bool lobby=false;const bool found=identify(script,lobby,snapshot.span);
    const unsigned currentEpoch=generation();
    if(nativeEpoch!=currentEpoch || nativeLobby!=lobby || nativeChunk!=snapshot.span.chunk || nativeSerial!=snapshot.span.serial) {
        if(nativeEpoch!=currentEpoch) {for(auto& ticket:pendingTickets) ticket=0;overloaded=false;}
        nativeEpoch=currentEpoch;nativeLobby=lobby;
        unsigned next=revision.fetch_add(1,std::memory_order_acq_rel)+1;
        if(!next) {revision.store(1,std::memory_order_release);next=1;}
        nativeChunk=snapshot.span.chunk;nativeSerial=snapshot.span.serial;
        activeEpoch.store(currentEpoch,std::memory_order_release);
        positionFlag.store(false,std::memory_order_release);positionRevision.store(0,std::memory_order_release);
    }
    snapshot.epoch=currentEpoch;snapshot.revision=revision.load(std::memory_order_acquire);
    snapshot.valid=found;snapshot.lobby=lobby;
    if(found && blitzSnapshotPreparation(snapshot)==AtelPreparation::Ready) {activeEpoch.store(currentEpoch,std::memory_order_release);active.store(true,std::memory_order_release);}
    else {active.store(false,std::memory_order_release);positionFlag.store(false,std::memory_order_release);}
    return found;
}
}

AtelPreparation blitzSnapshotPreparation(const BlitzSnapshot& snapshot) {
    if(!snapshot.valid || snapshot.epoch!=generation()) return AtelPreparation::Retired;
    const auto state=atelPreparation(snapshot.span.chunk);if(state!=AtelPreparation::Ready) return state;
    AtelIdentity identity;if(!readyAtelIdentity(snapshot.span.chunk,identity)) return AtelPreparation::Pending;
    if(identity.span.serial!=snapshot.span.serial) return AtelPreparation::Retired;
    if(snapshot.lobby) return identity.length==113266 && identity.workers==7 && identity.checksum==0x58a5c0a5 &&
        !std::strcmp(identity.name,"bltz0200")?AtelPreparation::Ready:AtelPreparation::Rejected;
    for(const auto& record:minigameCatalog) if(!std::strcmp(record.name,"bltz0000") && identity.length==record.length &&
        identity.workers==record.workers && identity.checksum==record.checksum && !std::strcmp(identity.name,record.name)) return AtelPreparation::Ready;
    return AtelPreparation::Rejected;
}
void releaseBlitzSnapshot(const BlitzSnapshot& snapshot) {
    if(snapshot.ticket>=pendingTickets.size()) return;
    auto serial=snapshot.ticketSerial;pendingTickets[snapshot.ticket].compare_exchange_strong(serial,0,std::memory_order_release,std::memory_order_relaxed);
}

BlitzSnapshot captureBlitzSnapshot(bool statistics,bool wantPosition) {
    BlitzSnapshot snapshot;
    if(!lobbyStorage.ready()) return snapshot;
    auto& script=lobbyStorage.get();
    if(!identifyAndPublish(script,snapshot)) return snapshot;
    if(blitzSnapshotPreparation(snapshot)==AtelPreparation::Pending) {
        const auto serial=++ticketSerial?ticketSerial:++ticketSerial;
        for(unsigned i=0;i<pendingTickets.size();++i) {
            const auto at=(ticketHint+i)%pendingTickets.size();unsigned empty=0;
            if(pendingTickets[at].compare_exchange_strong(empty,serial,std::memory_order_acquire,std::memory_order_relaxed)) {
                snapshot.ticket=static_cast<unsigned>(at);snapshot.ticketSerial=serial;ticketHint=static_cast<unsigned>(at+1);overloaded=false;return snapshot;
            }
        }
        if(!overloaded) {overloaded=true;output(L"The Blitzball preparation queue is full.",false,"blitz-overload");}
        snapshot.valid=snapshot.lobby=false;return snapshot;
    }
    if(!snapshot.lobby) return snapshot;
    if(statistics) {
        int selected=-1,player=-1;
        if(captureUtilityValue(script,0x56000000,1,0,selected) && selected>=0 && selected<16 &&
            captureUtilityValue(script,0x5d0003f4,16,static_cast<unsigned>(selected),player) && player>=0 && player<60) {
            snapshot.statsValid=true;
            if(captureUtilityValue(script,0x010013ce,60,static_cast<unsigned>(player),snapshot.stats[0])) {
                snapshot.statsCount=1;
                for(unsigned i=0;i<std::size(stats);++i) {
                    if(!captureUtilityValue(script,stats[i].low,16,static_cast<unsigned>(selected),snapshot.stats[i+1])) {
                        recordLog("blitz review: displayed stats unavailable");break;
                    }
                    ++snapshot.statsCount;
                }
            } else recordLog("blitz review: displayed stats unavailable");
        } else recordLog("blitz review: displayed stats unavailable");
    }
    if(wantPosition) {
        snapshot.position=-1;
        for(int i=0;i<6;++i) {
            int value=0;
            if(!captureUtilityValue(script,0x5d000340,6,static_cast<unsigned>(i),value)) {snapshot.position=-2;break;}
            if(value==100) {snapshot.position=i;break;}
        }
    }
    return snapshot;
}

bool observeBlitzMessageOwned(unsigned slot,const MessagePage& page,const BlitzSnapshot& snapshot) {
    if(slot>=8 || blitzSnapshotPreparation(snapshot)!=AtelPreparation::Ready) return false;
    if(snapshot.revision==revision.load(std::memory_order_acquire)) {activeEpoch=snapshot.epoch;active=true;}
    auto& s=storage.get();
    if(s.epoch!=snapshot.epoch || s.revision!=snapshot.revision) {
        for(auto& old:s.pages) std::vector<std::wstring>{}.swap(old);
        s.rows.clear();s.row=0;s.epoch=snapshot.epoch;s.revision=snapshot.revision;s.valid=true;
    }
    if(slot==6 && snapshot.lobby) {
        const bool flag=!page.lines.empty() && page.lines.front()==L"Assign Positions";
        positionRevision.store(flag?snapshot.revision:0,std::memory_order_release);
        positionFlag.store(flag,std::memory_order_release);
    }
    if(slot==1 && page.selector) {s.pages[0].clear();s.pages[2].clear();s.pages[3].clear();s.pages[4].clear();s.pages[5].clear();s.row=0;}
    const auto count=(std::min)(page.lines.size(),static_cast<size_t>(PageLineLimit));
    s.pages[slot].assign(page.lines.begin(),page.lines.begin()+count);
    return snapshot.lobby && slot==2 && !page.selector;
}

void installBlitzReview() {
    lobbyStorage.prepare();storage.prepare();
    registerTextPlanReceiver(Receiver,receive);registerTextPlanMaintenance(Receiver,retireWorker);
    if(!onAtelPrepared(speechq::notifyPreparation)) throw std::runtime_error("Blitz preparation notification unavailable");
}
bool blitzReviewAvailable() {return active.load(std::memory_order_acquire) && activeEpoch.load(std::memory_order_acquire)==generation() && !scene;}
bool blitzPositionAvailable() {
    const auto window=read<uintptr_t>(address(0x14676b0)+4),cursor=read<uintptr_t>(address(0x1468a50)+4);
    const auto rev=revision.load(std::memory_order_acquire);
    return blitzReviewAvailable() && positionFlag.load(std::memory_order_acquire) &&
        positionRevision.load(std::memory_order_acquire)==rev && window && cursor &&
        read<uint16_t>(window+0x2c)==2 && read<uint8_t>(cursor+1)==1;
}
bool blitzCapturesKey(unsigned vk,bool control) {
    return (control && blitzReviewAvailable() && (vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT)) ||
        (!control && vk==VK_OEM_5 && blitzPositionAvailable());
}
bool dispatchBlitzKey(unsigned vk,bool control) {
    if(!blitzCapturesKey(vk,control)) return false;
    const bool position=vk==VK_OEM_5;
    const auto snapshot=captureBlitzSnapshot(!position,position);
    if(blitzSnapshotPreparation(snapshot)!=AtelPreparation::Ready) {releaseBlitzSnapshot(snapshot);return false;}
    TextPlan p;prepareTextPlan(p,position?"blitz-position":"blitz-review",false);
    p.receiver=Receiver;p.command=position?Position:Review;p.flags=vk;
    std::memcpy(p.context.data(),&snapshot,sizeof(snapshot));
    const bool accepted=commitTextPlan(p);releaseBlitzSnapshot(snapshot);return accepted;
}
void resetBlitzReview() {
    revision.fetch_add(1,std::memory_order_acq_rel);active.store(false,std::memory_order_release);
    positionFlag.store(false,std::memory_order_release);positionRevision.store(0,std::memory_order_release);
    activeEpoch.store(generation(),std::memory_order_release);nativeEpoch=generation();nativeLobby=false;nativeChunk=0;nativeSerial=0;
    overloaded=false;for(auto& ticket:pendingTickets) ticket=0;
}
}
