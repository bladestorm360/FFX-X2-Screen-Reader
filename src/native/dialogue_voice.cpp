// Qualify native voice playback before deciding speech/braille policy. Missing,
// muted or failed voice must leave the unvoiced line readable; the policy is
// local to dialogue rather than a global filter on all output.
// See Docs/features/menus/index.md#dialogue-and-review.
#include "events.hpp"
#include "dialogue_voice.hpp"
#include "movie_dialogue.hpp"
#include "speech_domains.hpp"
#include "process_storage.hpp"
#include <atomic>
#include <cstring>
namespace ffx {
#ifdef FFX_READER_TEST
void drainSpeechDomainsForTest();
#endif
namespace {
struct Voice {
    uintptr_t owner=0,event=0;
    bool playing=false,prepared=false,paused=false,muted=false;
    float volume=0,mix=0;
};
enum Kind : unsigned {Prepared,Started,Changed,Cleared,Completed,State,Submit,Reset};
struct Packet {
    Voice voice;
    unsigned kind=Reset,query=0;
    uintptr_t event=0,owner=0;
    bool explicitMovie=false,movie=false;
    char producer[64]{};
};
struct Caption {std::wstring text;char producer[64];speechq::Header header;};
Voice voice;
ProcessStorage<std::vector<Caption>> waitingStorage;
std::vector<Caption>& waiting() noexcept {return waitingStorage.get();}
unsigned workerEpoch=0,waitingCharacters=0;
std::atomic<unsigned> publishedEpoch{0};
std::atomic<bool> publishedVoiced{false};
thread_local unsigned startSerial=0,attemptSerial=0;
thread_local bool startSucceeded=false;
thread_local uintptr_t startedEvent=0;
bool audible(const Voice& s) {return s.playing && !s.paused && !s.muted && s.volume>0 && s.mix>0;}
Voice snapshot(uintptr_t owner,bool playing) {
    Voice s;s.owner=owner;s.event=read<uintptr_t>(owner+0x14);
    s.playing=playing && s.event;s.paused=read<unsigned>(owner+0x54)==1;
    s.muted=read<unsigned>(owner+0x80)==1;
    s.volume=read<float>(owner+0x50);s.mix=read<float>(owner+0x88);return s;
}
void publish(unsigned epoch) {
    publishedVoiced.store(audible(voice),std::memory_order_relaxed);
    publishedEpoch.store(epoch,std::memory_order_release);
}
void resetOwned(unsigned epoch) {
    if(workerEpoch && static_cast<int>(epoch-workerEpoch)<0) return;
    workerEpoch=epoch;voice={};waiting().clear();waitingCharacters=0;publish(epoch);
}
void flush(const speechq::Header& header,bool voiced) {
    (void)header;auto captions=std::move(waiting());waiting()={};waitingCharacters=0;
    for(auto& caption:captions) speechq::emitOwned(std::move(caption.text),caption.header,caption.producer,voiced);
}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(Packet)) return;
    if(workerEpoch && static_cast<int>(header.generation-workerEpoch)<0) return;
    if(workerEpoch!=header.generation) resetOwned(header.generation);
    Packet packet;std::memcpy(&packet,owned.data(),sizeof(packet));
    switch(packet.kind) {
    case Prepared:
        voice=packet.voice;
        recordLog("dialogue voice prepared event=%p pending=%d",reinterpret_cast<void*>(voice.event),voice.prepared);break;
    case Started:
        voice=packet.voice;
        recordLog("dialogue voice started event=%p audible=%d captions=%zu",reinterpret_cast<void*>(voice.event),audible(voice),waiting().size());
        flush(header,audible(voice));break;
    case Changed: {
        auto next=packet.voice;
        if(voice.owner==next.owner && voice.event==next.event) {next.playing=voice.playing;next.prepared=voice.prepared;voice=next;}
        break; }
    case Cleared:
        if(packet.query || voice.owner==packet.owner) {voice={};flush(header,false);}break;
    case Completed:
        if(voice.owner && packet.event==voice.event && packet.owner==voice.owner+0x40) voice.playing=false;break;
    case State:
        if(voice.event && voice.event==packet.event) voice.playing=packet.query!=0;break;
    case Submit: {
        submitDialogueOwned(owned.substr(sizeof(Packet)/sizeof(wchar_t)),header,packet.producer,packet.explicitMovie,packet.movie);
        break; }
    case Reset:resetOwned(header.generation);break;
    }
    publish(header.generation);
}
void send(Packet packet,unsigned epoch) {
    speechq::captureDomain(speechq::DialogueVoiceDomain,packet,epoch?epoch:generation());
}
void prepared(Event& e) {
    Packet packet;packet.kind=Prepared;packet.voice=snapshot(e.ecx,false);
    packet.voice.prepared=packet.voice.event && read<unsigned>(e.ecx+0x60)==1;send(packet,e.epoch);
}
void starting(Event& e) {e.before[0]=startSerial;e.before[1]=attemptSerial;}
void started(Event& e) {
    Packet packet;
    if(attemptSerial==static_cast<unsigned>(e.before[1])) {
        if(!read<uintptr_t>(e.ecx+0x14)) {packet.kind=Cleared;packet.query=1;send(packet,e.epoch);}return;
    }
    packet.voice=snapshot(e.ecx,startSucceeded);if(packet.voice.event!=startedEvent) return;
    packet.kind=Started;send(packet,e.epoch);
}
void fmodStarted(Event& e) {
    if(e.parentRva==0x30b6b0 || e.parentRva==0x30b250) {
        ++attemptSerial;startSucceeded=e.result==0;if(startSucceeded) ++startSerial;startedEvent=e.args[0];
    }
}
void stateChanged(Event& e) {
    Packet packet;packet.kind=State;packet.event=e.args[0];packet.query=e.result==0 && (read<unsigned>(e.args[1])&8)!=0;send(packet,e.epoch);
}
void changed(Event& e) {Packet packet;packet.kind=Changed;packet.voice=snapshot(e.ecx,false);send(packet,e.epoch);}
void cleared(Event& e) {Packet packet;packet.kind=Cleared;packet.owner=e.ecx;send(packet,e.epoch);}
void completed(Event& e) {
    if(e.args[1]!=2 && e.args[1]!=4) return;
    Packet packet;packet.kind=Completed;packet.event=e.args[0];packet.owner=e.args[4];send(packet,e.epoch);
}
void submit(const std::wstring& text,const char* producer,bool explicitMovie,bool movie) {
    Packet packet;packet.kind=Submit;packet.explicitMovie=explicitMovie;packet.movie=movie;
    if(producer) std::memcpy(packet.producer,producer,strnlen_s(producer,sizeof(packet.producer)-1));
    speechq::captureDomain(speechq::DialogueVoiceDomain,packet,generation(),text);
}
}
void submitDialogueOwned(std::wstring text,const speechq::Header& header,const char* producer,bool explicitMovie,bool explicitVoiced) {
    if(header.generation!=generation()) return;
    if(workerEpoch!=header.generation) resetOwned(header.generation);
    const bool movie=explicitMovie?explicitVoiced:movieDialogueVoicedAt(header.generation);
    if(!movie && voice.prepared && !voice.playing) {
        if(waiting().size()>=speechq::EventLimit || text.size()>speechq::CharacterLimit-waitingCharacters) {
            recordLog("dialogue pending caption capacity refused");return;
        }
        Caption caption{std::move(text),{},header};
        if(producer) std::memcpy(caption.producer,producer,strnlen_s(producer,sizeof(caption.producer)-1));
        waitingCharacters+=static_cast<unsigned>(caption.text.size());waiting().push_back(std::move(caption));
        recordLog("dialogue caption awaiting native voice event=%p",reinterpret_cast<void*>(voice.event));
    } else speechq::emitOwned(std::move(text),header,producer,movie || audible(voice));
}
bool voicedDialogue() {
#ifdef FFX_READER_TEST
    drainSpeechDomainsForTest();
#endif
    return publishedEpoch.load(std::memory_order_acquire)==generation() && publishedVoiced.load(std::memory_order_relaxed);}
bool voicedDialogueAt(unsigned epoch) {return workerEpoch==epoch && audible(voice);}
void submitDialogue(const std::wstring& text,const char* producer,bool movieVoiced) {submit(text,producer,true,movieVoiced);}
void submitDialogue(const std::wstring& text,const char* producer) {submit(text,producer,false,false);}
void resetDialogueVoice() {Packet packet;packet.kind=Reset;send(packet,generation());}
void installDialogueVoice() {
    waitingStorage.prepare();
    speechq::registerDomain(speechq::DialogueVoiceDomain,process,resetOwned);
    hookSnapshot(0x30b6b0,starting,started);hookSnapshot(0x30b250,starting,started);
    hookSnapshot(0x548c9a,nullptr,fmodStarted);hookSnapshot(0x548ca6,nullptr,stateChanged);
    hookSnapshot(0x30aec0,cleared,prepared);hookSnapshot(0x30a680,cleared,nullptr);
    hookSnapshot(0x307b40,completed,nullptr);
    for(unsigned id:{0x30b8e0u,0x30b420u,0x30b3a0u,0x30b460u}) hookSnapshot(id,nullptr,changed);
}
}
