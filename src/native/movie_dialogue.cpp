#include "events.hpp"
#include "dialogue_voice.hpp"
#include "movie_dialogue.hpp"
#include "speech_domains.hpp"
#include "copied_controls.hpp"
#include <atomic>
#include <cstring>
namespace ffx {
#ifdef FFX_READER_TEST
void drainSpeechDomainsForTest();
#endif
namespace {
struct Audio {uintptr_t owner=0,channel=0;bool started=false,paused=false;float volume=1;};
struct Movie {uintptr_t owner=0,channel=0,track=0;bool playing=false,initialized=false,paused=false;int playback=0;};
struct Caption {uintptr_t owner=0,row=0;int index=-1;unsigned epoch=0;};
enum Kind : unsigned {Preparing,Prepared,Volume,Paused,Stopped,Updated,Gap,Drawn,Reset};
struct Packet {Audio audio;Movie movie;Caption caption;CopiedControls controls;
    std::array<uint8_t,256> encoded{};unsigned length=0,kind=Reset;};
Audio audio;Movie movie;Caption caption; // Speech preparation owner only.
unsigned workerEpoch=0;
std::atomic<unsigned> publishedEpoch{0};
std::atomic<bool> publishedVoiced{false};
thread_local unsigned playSerial=0;
thread_local uintptr_t playedChannel=0,timelineOwner=0;
thread_local bool playedPaused=false,timelineDrawn=false;
bool captureControls=true; // Bootstrap-configured, immutable after hooks enable.
Movie snapshot(uintptr_t owner) {
    Movie s;s.owner=owner;s.playing=read<uint8_t>(owner+0x6d0)!=0;s.initialized=read<uint8_t>(owner+0x6d2)!=0;
    s.paused=read<uint8_t>(owner+0x6fc)!=0;s.playback=read<int>(owner+0x38+0xe4);
    s.channel=read<uintptr_t>(owner+0x38+0x2a0);s.track=read<uintptr_t>(owner+0x38+0x294);return s;
}
bool audible(const Movie& s) {
    return s.playing && s.initialized && (s.playback==1 || s.playback==2) && s.track &&
        s.channel && audio.started && audio.owner==s.owner+0x38 &&
        audio.channel==s.channel && !audio.paused && audio.volume>0;
}
void publish(unsigned epoch) {publishedVoiced.store(audible(movie),std::memory_order_relaxed);publishedEpoch.store(epoch,std::memory_order_release);}
void resetOwned(unsigned epoch) {
    if(workerEpoch && static_cast<int>(epoch-workerEpoch)<0) return;
    workerEpoch=epoch;audio={};movie={};caption={};publish(epoch);
}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(Packet)) return;
    if(workerEpoch && static_cast<int>(header.generation-workerEpoch)<0) return;
    if(workerEpoch!=header.generation) resetOwned(header.generation);
    Packet packet;std::memcpy(&packet,owned.data(),sizeof(packet));
    switch(packet.kind) {
    case Preparing:caption={};if(audio.owner==packet.audio.owner) audio={};break;
    case Prepared:audio=packet.audio;break;
    case Volume:if(audio.owner==packet.audio.owner && audio.channel==packet.audio.channel) audio.volume=packet.audio.volume;break;
    case Paused:if(audio.channel && audio.channel==packet.audio.channel) audio.paused=packet.audio.paused;break;
    case Stopped:if(audio.owner==packet.audio.owner) audio={};break;
    case Updated:movie=packet.movie;if(!movie.playing) caption={};break;
    case Gap:caption={};break;
    case Drawn:
    {
        const auto text=clean(join(decodeOwnedBytes(packet.encoded.data(),packet.length,
            OwnedTextMode::Terminated,copiedControlContext(packet.controls)),L" "));
        if(text.empty()) {caption={};break;}
        if(caption.owner==packet.caption.owner && caption.row==packet.caption.row &&
            caption.index==packet.caption.index && caption.epoch==packet.caption.epoch) return;
        caption=packet.caption;movie=packet.movie;
        speechq::emitOwned(text,header,"movie-caption",audible(movie) || voicedDialogueAt(header.generation));break;
    }
    case Reset:resetOwned(header.generation);break;
    }
    publish(header.generation);
}
void send(Packet packet,unsigned epoch) {speechq::captureDomain(speechq::MovieVoiceDomain,packet,epoch?epoch:generation());}
void audioPreparing(Event& e) {e.before[0]=playSerial;Packet packet;packet.kind=Preparing;packet.audio.owner=e.ecx;send(packet,e.epoch);}
void fmodPlayed(Event& e) {
    if(e.result!=0 || e.parentRva!=0x6273b0) return;
    const auto channel=read<uintptr_t>(e.args[4]);if(!channel) return;
    ++playSerial;playedChannel=channel;playedPaused=e.args[3]!=0;
}
void audioPrepared(Event& e) {
    if(playSerial==static_cast<unsigned>(e.before[0]) || e.result==0) return;
    const auto channel=read<uintptr_t>(e.ecx+0x2a0);if(channel!=playedChannel) return;
    Packet packet;packet.kind=Prepared;packet.audio={e.ecx,channel,true,playedPaused,1};send(packet,e.epoch);
}
void volumeChanged(Event& e) {
    if(e.result!=1) return;
    Packet packet;packet.kind=Volume;packet.audio.owner=e.ecx;packet.audio.channel=read<uintptr_t>(e.ecx+0x2a0);
    const auto bits=static_cast<uint32_t>(e.args[0]);std::memcpy(&packet.audio.volume,&bits,sizeof(bits));send(packet,e.epoch);
}
void paused(Event& e) {
    if(e.result!=0) return;
    Packet packet;packet.kind=Paused;packet.audio.channel=e.args[0];packet.audio.paused=e.args[1]!=0;send(packet,e.epoch);
}
void audioStopped(Event& e) {Packet packet;packet.kind=Stopped;packet.audio.owner=e.ecx;send(packet,e.epoch);}
void updated(Event& e) {Packet packet;packet.kind=Updated;packet.movie=snapshot(e.ecx);send(packet,e.epoch);}
void movieReset(Event&) {resetMovieDialogue();}
void timelineBefore(Event& e) {e.before[0]=timelineOwner;e.before[1]=timelineDrawn;timelineOwner=e.ecx;timelineDrawn=false;}
void timelineAfter(Event& e) {
    if(!timelineDrawn) {Packet packet;packet.kind=Gap;send(packet,e.epoch);}
    timelineOwner=static_cast<uintptr_t>(e.before[0]);timelineDrawn=e.before[1]!=0;
}
void captionBefore(Event& e) {
    if(e.parentRva!=0x2dad80 || !timelineOwner) return;
    const auto owner=timelineOwner,row=read<uintptr_t>(owner+0x72c);
    if(!row || row==read<uintptr_t>(owner+0x724)) return;
    const int index=read<int>(row+8,-1),count=read<int>(owner+0x748);if(index<0 || index>=count) return;
    // Copy only the exact renderer-selected ready bytes and native controls.
    // Preparation owns decoding, formatting and the existing row-lifetime guard.
    Packet packet;packet.kind=Drawn;packet.movie=snapshot(owner);packet.caption={owner,row,index,e.epoch};
    packet.length=copyReadyGameText(e.args[0],packet.encoded.data(),static_cast<unsigned>(packet.encoded.size()));
    if(!packet.length || !packet.encoded[0]) return;
    if(captureControls) prepareCopiedControlSnapshot(packet.controls);
    else packet.controls.polish=textEncodingSnapshot()==1;
    timelineDrawn=speechq::captureDomain(speechq::MovieVoiceDomain,packet,e.epoch?e.epoch:generation());
}
}
bool movieDialogueVoiced() {
#ifdef FFX_READER_TEST
    drainSpeechDomainsForTest();
#endif
    return publishedEpoch.load(std::memory_order_acquire)==generation() && publishedVoiced.load(std::memory_order_relaxed);}
bool movieDialogueVoicedAt(unsigned epoch) {return workerEpoch==epoch && audible(movie);}
void resetMovieDialogue() {Packet packet;packet.kind=Reset;send(packet,generation());timelineOwner=0;timelineDrawn=false;}
void installMovieDialogue(bool captionsOnly) {
    captureControls=!captionsOnly;
    speechq::registerDomain(speechq::MovieVoiceDomain,process,resetOwned);
    hookSnapshot(0x6273b0,audioPreparing,audioPrepared);hookSnapshot(0x548cf4,nullptr,fmodPlayed);
    hookSnapshot(0x628330,nullptr,volumeChanged);hookSnapshot(0x627e90,nullptr,nullptr);hookSnapshot(0x67c90a,nullptr,paused);
    hookSnapshot(0x628350,audioStopped,nullptr);
    if(!captionsOnly) {hookSnapshot(0x2d9db0,movieReset,nullptr);hookSnapshot(0x2d72e0,movieReset,nullptr);}
    hookSnapshot(0x2d90f0,nullptr,nullptr);hookSnapshot(0x2d77b0,nullptr,updated);
    hookSnapshot(0x2dad80,timelineBefore,timelineAfter);hookSnapshot(0x505d30,captionBefore,nullptr);
}
}
