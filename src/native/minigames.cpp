#include "minigames.hpp"
#include "minigame_memory.hpp"
#include "minigame_spatial.hpp"
#include "camera_direction.hpp"
#include "events.hpp"
#include "nav_audio.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "butterfly.hpp"
#include "speech_domains.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>
#include <deque>
#include <cstring>
#include <stdexcept>
namespace ffx {
namespace {
struct Match {
    std::array<MinigameActor,12> players{};
    int carrier=-1,acting=-1,own=-1,opponent=-1;
    bool active=false,automatic=false;
};
struct Race { bool active=false; int balloons=-1,hits=-1; MinigameActor pickup; PickupSolution solution; };
// Native owns only current engine identity and ready fact capture.
MinigameScript current;
struct Frame {
    MinigameScript script;
    std::array<MinigameActor,12> players{};
    std::array<MinigameActor,20> pickups{};
    MinigameActor rider;
    std::array<float,8> values{};std::array<unsigned,4> available{};
    Point cameraForward;
    unsigned epoch=0,serial=0,vk=0,ticket=0,ticketSerial=0;
    bool events=false,allowed=false,cues=false,riderIsPlayer=false,cameraValid=false;
};
static_assert(std::is_trivially_copyable_v<Frame> && sizeof(Frame)%sizeof(wchar_t)==0);
struct Pending {speechq::Header anchor;Frame frame;};
using PendingFrames=std::deque<Pending>;
ProcessStorage<PendingFrames> pendingStorage;
constexpr unsigned FrameLimit=128;
std::array<std::atomic<unsigned>,FrameLimit> tickets{};
std::atomic<unsigned> latestSerial{0},rejectedFrames{0};
async::AtomicSnapshot<Frame> recovery;
std::atomic<unsigned> recoverySerial{0};
bool overloadReported=false; // Native owner only; once per pressure episode.
unsigned ticketHint=0,ticketSerial=0; // Native owner only.
void releaseTicket(const Frame& frame) {
    if(frame.ticket>=FrameLimit) return;
    auto expected=frame.ticketSerial;
    tickets[frame.ticket].compare_exchange_strong(expected,0,std::memory_order_release,std::memory_order_relaxed);
}
bool claimTicket(Frame& frame) {
    auto serial=++ticketSerial;if(!serial) serial=++ticketSerial;
    for(unsigned i=0;i<FrameLimit;++i) {
        const auto at=(ticketHint+i)%FrameLimit;unsigned empty=0;
        if(tickets[at].compare_exchange_strong(empty,serial,std::memory_order_acquire,std::memory_order_relaxed)) {
            frame.ticket=at;frame.ticketSerial=serial;ticketHint=at+1;return true;
        }
    }
    return false;
}
speechq::Header currentAnchor{};Frame currentFrame;
bool say(std::wstring text,bool interrupt,const char* producer) {
    return speechq::emitOwned(std::move(text),currentAnchor,producer,false,interrupt);
}
Match match; Race race;
unsigned epoch=0; // Sole speech-preparation owner.
unsigned ignoredEpoch=0;
int lastOwner=-1;
std::atomic<unsigned> inputKind{0},inputEpoch{0};
bool contextAllowed() { return !scene && !inBattle() && !modMenuActive() && !utilityConsoleActive() && gameFocused(); }
std::wstring decimal(double number) { wchar_t text[32]{}; swprintf_s(text,L"%.1f",number); return text; }
const MinigameActor* player(int index) { for(const auto& actor:match.players) if(actor.valid && actor.index==static_cast<unsigned>(index)) return &actor; return nullptr; }
std::wstring playerLabel(const MinigameActor& actor) {
    const bool own=blitzTeam(actor.local)==0;
    const auto slot=actor.local-(own?2:10);
    return std::wstring(own?L"Your team ":L"Opponent ")+(slot==5?L"goalkeeper":L"player "+std::to_wstring(slot+1));
}
std::wstring score() { return L"Score: your team "+std::to_wstring(match.own)+L", opponents "+std::to_wstring(match.opponent); }
void publishContext() {
    if(currentFrame.serial!=latestSerial.load(std::memory_order_acquire)) return;
    inputEpoch=epoch;
    inputKind=race.active?2u:match.active?(match.automatic?1u:3u):0u;
}
void updateMatch(const Frame& frame) {
    const bool events=frame.events;
    Match next;
    next.players=frame.players;
    next.active=next.players[0].valid && next.players[6].valid;
    next.automatic=frame.values[5]==0;
    const int base=static_cast<int>(frame.values[3]);
    const int slot=static_cast<int>(frame.values[2]);
    if(slot>=0 && slot<6) for(const auto& actor:next.players) if(actor.valid && actor.index==static_cast<unsigned>(base+slot)) next.carrier=base+slot;
    next.acting=static_cast<int>(frame.values[4]);
    next.own=static_cast<int>(frame.values[0]);
    next.opponent=static_cast<int>(frame.values[1]);
    if(events && next.active && match.active) {
        if(next.own>match.own || next.opponent>match.opponent) {
            say(next.own>match.own?L"Goal for your team":L"Goal for opponents",false,"blitz-goal");
            say(L"Score: your team "+std::to_wstring(next.own)+L", opponents "+std::to_wstring(next.opponent),false,"blitz-score");
        }
        const auto previous=player(lastOwner);
        if(next.carrier>=0 && previous) for(const auto& actor:next.players) if(actor.valid && actor.index==static_cast<unsigned>(next.carrier) && blitzTeam(previous->local)!=blitzTeam(actor.local)) {
            say(blitzTeam(actor.local)==0?L"Your team has the ball":L"Opponents have the ball",false,"blitz-possession"); break;
        }
    }
    // Keep event history separate from the query's actual loose-ball state.
    if(next.active && next.carrier>=0) lastOwner=next.carrier;
    match=next; race={}; publishContext();
}
void updateRace(const Frame& frame) {
    const bool events=frame.events;
    Race next;
    const int course=static_cast<int>(frame.values[1]);
    const int leg=static_cast<int>(frame.values[2]);
    const int finish=static_cast<int>(frame.values[3]);
    const auto& rider=frame.rider;
    next.active=course==4 && leg>=1 && leg<=4 && frame.values[7]==1 && !(finish&2) && rider.valid && rider.model==0x5033 && frame.riderIsPlayer;
    next.balloons=static_cast<int>(frame.values[5]);
    next.hits=static_cast<int>(frame.values[6]);
    if(events && race.active) {
        if(next.balloons>race.balloons) say(L"Balloon collected",false,"race-balloon");
        if(next.hits>race.hits) say(L"Bird hit",false,"race-bird");
    }
    if(next.active) {
        const auto angle=frame.values[0];
        for(unsigned i=0;i<std::size(racePickups);++i) {
            const auto& binding=racePickups[i];if(!(frame.available[binding.element]&binding.mask)) continue;
            const auto& target=frame.pickups[i];
            if(!target.valid || target.model!=0x5063) continue;
            const auto solution=pickupSolution(rider.position,target.position,angle,target.radius,target.height);
            if(!solution.valid || solution.ahead>450) continue;
            if(!next.solution.valid || solution.ahead<next.solution.ahead ||
               (solution.ahead==next.solution.ahead && std::abs(solution.lateral)<std::abs(next.solution.lateral))) {
                next.pickup=target; next.solution=solution;
            }
        }
    }
    race=next; match={}; publishContext();
    if(frame.serial==latestSerial.load(std::memory_order_acquire))
        publishPositionalCue(race.solution.pan,race.solution.proximity,race.active && race.solution.valid && frame.cues && frame.allowed,epoch);
}
void captureFrame(const MinigameScript& script,bool events,unsigned vk=0) {
    if(script.kind==MinigameKind::None) return;
    Frame frame;frame.script=script;frame.epoch=generation();frame.events=events;frame.vk=vk;
    frame.serial=latestSerial.fetch_add(1,std::memory_order_acq_rel)+1;
    frame.allowed=contextAllowed();frame.cues=setting(Setting::RaceCues)!=0;
    Point right;frame.cameraValid=cameraBasis(frame.cameraForward,right);
    const auto* bindings=script.kind==MinigameKind::Blitz?blitzBindings:raceBindings;
    const auto count=script.kind==MinigameKind::Blitz?std::size(blitzBindings):std::size(raceBindings);
    for(unsigned i=0;i<count;++i) if(!captureMinigameValue(script,bindings[i],0,frame.values[i])) {
        inputKind=0;publishPositionalCue(0,0,false,generation());recordLog("minigame ready fact capture unavailable");return;
    }
    if(script.kind==MinigameKind::Blitz) {
        for(unsigned i=0;i<12;++i) frame.players[i]=minigameActor(script,i<6?i+2:i+4);
    } else {
        frame.rider=minigameActor(script,0);frame.riderIsPlayer=frame.rider.actor==playerActor();
        for(unsigned i=0;i<4;++i) {float value;if(!captureMinigameValue(script,raceBindings[4],i,value)) return;frame.available[i]=static_cast<unsigned>(value);}
        for(unsigned i=0;i<std::size(racePickups);++i) frame.pickups[i]=minigameActor(script,racePickups[i].worker);
    }
    if(frame.epoch!=generation()) return;
    current=script;
    const bool claimed=claimTicket(frame);
    if(!claimed || !speechq::captureDomain(speechq::MinigameDomain,frame,frame.epoch)) {
        if(claimed) releaseTicket(frame);
        ++rejectedFrames;inputKind=0;publishPositionalCue(0,0,false,generation());
        // Complete latest ready facts recover input/audio after pressure drains.
        // Rejected semantic requests are explicitly reported, never replayed as
        // if accepted; all accepted FIFO events retain their original order.
        frame.events=false;frame.vk=0;recovery.publish(frame);recoverySerial=frame.serial;
        if(!overloadReported) {overloadReported=true;output(L"The minigame output queue is full. Try again.",true,"mod-minigame-overload");}
        recordLog("minigame snapshot rejected: bounded capture pressure");
    } else {recoverySerial=0;overloadReported=false;}
}
void update(const MinigameScript& script,bool events) {captureFrame(script,events);}
void variableBefore(Event& event) {
    // Ordinary field scripts incur no reader work on this high-frequency
    // setter after their first identity check in a resource generation.
    if(ignoredEpoch==generation()) return;
    if(current.span.epoch==generation() && current.kind!=MinigameKind::None && !minigameBinding(current,static_cast<unsigned>(event.args[1]))) return;
    const auto script=captureMinigameScript(event.args[0]);
    if(script.span.serial && script.kind==MinigameKind::None && read<uint16_t>(script.worker+0x2e,0xffff)==0) { ignoredEpoch=generation(); return; }
    const auto binding=minigameBinding(script,static_cast<unsigned>(event.args[1]));
    if(binding) {
        float value;if(captureMinigameValue(script,*binding,static_cast<unsigned>(event.args[2]),value)) {event.values[0]=value;event.before[0]=1;}
    }
}
void variableAfter(Event& event) {
    if(!event.before[0]) return;
    const auto script=captureMinigameScript(event.args[0]); const auto binding=minigameBinding(script,static_cast<unsigned>(event.args[1]));
    if(!binding) return;
    float value;if(captureMinigameValue(script,*binding,static_cast<unsigned>(event.args[2]),value) &&
        (inputEpoch!=generation() || value!=event.values[0])) update(script,true);
}
void actorAfter(Event& event) {
    // Position writes refresh race audio only for the controlled rider.
    // Native collision commits provide the normal at-speed producer.
    if(event.rva==0x42b500 && (current.kind!=MinigameKind::Race || event.args[0]!=playerActor())) return;
    if(ignoredEpoch==generation() || (event.rva==0x426770 && current.kind==MinigameKind::None)) return;
    const auto script=captureMinigameScript(current.worker);
    if(script.kind!=MinigameKind::None) update(script,false);
}
void movementAfter(Event&) {
    if(current.kind==MinigameKind::Race && current.span.epoch==generation()) update(captureMinigameScript(current.worker),false);
}
std::wstring position(const MinigameActor& actor,Point origin) {
    return playerLabel(actor)+L": "+decimal(distanceXZ(origin,actor.position)/11.13)+L" steps "+cameraDirectionLabel({actor.position.x-origin.x,0,actor.position.z-origin.z},currentFrame.cameraForward,currentFrame.cameraValid);
}
void blitzKey(unsigned vk) {
    const auto carrier=player(match.carrier),acting=player(match.acting);
    const auto origin=acting?acting->position:carrier?carrier->position:match.players[0].position;
    if(vk==VK_F1) {
        say(carrier?L"Ball carrier. "+position(*carrier,origin):L"The ball is loose or in flight",true,"blitz-query");
        if(acting) say(L"Acting player: "+playerLabel(*acting),false,"blitz-query");
    } else if(vk==VK_F5) say(score(),true,"blitz-query");
    else {
        say(vk==VK_F2?L"Your team positions":vk==VK_F3?L"Opponent positions":L"Goalkeepers and goal directions",true,"blitz-query");
        for(const auto& actor:match.players) if(actor.valid &&
           (vk==VK_F4?actor.local==7 || actor.local==15:blitzTeam(actor.local)==(vk==VK_F2?0:1))) say(position(actor,origin),false,"blitz-query");
    }
}
void raceKey(unsigned vk) {
    if(vk==VK_F6) {
        say(std::wstring(L"Race pickup cue: ")+(currentFrame.cues?L"on":L"off"),true,"race-query");
    } else if(vk==VK_F5) say(L"Balloons: "+std::to_wstring(race.balloons)+L". Bird hits: "+std::to_wstring(race.hits),true,"race-query");
    else if(!race.solution.valid) say(L"No available balloon ahead",true,"race-query");
    else say(L"Balloon: "+decimal(std::max(0.0,race.solution.ahead)/11.13)+L" steps ahead. "+
        (race.solution.aligned?L"Centered":decimal(std::abs(race.solution.lateral)/11.13)+L" steps "+(race.solution.lateral<0?L"left":L"right")),true,"race-query");
}
void process(const speechq::Header& anchor,const Frame& frame) {
    if(frame.epoch!=generation()) return;
    if(epoch!=frame.epoch) {match={};race={};lastOwner=-1;epoch=frame.epoch;}
    currentAnchor=anchor;currentFrame=frame;
    if(frame.script.kind==MinigameKind::Blitz) updateMatch(frame);else updateRace(frame);
    if(frame.vk && frame.allowed) {
        if(frame.script.kind==MinigameKind::Blitz && match.active && match.automatic) blitzKey(frame.vk);
        else if(frame.script.kind==MinigameKind::Race && race.active) raceKey(frame.vk);
    }
}
void maintain() {
    auto& pending=pendingStorage.get();
    while(!pending.empty()) {
        const auto& frame=pending.front().frame;
        const auto status=frame.epoch==generation()?qualifyMinigameScript(frame.script):AtelPreparation::Retired;
        if(status==AtelPreparation::Pending) break;
        auto item=std::move(pending.front());pending.pop_front();releaseTicket(item.frame);
        if(status==AtelPreparation::Ready) process(item.anchor,item.frame);
        else if(item.frame.serial==latestSerial.load(std::memory_order_acquire)) {inputKind=0;publishPositionalCue(0,0,false,generation());}
    }
    Frame fresh;auto serial=recoverySerial.load(std::memory_order_acquire);
    if(serial && pending.empty() && recovery.tryRead(fresh) && fresh.serial==serial) {
        const auto status=fresh.epoch==generation()?qualifyMinigameScript(fresh.script):AtelPreparation::Retired;
        if(status!=AtelPreparation::Pending && recoverySerial.compare_exchange_strong(serial,0,std::memory_order_acq_rel) &&
            status==AtelPreparation::Ready) process({},fresh);
    }
}
void receive(const speechq::Header& anchor,const std::wstring& packet) {
    if(packet.size()*sizeof(wchar_t)!=sizeof(Frame)) return;
    Frame frame;std::memcpy(&frame,packet.data(),sizeof(frame));
    try {pendingStorage.get().push_back({anchor,frame});}
    catch(...) {releaseTicket(frame);throw;}
    maintain();
}
void resetWorker(unsigned nextEpoch) {
    auto& pending=pendingStorage.get();
    for(auto i=pending.begin();i!=pending.end();) if(i->frame.epoch!=nextEpoch) {
        releaseTicket(i->frame);i=pending.erase(i);
    } else ++i;
    match={};race={};lastOwner=-1;epoch=nextEpoch;
}

}
void resetMinigames() {
    inputKind=0;inputEpoch=0;current={};ignoredEpoch=0;latestSerial.fetch_add(1,std::memory_order_acq_rel);
    recoverySerial=0;overloadReported=false;
    // A late old-generation release cannot free a new serial's ticket.
    // Central dispatch may discard old records before calling this domain.
    for(auto& ticket:tickets) ticket.store(0,std::memory_order_release);
    publishPositionalCue(0,0,false,generation());
}
bool minigameCapturesKey(unsigned vk) {
    if(butterflyActive() && contextAllowed()) return vk==VK_F1 || vk==VK_F5 || vk==VK_F6 || vk=='G';
    if(inputEpoch!=generation() || !contextAllowed()) return false;
    const auto kind=inputKind.load();
    return kind==1? vk>=VK_F1 && vk<=VK_F5:kind==2 && (vk==VK_F1 || vk==VK_F5 || vk==VK_F6);
}
bool minigameActive() { return butterflyActive() || (inputEpoch==generation() && inputKind!=0); }
bool dispatchMinigameKey(unsigned vk,bool) {
    if(!minigameCapturesKey(vk)) return false;
    if(butterflyActive()) {
        if(vk==VK_F6) {
            adjustSetting(Setting::ButterflyCues,1); refreshButterflyAudio();
            output(L"Butterfly cues: "+settingValue(Setting::ButterflyCues),true,"butterfly-query");
        } else if(vk==VK_F5 || vk=='G') butterflyStatus();
        else butterflyDirections();
        return true;
    }
    if(vk==VK_F6) adjustSetting(Setting::RaceCues,1);
    captureFrame(captureMinigameScript(current.worker),false,vk);
    return true;
}
void refreshMinigameAudio() {
    if(butterflyActive()) refreshButterflyAudio();
    else if(current.kind==MinigameKind::Race && current.span.epoch==generation()) update(captureMinigameScript(current.worker),false);
}
void installMinigames() {
    pendingStorage.prepare();
    speechq::registerDomain(speechq::MinigameDomain,receive,resetWorker);
    speechq::registerDomainMaintenance(speechq::MinigameDomain,maintain);
    if(!onAtelPrepared(speechq::notifyPreparation)) throw std::runtime_error("Minigame preparation notification unavailable");
    hook(0x4713a0,variableBefore,variableAfter);
    hook(0x43d3b0,nullptr,movementAfter);
    hook(0x425010,nullptr,actorAfter); hook(0x42b500,nullptr,actorAfter); hook(0x426770,nullptr,actorAfter);
}
}
