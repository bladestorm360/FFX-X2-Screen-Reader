#include "jecht_voice.hpp"
#include "events.hpp"
#include "jecht_caption.hpp"
#include "speech_domains.hpp"
#include <atomic>
#include <array>
namespace ffx {
#ifdef FFX_READER_TEST
unsigned jechtTickForTest();
#endif
namespace {
// Exact original Winno playSfx IDs, shared by both script variants.
constexpr unsigned ids[]={85040,85037,85041,85038,85050,85039,85052,85051,85049};
constexpr unsigned EventLimit=16;
static_assert(std::atomic<unsigned>::is_always_lock_free && std::atomic<uintptr_t>::is_always_lock_free);
struct SoundEvent {
    std::atomic<uintptr_t> group{0},event{0},user{0};
    // Immutable identity publication for one instance; low bit is playing.
    std::atomic<unsigned> state{0};
};
struct Sequence {
    std::atomic<unsigned> counter{0},serial{0},epoch{0},ready{0},succeeded{0},voiced{0};
    std::array<SoundEvent,EventLimit> events{};
};
std::array<Sequence,9> sequences{};
thread_local unsigned current=9,currentEpoch=0;
unsigned index(unsigned id) {for(unsigned i=0;i<9;++i) if(ids[i]==id) return i;return 9;}
void rootBefore(Event& e) {
    e.before[0]=current;e.before[1]=currentEpoch;
    current=index(static_cast<unsigned>(e.args[0]));currentEpoch=e.epoch;
    e.before[2]=current<9?sequences[current].serial.load(std::memory_order_acquire):0;
}
void rootAfter(Event& e) {
    if(current<9 && e.epoch==generation()) {
        auto& s=sequences[current];
        // A disabled/missing sequence or refused array never reached Events.
        // Publish a failed attempt so its caption falls back without waiting.
        if(s.serial.load(std::memory_order_acquire)==static_cast<unsigned>(e.before[2])) {
            const auto serial=s.counter.fetch_add(1,std::memory_order_relaxed)+1;
            if(serial && serial<=0x7fffffff) {
                s.ready.store(0,std::memory_order_release);s.epoch.store(e.epoch,std::memory_order_relaxed);
                s.voiced.store(0,std::memory_order_relaxed);s.serial.store(serial,std::memory_order_release);
                s.ready.store(serial,std::memory_order_release);
            }
        }
        speechq::notifyPreparation();
    }
    current=static_cast<unsigned>(e.before[0]);currentEpoch=static_cast<unsigned>(e.before[1]);
}
void eventsBefore(Event& e) {
    e.before[0]=0;e.before[1]=9;
    if(current>=9 || currentEpoch!=e.epoch || e.epoch!=generation()) return;
    // Read only the ready SfxEvent array at its original owner-side start.
    // The ctor establishes count/capacity and immutable 8-byte event records.
    const auto count=read<unsigned>(e.ecx),capacity=read<unsigned>(e.ecx+4);
    const auto array=read<uintptr_t>(e.ecx+8);
    if(!count || count>EventLimit || capacity<count || !array) return;
    std::array<uintptr_t,EventLimit> events{};
    for(unsigned i=0;i<count;++i) if(!copy(array+i*8,&events[i],sizeof(uintptr_t)) || !events[i]) return;
    auto& s=sequences[current];
    const auto serial=s.counter.fetch_add(1,std::memory_order_relaxed)+1;
    // Avoid an ambiguous publication after the 31-bit instance counter wraps.
    if(!serial || serial>0x7fffffff) return;
    s.ready.store(0,std::memory_order_release);
    s.succeeded.store(0,std::memory_order_relaxed);s.voiced.store(0,std::memory_order_relaxed);
    s.epoch.store(e.epoch,std::memory_order_relaxed);
    s.serial.store(serial,std::memory_order_release);
    for(unsigned i=0;i<EventLimit;++i) {
        auto& item=s.events[i];item.state.store(0,std::memory_order_release);
        item.group.store(i<count?e.ecx:0,std::memory_order_relaxed);
        item.event.store(i<count?events[i]:0,std::memory_order_relaxed);
        item.user.store(i<count?array+i*8+4:0,std::memory_order_relaxed);
        item.state.store(i<count?(serial<<1)|1u:0,std::memory_order_release);
    }
    e.before[0]=serial;e.before[1]=current;e.before[2]=static_cast<int>(e.args[0])>0;
}
void eventsAfter(Event& e) {
    const auto serial=static_cast<unsigned>(e.before[0]),which=static_cast<unsigned>(e.before[1]);
    if(!serial || which>=9) return;
    auto& s=sequences[which];
    if(s.epoch.load(std::memory_order_acquire)==e.epoch && s.serial.load(std::memory_order_acquire)==serial) {
        s.voiced.store(e.before[2] && s.succeeded.load(std::memory_order_acquire)==serial?serial:0,std::memory_order_relaxed);
        s.ready.store(serial,std::memory_order_release);
        speechq::notifyPreparation();
    }
}
template<class Match> void retire(Match match,unsigned epoch) {
    for(auto& s:sequences) {
        if(s.epoch.load(std::memory_order_acquire)!=epoch) continue;
        for(auto& item:s.events) {
            auto state=item.state.load(std::memory_order_acquire);
            if(!(state&1u) || !match(item)) continue;
            // A late completion cannot retire a replacement with another serial.
            item.state.compare_exchange_strong(state,state&~1u,std::memory_order_acq_rel);
        }
    }
}
void started(Event& e) {
    if(e.parentRva!=jecht_voice_rva::Events) return;
    if(e.result==0) {
        if(current<9 && currentEpoch==e.epoch) {
            auto& s=sequences[current];s.succeeded.store(s.serial.load(std::memory_order_acquire),std::memory_order_release);
        }
        return;
    }
    retire([&](const SoundEvent& item){return item.event.load(std::memory_order_relaxed)==e.args[0];},e.epoch);
}
void completed(Event& e) {
    if(e.args[1]!=2 && e.args[1]!=4) return;
    // Opaque IDs only: never dereference an FMOD event/user pointer here or on
    // the game owner. Native completion retains its own original user-data write.
    retire([&](const SoundEvent& item){return item.event.load(std::memory_order_relaxed)==e.args[0] &&
        item.user.load(std::memory_order_relaxed)==e.args[4];},e.epoch);
}
void disposed(Event& e) {
    retire([&](const SoundEvent& item){return item.group.load(std::memory_order_relaxed)==e.ecx;},e.epoch);
}
}
unsigned jechtVoiceSerial(unsigned id) noexcept {
    const auto which=index(id);return which<9?sequences[which].serial.load(std::memory_order_acquire):0;
}
JechtVoiceState jechtVoiceState(unsigned id,unsigned epoch) noexcept {
    JechtVoiceState result;const auto which=index(id);if(which==9) return result;
    const auto& s=sequences[which];result.serial=s.serial.load(std::memory_order_acquire);
    if(!result.serial || s.epoch.load(std::memory_order_acquire)!=epoch || s.ready.load(std::memory_order_acquire)!=result.serial) return result;
    for(const auto& item:s.events) {
        const auto state=item.state.load(std::memory_order_acquire);
        if((state>>1)==result.serial && (state&1u)) result.playing=true;
    }
    result.ready=s.serial.load(std::memory_order_acquire)==result.serial && s.epoch.load(std::memory_order_acquire)==epoch;
    result.voiced=result.ready && s.voiced.load(std::memory_order_acquire)==result.serial;
    return result;
}
JechtCaptionState jechtCaptionState(const JechtCaption& caption) noexcept {
    if(!caption.epoch || caption.epoch!=generation()) return {};
    const auto voice=jechtVoiceState(caption.sequence,caption.epoch);
    if(voice.ready && voice.serial==caption.baseline+1) return {true,voice.voiced};
#ifdef FFX_READER_TEST
    const auto now=jechtTickForTest();
#else
    const auto now=GetTickCount();
#endif
    return {now-caption.requestedAt>=500,false};
}
void installJechtVoice() {
    hookSnapshot(jecht_voice_rva::Start,rootBefore,rootAfter);
    hookSnapshot(jecht_voice_rva::Events,eventsBefore,eventsAfter);
    hookSnapshot(jecht_voice_rva::Dispose,disposed,nullptr);
    hookSnapshot(0x548c9a,nullptr,started);hookSnapshot(0x307b40,nullptr,completed);
}
}
