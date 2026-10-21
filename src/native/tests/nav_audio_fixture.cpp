#include <atomic>
#include "../nav_audio.hpp"
#include "../sdl_runtime.hpp"
#include "../settings.hpp"
#include "../audio_sample.hpp"
#include "../butterfly.hpp"
#include "ready_speech_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <vector>
namespace ffx {
bool testForeground=true;
bool audioReady=false;
extern void(*navAudioAfterDrainForTest)();
std::atomic<int> scene{0};
bool nativeGameThread() {return true;}
bool modMenuActive() { return false; }
unsigned epoch=1;
unsigned generation() { return epoch; }
unsigned copiedWallCues() { return 0; }
void cancelWallCues() {}
bool output(const std::wstring&,bool,const char*) { return true; }
void log(const char* message,...) { if(strstr(message,"audio ready")) audioReady=true; }
}
using namespace ffx;
void check(bool ok,const char* message) { if(!ok) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
namespace {
SDL_AudioStreamCallback productionCallback=nullptr;
void* callbackData=nullptr;
SDL_AudioStream* testStream=nullptr;
decltype(&SDL_OpenAudioDeviceStream) realOpen=nullptr;
std::vector<float> samples;
SDL_AudioStream* SDLCALL openStream(SDL_AudioDeviceID device,const SDL_AudioSpec* spec,SDL_AudioStreamCallback callback,void* data) {
    productionCallback=callback; callbackData=data;
    // Open/resume the real dummy device without its asynchronous callback.
    // Drive the exact production mixer synchronously so silence is measurable.
    return testStream=realOpen(device,spec,nullptr,nullptr);
}
bool SDLCALL captureSamples(SDL_AudioStream*,const void* data,int bytes) {
    const auto* values=static_cast<const float*>(data);
    samples.insert(samples.end(),values,values+bytes/sizeof(float)); return true;
}
void mix(int frames=4800) {
    samples.clear(); productionCallback(callbackData,testStream,frames*2*sizeof(float),0);
    check(samples.size()==static_cast<size_t>(frames)*2,"production callback fills requested stereo frames");
}
double energy(unsigned channel) {
    double total=0;
    for(size_t i=channel;i<samples.size();i+=2) {
        check(std::isfinite(samples[i]) && std::abs(samples[i])<=1,"finite bounded mixed samples");
        total+=std::abs(samples[i]);
    }
    return total;
}
bool silent() { return energy(0)==0 && energy(1)==0; }
}
int main() {
    // Observe the production copied-record logger rather than the retired
    // synchronous log function. No formatted logging is needed by this host.
    logCaptureHandler=[](const LogCapture& packet) noexcept {
        if(std::strstr(packet.format.data(),"audio ready")) audioReady=true;
        if(std::strstr(packet.format.data(),"unavailable") || std::strstr(packet.format.data(),"invalid"))
            std::fprintf(stderr,"Audio initialization: %s\n",packet.format.data());
    };
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER,"dummy");
    check(sdl::load(),"x86 SDL runtime");
    audio::Sample beacon,arrival;
    check(beacon.load(GetModuleHandleW(nullptr),201) && arrival.load(GetModuleHandleW(nullptr),202),"embedded audio decodes");
    check(beacon.count>0 && arrival.count>0 && beacon.read(-1)==0,"bounded sample playback");
    realOpen=sdl::SDL_OpenAudioDeviceStream; sdl::SDL_OpenAudioDeviceStream=openStream;
    sdl::SDL_PutAudioStreamData=captureSamples;
    setSetting(Setting::Beacon,1); setSetting(Setting::Walls,0); setSetting(Setting::Arrival,1);
    startNavAudio(); check(audioReady && productionCallback && testStream,"real dummy stream opens with production callback");
    // Cancellation after a command was accepted must still retire it before
    // synthesis, even when a new event counter would otherwise start a tone.
    navAudioLightningWarning(epoch);navAudioAfterDrainForTest=[] { navAudioCancelLightningWarning(); };
    mix();check(silent(),"post-drain warning cancellation cannot restart accepted old event");navAudioAfterDrainForTest=nullptr;
    previewNavAudio(false);navAudioAfterDrainForTest=[] { stopNavPreview(); };
    mix();check(silent(),"post-drain preview cancellation cannot restart accepted old event");navAudioAfterDrainForTest=nullptr;
    navAudioOverdriveBeep(880,epoch);navAudioAfterDrainForTest=[] { navAudioCancelOverdriveBeep(); };
    mix();check(silent(),"post-drain overdrive cancellation cannot restart accepted old event");navAudioAfterDrainForTest=nullptr;
    NavAudioState state; state.active=state.beacon=true; state.pan=1; state.front=-1; state.steps=30;
    state.contextSerial=navAudioContextSerial();publishNavAudio(state); mix();
    check(energy(0)==0 && energy(1)>1,"enabled route beacon produces right-channel audio immediately");
    setSetting(Setting::Beacon,0); mix(); check(silent(),"beacon off stops the current ping");
    setSetting(Setting::Beacon,1); mix();
    check(energy(1)>1,"re-enabling starts immediately before the old ping deadline");
    ++epoch; state.pan=-1; state.contextSerial=navAudioContextSerial();publishNavAudio(state); mix();
    check(energy(0)>1 && energy(1)==0,"new route bearing produces left-channel audio");
    state.beacon=false; state.contextSerial=navAudioContextSerial();publishNavAudio(state); mix(); check(silent(),"route with no next leg stops beacon");
    navAudioArrival(); mix(); check(energy(0)>1 && energy(1)>1,"enabled beacon permits centered arrival audio");
    setSetting(Setting::Beacon,0); mix(); check(silent(),"beacon off cancels an arrival already playing");
    setSetting(Setting::Beacon,1); mix(48000); check(silent(),"re-enable cannot replay cancelled arrival");
    setSetting(Setting::Beacon,0); navAudioArrival(); setSetting(Setting::Beacon,1); mix();
    check(silent(),"arrival while beacon off is never queued for later");
    navAudioArrival(); setSetting(Setting::Beacon,0); mix(); check(silent(),"callback suppresses arrival queued just before toggle off");
    setSetting(Setting::Beacon,1); mix(); check(silent(),"callback consumes suppressed arrival without replay");
    setSetting(Setting::Arrival,0); navAudioArrival(); mix(); check(silent(),"arrival preference off remains authoritative");
    setSetting(Setting::Arrival,1); setSetting(Setting::Beacon,0); state.walls=15;
    setSetting(Setting::Walls,1); state.contextSerial=navAudioContextSerial();publishNavAudio(state); mix();
    check(energy(0)>1 && energy(1)>1,"independent wall tones still work with beacon off");
    ++epoch; state.walls=0; setSetting(Setting::Walls,0); state.contextSerial=navAudioContextSerial();publishNavAudio(state); pauseNavAudio();
    previewNavAudio(false); mix(); check(energy(1)>energy(0) && energy(0)>1,"explicit beacon preview works while route beacon is off");
    stopNavPreview(); mix(); check(silent(),"stopping preview silences its sample");
    state.beacon=true; setSetting(Setting::Beacon,1); state.contextSerial=navAudioContextSerial();publishNavAudio(state); pauseNavAudio(); mix();
    check(silent(),"paused field cues cannot play beacon");
    const auto stale=state;pauseNavAudio();publishNavAudio(stale);mix();check(silent(),"late copied state cannot reopen Native-paused audio");
    state.contextSerial=navAudioContextSerial();publishNavAudio(state);mix();check(!silent(),"fresh Native context recovers beacon after pause");
    testForeground=false; state.contextSerial=navAudioContextSerial();publishNavAudio(state); navAudioArrival(); mix(); check(silent(),"foreign focus silences beacon and arrival");
    testForeground=true; ++epoch; publishNavAudio({}); mix(); check(silent(),"map teardown clears audio");
    publishPositionalCue(1,.8f,true,epoch); mix();
    check(energy(0)==0 && energy(1)>1,"continuous pickup loop gives accurate right-channel alignment without a ping deadline");
    setSetting(Setting::RaceCues,0);mix();
    check(silent(),"race toggle retires an already mixed continuous pickup without a producer refresh");
    setSetting(Setting::RaceCues,1);mix();
    check(silent(),"race re-enable cannot revive the retired pickup");
    publishPositionalCue(1,.8f,true,epoch);mix();++epoch;mix();
    check(silent(),"generation change retires an already mixed pickup without a reset command");
    publishPositionalCue(1,.8f,true,epoch);mix();
    pauseNavAudio(); mix(); check(silent(),"pause cancels pickup cues immediately");
    publishPositionalCue(-1,.8f,true,epoch); mix(); check(energy(0)>1 && energy(1)==0,"pickup stereo follows new lateral error immediately");
    publishPositionalCue(0,0,false,epoch); publishPositionalCue(1,1,true,epoch-1); mix(); check(silent(),"stale resource generation cannot restart a pickup cue");
    scene=1; publishPositionalCue(0,1,true,epoch); mix(); check(silent(),"native menu context suppresses pickup audio"); scene=0;
    setSetting(Setting::Beacon,0); setSetting(Setting::Walls,0); setSetting(Setting::Speech,0);
    publishNavAudio({}); pauseNavAudio();
    setSetting(Setting::LightningWarning,1); navAudioLightningWarning(epoch); mix();
    check(energy(0)>1 && energy(0)==energy(1),"lightning beep is centered and independent of speech, routes, beacon and walls");
    for(size_t i=2880*2;i<samples.size();++i) check(samples[i]==0,"lightning beep ends after sixty milliseconds");
    mix(); check(silent(),"lightning event plays once");
    navAudioLightningWarning(epoch); pauseNavRouteAudio(); mix();
    check(energy(0)>1 && energy(0)==energy(1),"ordinary route changes cannot erase the independent lightning warning");
    navAudioLightningWarning(epoch); pauseNavAudio(); mix();
    check(silent(),"unsafe-context pause still cancels independent lightning warning");
    navAudioLightningWarning(epoch); setSetting(Setting::LightningWarning,0); mix(); check(silent(),"lightning toggle cancels a pending beep");
    setSetting(Setting::LightningWarning,1); mix(); check(silent(),"re-enable never replays suppressed beep");
    navAudioLightningWarning(epoch); navAudioCancelLightningWarning(); mix(); check(silent(),"context cancellation suppresses pending lightning");
    navAudioLightningWarning(epoch); testForeground=false; mix(); check(silent(),"focus loss suppresses pending lightning");
    testForeground=true; mix(); check(silent(),"focus return never replays old lightning");
    navAudioLightningWarning(epoch); ++epoch; mix(); check(silent(),"generation change cancels old lightning");
    navAudioLightningWarning(epoch-1); mix(); check(silent(),"stale producer cannot queue a beep");
    setSetting(Setting::ButterflyCues,1);
    ButterflyCue butterfly; butterfly.epoch=epoch; butterfly.hazard=true; butterfly.active=true;
    butterfly.pan=1; butterfly.proximity=1; publishButterflyCue(butterfly); mix(24000);
    check(energy(0)==0 && energy(1)>1,"red butterfly warning is spatial and independent of route/speech settings");
    setSetting(Setting::ButterflyCues,0); mix(); check(silent(),"butterfly setting immediately suppresses hazard audio");
    setSetting(Setting::ButterflyCues,1); publishButterflyCue(butterfly); pauseNavAudio(); mix(); check(silent(),"context pause cancels red hazard audio");
    butterfly.active=false; publishButterflyCue(butterfly); mix(); check(silent(),"inactive red event cannot retain warning");
    setSetting(Setting::RaceCues,0);
    butterfly.hazard=false; butterfly.active=true; butterfly.pan=-1; publishButterflyCue(butterfly); mix();
    check(energy(0)>1 && energy(1)==0,"blue butterfly uses the distinct continuous pickup cue");
    setSetting(Setting::ButterflyCues,0);mix();check(silent(),"butterfly toggle retires an accepted blue pickup without a producer refresh");
    setSetting(Setting::ButterflyCues,1);mix();check(silent(),"butterfly re-enable cannot revive the old blue pickup");
    setSetting(Setting::RaceCues,1);
    butterfly.active=false; publishButterflyCue(butterfly); mix(); check(silent(),"blue target removal cancels pickup cue");
    navAudioOverdriveBeep(880.f,epoch); mix(); check(energy(0)>1 && energy(0)==energy(1),"manual overdrive cue is independent centered audio");
    for(size_t i=1920*2;i<samples.size();++i) check(samples[i]==0,"overdrive beep ends after forty milliseconds");
    mix(); check(silent(),"short overdrive cue finishes without repeating");
    navAudioOverdriveBeep(880.f,epoch); navAudioCancelOverdriveBeep(); mix(); check(silent(),"context cancels queued overdrive cue");
    navAudioOverdriveBeep(880.f,epoch); setSetting(Setting::OverdriveCues,0); mix(); check(silent(),"overdrive cue setting suppresses a pending beep");
    setSetting(Setting::OverdriveCues,1); mix(); check(silent(),"re-enable does not replay an old cue");
    setSetting(Setting::OverdriveCues,0);
    navAudioTimingBeep(1100.f,epoch,Setting::GagazetCues); mix();
    check(energy(0)>1 && energy(0)==energy(1),"Gagazet cue works with Overdrive cues off");
    mix();navAudioTimingBeep(1100.f,epoch,Setting::GagazetCues);navAudioCancelGagazetBeep();mix();
    check(silent(),"leaving the launch window cancels queued Gagazet audio");
    navAudioTimingBeep(1100.f,epoch,Setting::GagazetCues);Sleep(120);mix();
    check(silent(),"a delayed audio queue cannot play an obsolete Gagazet launch tone");
    setSetting(Setting::OverdriveCues,1);navAudioOverdriveBeep(880.f,epoch);navAudioCancelGagazetBeep();mix();
    check(energy(0)>1,"Gagazet cancellation leaves independent battle cues available");mix();
    navAudioTimingBeep(1100.f,epoch,Setting::GagazetCues); setSetting(Setting::GagazetCues,0); mix();
    check(silent(),"Gagazet cue toggle cancels its own pending beep");
    setSetting(Setting::GagazetCues,1); mix(); check(silent(),"Gagazet re-enable does not replay old cue");
    setSetting(Setting::OverdriveCues,1);
    navAudioOverdriveBeep(880.f,epoch); testForeground=false; mix(); check(silent(),"focus loss suppresses overdrive beep");
    testForeground=true; mix(); check(silent(),"focus return cannot replay overdrive beep");
    navAudioOverdriveBeep(880.f,epoch-1); mix(); check(silent(),"stale overdrive producer is rejected");
    setSetting(Setting::LightningWarning,1);
    for(unsigned i=0;i<80;++i) navAudioLightningWarning(epoch);
    navAudioCancelLightningWarning(); mix(); check(silent(),"full command queue cancellation retires pending lightning events");
    navAudioLightningWarning(epoch); mix(); check(energy(0)>1 && energy(0)==energy(1),"fresh lightning event works after queue saturation and cancellation");
    // The temporary blanket mute is gone. Only field/minigame continuous
    // voices are ineligible while search owns movement; previews and actual
    // independent timing events must still play through their normal gates.
    publishPositionalCue(0,1,true,epoch);
    navAudioAfterDrainForTest=[] {setFieldAudioAllowed(false);};mix();
    check(silent(),"search context retires accepted looping voices after command drain");navAudioAfterDrainForTest=nullptr;
    state.active=state.beacon=true;state.walls=15;setSetting(Setting::Beacon,1);setSetting(Setting::Walls,1);
    state.contextSerial=navAudioContextSerial();publishNavAudio(state);publishPositionalCue(0,1,true,epoch);
    butterfly.active=butterfly.hazard=true;publishButterflyCue(butterfly);
    navAudioArrival();mix(48000);
    check(silent(),"search state rejects route, wall, arrival and both pickup/hazard sources");
    for(unsigned i=0;i<80;++i) publishPositionalCue(0,1,true,epoch);
    mix(48000);check(silent(),"rapid search publications cannot start a pickup loop");
    previewNavAudio(false);mix();check(!silent(),"explicit beacon preview is restored in search");stopNavPreview();mix();
    previewNavAudio(true);mix();check(!silent(),"explicit wall preview is restored in search");stopNavPreview();mix();
    navAudioTimingBeep(1100,epoch,Setting::GagazetCues);mix();check(!silent(),"independent gated timing event is not blanket muted in search");mix();
    setFieldAudioAllowed(true);mix(48000);check(silent(),"leaving search cannot replay old field commands or fade tails");
    previewNavAudio(false);mix();check(!silent(),"fresh non-search audio remains available after return");
    stopNavPreview();
    SDL_DestroyAudioStream(testStream); SDL_QuitSubSystem(SDL_INIT_AUDIO);
    puts("PASS: real x86 SDL dummy stream, production mixed audio, stereo beacon, immediate toggles, gated/cancelled arrival, independent walls, preview, focus and pause.");
}
