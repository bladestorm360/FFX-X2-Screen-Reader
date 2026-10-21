// Owns bounded command transport to the SDL mixer plus separate cue cancellation/gates; gameplay callbacks never initialize devices. See Docs/features/index.md and Docs/dylan_reports_20261018.md.
#include "nav_audio.hpp"
#include "sdl_runtime.hpp"
#include "settings.hpp"
#include "audio_sample.hpp"
#include "mod_menu.hpp"
#include "butterfly.hpp"
#include "nonblocking_snapshots.hpp"
#include "physical_gamepad.hpp"
#include "wall_cues.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <mutex>
namespace ffx {
#ifdef FFX_READER_TEST
void(*navAudioAfterDrainForTest)()=nullptr;
#endif
namespace {
enum class CommandKind : unsigned { State, Preview, Arrival, Positional, Hazard, Lightning, Overdrive };
struct Command { CommandKind kind{}; NavAudioState state{}; float pan=0,gain=0,frequency=880; unsigned generation=0,serial=0; uint64_t stamp=0;bool flag=false,enabled=false;Setting timingGate=Setting::OverdriveCues; };
async::TryCaptureQueue<Command,64> commands;
std::atomic<bool> streamReady{false};
// Native context owns permission for field/minigame loops, independently of
// previews and timing events. A delayed worker cannot re-arm these loops in a
// search UI. Context transitions retire their accepted voices and queued work.
std::atomic<bool> fieldAudioAllowed{true};
std::atomic<unsigned> fieldAudioRevision{0};
std::atomic<unsigned> previewCancel{0},lightningCancel{0},overdriveCancel{0},arrivalCancel{0};
std::atomic<unsigned> gagazetCancel{0};
unsigned timingSerial(Setting gate) {return gate==Setting::GagazetCues?gagazetCancel.load():overdriveCancel.load();}
std::atomic<unsigned> stateSerial{0},cueSerial{0},hazardSerial{0};
struct Sound { NavAudioState state; unsigned epoch=0,arrival=0,preview=0,cueEpoch=0,lightning=0,lightningEpoch=0;
    unsigned stateSerial=0,cueSerial=0,hazardSerial=0,stateEpoch=0;
    unsigned previewSerial=0,lightningSerial=0,overdriveSerial=0,arrivalSerial=0;
    unsigned overdrive=0,overdriveEpoch=0; float overdriveFrequency=880; bool overdriveOn=false;Setting timingGate=Setting::OverdriveCues;
    float cuePan=0,cueGain=0,hazardPan=0,hazardGain=0; unsigned hazardEpoch=0;
    bool cue=false,hazard=false,previewWalls=false,previewOn=false,warning=false;
    Setting cueGate=Setting::RaceCues; } sound;
std::atomic<bool> paused{true};
std::atomic<bool> cuePaused{true};
std::atomic<bool> hazardPaused{true};
SDL_AudioStream* stream=nullptr;
audio::Sample beaconSample,arrivalSample,pickupSample;
std::once_flag audioOnce;
struct Mixer {
    Sound snapshot;
    uint64_t clock=0,nextPing=0;
    unsigned previewSerial=0,lightningSerial=0,overdriveSerial=0;
    unsigned epoch=0,arrival=0,preview=0,lightning=0;
    unsigned overdrive=0; int overdriveFrames=0; float overdrivePhase=0;
    int previewFrames=0,lightningFrames=0;
    float lightningPhase=0,hazardPhase=0;
    bool previewWalls=false,beacon=false;
    float beaconAt=-1,arrivalAt=-1,pickupAt=0,phases[4]{},gains[4]{},pan=0,rate=1;
} mixer;
unsigned mixedFieldRevision=0; // Audio callback owner only.
void retireFieldVoices() {
    const auto revision=fieldAudioRevision.load(std::memory_order_acquire);
    if(revision!=mixedFieldRevision) {
        sound.state={};sound.cue=sound.hazard=false;
        mixer.beaconAt=mixer.arrivalAt=-1;mixer.pickupAt=0;mixer.beacon=false;
        std::fill(std::begin(mixer.gains),std::end(mixer.gains),0.f);
        mixedFieldRevision=revision;
    }
}
float wave(float& phase,float frequency) {
    phase+=frequency/48000.f; phase-=std::floor(phase);
    return std::sin(phase*6.28318530718f);
}
void callback(void*,SDL_AudioStream* output,int additional,int) {
    retireFieldVoices();
    Command command{};
    for(unsigned drained=0;drained<64 && commands.tryPop(command);++drained) {
        if(command.generation!=generation()) continue;
        switch(command.kind) {
        case CommandKind::State: if(command.serial==stateSerial.load()) { sound.state=command.state; sound.epoch=sound.stateEpoch=command.generation;sound.stateSerial=command.serial; }break;
        case CommandKind::Preview: if(command.serial==previewCancel.load()) { ++sound.preview; sound.previewSerial=command.serial;sound.previewWalls=command.flag; sound.previewOn=command.enabled; } break;
        case CommandKind::Arrival: if(command.serial==arrivalCancel.load()) { ++sound.arrival;sound.arrivalSerial=command.serial; }break;
        case CommandKind::Positional: if(command.serial!=cueSerial.load()) break;sound.cueSerial=command.serial;sound.cue=command.enabled; sound.cueGate=command.timingGate;sound.cuePan=command.pan; sound.cueGain=command.gain; sound.cueEpoch=command.generation; sound.epoch=command.generation; break;
        case CommandKind::Hazard: if(command.serial!=hazardSerial.load()) break;sound.hazardSerial=command.serial;sound.hazard=command.enabled; sound.hazardPan=command.pan; sound.hazardGain=command.gain; sound.hazardEpoch=command.generation; sound.epoch=command.generation; break;
        case CommandKind::Lightning: if(command.serial==lightningCancel.load()) { sound.lightningSerial=command.serial;sound.lightningEpoch=command.generation; ++sound.lightning; sound.warning=true; } break;
        case CommandKind::Overdrive: if(command.serial==timingSerial(command.timingGate) &&
            (command.timingGate!=Setting::GagazetCues || GetTickCount64()-command.stamp<=100)) {
            sound.overdriveSerial=command.serial;sound.overdriveEpoch=command.generation; sound.overdriveFrequency=command.frequency;sound.timingGate=command.timingGate; ++sound.overdrive; sound.overdriveOn=true;
        } break;
        }
    }
#ifdef FFX_READER_TEST
    if(navAudioAfterDrainForTest) navAudioAfterDrainForTest();
#endif
    retireFieldVoices();
    // Retire accepted loops, rather than merely turning their gain down. In
    // particular, comparing cueEpoch with copy.epoch compares two old values
    // and used to let a pickup loop outlive its resource generation forever.
    if(!fieldAudioAllowed.load(std::memory_order_acquire) || sound.cueEpoch!=generation() ||
        !setting(sound.cueGate)) sound.cue=false;
    if(!fieldAudioAllowed.load(std::memory_order_acquire) || sound.hazardEpoch!=generation() ||
        !setting(Setting::ButterflyCues)) sound.hazard=false;
    const auto previewToken=previewCancel.load(),lightningToken=lightningCancel.load(),overdriveToken=timingSerial(sound.timingGate);
    if(mixer.previewSerial!=previewToken) { mixer.previewSerial=previewToken; mixer.previewFrames=0; }
    if(mixer.lightningSerial!=lightningToken) { mixer.lightningSerial=lightningToken; mixer.lightningFrames=0; }
    if(mixer.overdriveSerial!=overdriveToken) { mixer.overdriveSerial=overdriveToken; mixer.overdriveFrames=0; }
    Sound copy=sound; mixer.snapshot=copy;
    const bool focused=gameFocused();
    if(copy.epoch!=mixer.epoch) {
        mixer.epoch=copy.epoch; mixer.beaconAt=mixer.arrivalAt=-1; mixer.lightningFrames=0;
        mixer.nextPing=mixer.clock; std::fill(std::begin(mixer.gains),std::end(mixer.gains),0.f);
    }
    if(copy.preview!=mixer.preview) {
        mixer.preview=copy.preview; mixer.previewFrames=copy.previewOn && copy.previewSerial==previewToken?96000:0;
        mixer.previewWalls=copy.previewWalls; mixer.nextPing=mixer.clock; mixer.beaconAt=-1;
    }
    const bool active=focused && !paused.load() && copy.state.active && copy.stateEpoch==generation() && copy.stateSerial==stateSerial.load() && copy.state.contextSerial==navAudioContextSerial();
    // Wall calculations have their own copied-data owner. A stalled geometry
    // worker cannot hold the game or the SDL mixer; canceled/stale facts yield0.
    const unsigned ownedWalls=copiedWallCues();
    const bool arrivalEnabled=active && copy.arrivalSerial==arrivalCancel.load() && setting(Setting::Beacon) && setting(Setting::Arrival);
    if(copy.arrival!=mixer.arrival) {
        mixer.arrival=copy.arrival;
        if(arrivalEnabled) mixer.arrivalAt=0;
    }
    if(!arrivalEnabled) mixer.arrivalAt=-1;
    const bool warningEnabled=focused && copy.warning && setting(Setting::LightningWarning) &&
        copy.lightningEpoch==generation() && copy.lightningSerial==lightningToken;
    if(copy.lightning!=mixer.lightning) {
        mixer.lightning=copy.lightning; mixer.lightningFrames=warningEnabled?2880:0;
        mixer.lightningPhase=0;
    }
    if(!warningEnabled) mixer.lightningFrames=0;
    const bool overdriveEnabled=focused && copy.overdriveOn && copy.overdriveEpoch==generation() && copy.overdriveSerial==overdriveToken && setting(copy.timingGate);
    if(copy.overdrive!=mixer.overdrive) {
        mixer.overdrive=copy.overdrive; mixer.overdriveFrames=overdriveEnabled?1920:0; mixer.overdrivePhase=0;
    }
    if(!overdriveEnabled) mixer.overdriveFrames=0;
    if(!focused) { mixer.previewFrames=0; mixer.beaconAt=mixer.arrivalAt=-1; }
    float buffer[512*2]{};
    int frames=std::clamp(additional/static_cast<int>(sizeof(float)*2),0,48000);
    while(frames>0) {
        const int chunk=std::min(frames,512);
        for(int i=0;i<chunk;++i) {
            float left=0,right=0;
            const bool field=fieldAudioAllowed.load(std::memory_order_acquire);
            const bool routeActive=active && field;
            if(mixer.overdriveFrames>0) {
                const float envelope=std::min(1.f,std::min((1920-mixer.overdriveFrames)/120.f,mixer.overdriveFrames/120.f));
                const float value=wave(mixer.overdrivePhase,copy.overdriveFrequency)*.22f*envelope;
                left+=value; right+=value; --mixer.overdriveFrames;
            }
            if(field && focused && !hazardPaused.load() && copy.hazard && copy.hazardEpoch==generation() && copy.hazardSerial==hazardSerial.load() && setting(Setting::ButterflyCues)) {
                // A distinct low pulse marks a real red body-trigger hazard.
                // This renders a copied event state; it never queries game memory.
                const unsigned pulse=static_cast<unsigned>(mixer.clock%11520);
                if(pulse<2880) {
                    const float envelope=std::min(1.f,std::min(pulse/240.f,(2880-pulse)/240.f));
                    const float value=wave(mixer.hazardPhase,160.f)*.24f*envelope*copy.hazardGain*setting(Setting::BeaconVolume)/100.f;
                    left+=value*(1-std::max(0.f,copy.hazardPan));
                    right+=value*(1+std::min(0.f,copy.hazardPan));
                }
            }
            if(mixer.lightningFrames>0) {
                const float envelope=std::min(1.f,std::min((2880-mixer.lightningFrames)/240.f,mixer.lightningFrames/240.f));
                const float value=wave(mixer.lightningPhase,880.f)*.22f*envelope;
                left+=value; right+=value; --mixer.lightningFrames;
            }
            // A continuous replaceable pickup sample carries precise lateral
            // error. Unlike route pings, it never waits for a ping deadline.
            if(field && focused && !cuePaused.load() && copy.cue && copy.cueEpoch==generation() &&
                copy.cueSerial==cueSerial.load() && setting(copy.cueGate) && pickupSample.count) {
                const auto value=pickupSample.read(mixer.pickupAt)*copy.cueGain*setting(Setting::BeaconVolume)/100.f;
                left+=value*std::sqrt((1-copy.cuePan)*.5f); right+=value*std::sqrt((1+copy.cuePan)*.5f);
                if(++mixer.pickupAt>=pickupSample.count) mixer.pickupAt=0;
            } else mixer.pickupAt=0;
            const bool preview=focused && mixer.previewFrames>0;
            const bool beacon=(routeActive && copy.state.beacon && setting(Setting::Beacon)) || (preview && !mixer.previewWalls);
            if(beacon && !mixer.beacon) mixer.nextPing=mixer.clock;
            mixer.beacon=beacon;
            if(beacon && mixer.clock>=mixer.nextPing) {
                mixer.beaconAt=0; mixer.pan=preview?.65f:copy.state.pan;
                mixer.rate=preview?1.f:copy.state.front<0?.8f:1.15f;
                const float interval=preview?.6f:.25f+1.25f*std::clamp(copy.state.steps/30.f,0.f,1.f);
                mixer.nextPing=mixer.clock+static_cast<uint64_t>(interval*48000);
            }
            if(!beacon) mixer.beaconAt=-1;
            if(mixer.beaconAt>=0) {
                const float gain=setting(Setting::BeaconVolume)/100.f*(copy.state.front<0 && !preview?.7f:1.f);
                const float value=beaconSample.read(mixer.beaconAt)*gain;
                left+=value*std::sqrt((1-mixer.pan)*.5f); right+=value*std::sqrt((1+mixer.pan)*.5f);
                mixer.beaconAt+=mixer.rate; if(mixer.beaconAt>=beaconSample.count) mixer.beaconAt=-1;
            }
            constexpr float frequencies[]={294,196,247,165}; // ahead, left, right, behind
            for(unsigned direction=0;direction<4;++direction) {
                const bool previewWall=preview && mixer.previewWalls && direction==static_cast<unsigned>((96000-mixer.previewFrames)/24000);
                const bool wanted=previewWall || (routeActive && setting(Setting::Walls) && ((copy.state.walls|ownedWalls)&(1u<<direction)));
                mixer.gains[direction]+=std::clamp((wanted?1.f:0.f)-mixer.gains[direction],-1.f/480,1.f/480);
                if(!field && !previewWall) mixer.gains[direction]=0; // No field fade tail in search UI.
                const float value=mixer.gains[direction]>0?wave(mixer.phases[direction],frequencies[direction])*mixer.gains[direction]*.18f*setting(Setting::WallVolume)/100.f:0;
                left+=value*(direction==2?.25f:1.f); right+=value*(direction==1?.25f:1.f);
            }
            if(mixer.arrivalAt>=0) {
                if(routeActive) { const float value=arrivalSample.read(mixer.arrivalAt)*setting(Setting::ArrivalVolume)/100.f; left+=value; right+=value; }
                if(++mixer.arrivalAt>=arrivalSample.count || !routeActive) mixer.arrivalAt=-1;
            }
            buffer[2*i]=std::clamp(left,-1.f,1.f); buffer[2*i+1]=std::clamp(right,-1.f,1.f);
            ++mixer.clock; if(mixer.previewFrames>0) --mixer.previewFrames;
        }
        if(!sdl::SDL_PutAudioStreamData(output,buffer,chunk*static_cast<int>(sizeof(float)*2))) break;
        frames-=chunk;
    }
}
}
namespace {
void initializeAudioDevice() {
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&startNavAudio),&module);
    if(!beaconSample.load(module,201) || !arrivalSample.load(module,202)) { recordLog("SDL nav audio embedded assets invalid"); return; }
    pickupSample.load(module,204);
    wchar_t path[MAX_PATH]{};
    if(GetModuleFileNameW(module,path,MAX_PATH)) {
        std::wstring directory=path; directory.resize(directory.find_last_of(L"\\/")+1);
        for(const auto& asset:{std::pair<audio::Sample*,const wchar_t*>{&beaconSample,L"beacon.wav"},
            {&arrivalSample,L"arrival.wav"},{&pickupSample,L"pickup.wav"}}) {
            const auto file=directory+L"Sounds\\"+asset.second;
            // Missing/invalid replacements retain the embedded asset.
            auto replacement=new(std::nothrow) audio::Sample;
            if(replacement && replacement->loadFile(file.c_str())) { *asset.first=*replacement; recordLog("audio replacement loaded: %ls",asset.second); }
            delete replacement;
        }
    }
    const SDL_AudioSpec spec{SDL_AUDIO_F32,2,48000};
    stream=sdl::SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,callback,nullptr);
    if(!stream || !sdl::SDL_ResumeAudioStreamDevice(stream)) {
        recordLog("SDL nav audio unavailable: %s",sdl::SDL_GetError());
        if(stream) sdl::SDL_DestroyAudioStream(stream); stream=nullptr;
    } else { streamReady=true; recordLog("SDL nav audio ready: 48000 Hz stereo, embedded Bravely beacon/arrival"); }
}
#if !defined(FFX_READER_TEST) || defined(FFX_SDL_OWNER_TEST)
DWORD WINAPI audioDeviceWorker(void*) {
    physical::waitForAudio();
    if(physical::audioAvailable()) std::call_once(audioOnce,initializeAudioDevice);
    return 0;
}
#endif
}
void startNavAudio() {
#if defined(FFX_READER_TEST) && !defined(FFX_SDL_OWNER_TEST)
    if(sdl::load() && sdl::SDL_InitSubSystem(SDL_INIT_AUDIO)) std::call_once(audioOnce,initializeAudioDevice);
#else
    static std::atomic<bool> requested{false};
    if(requested.exchange(true)) return;
    physical::startOwner();
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&audioDeviceWorker),&pin)) return;
    const auto worker=CreateThread(nullptr,0,audioDeviceWorker,nullptr,0,nullptr);
    if(worker) CloseHandle(worker);else recordLog("navigation audio worker unavailable error=%lu",GetLastError());
#endif
}
void publishNavAudio(const NavAudioState& state) {
    if(!fieldAudioAllowed.load(std::memory_order_acquire) || state.contextSerial!=navAudioContextSerial()) return;
    Command command{}; command.kind=CommandKind::State; command.state=state; command.generation=generation();command.serial=stateSerial.fetch_add(1)+1;
    if(!commands.tryPush(command)) paused=true; else paused=!state.active;
}
void pauseNavAudio() { if(nativeGameThread()) navAudioContextRevision.fetch_add(1,std::memory_order_acq_rel);cancelWallCues();arrivalCancel.fetch_add(1);paused=true; cuePaused=true; hazardPaused=true; navAudioCancelLightningWarning(); }
void setFieldAudioAllowed(bool allowed) {
    if(!nativeGameThread() || allowed==fieldAudioAllowed.load(std::memory_order_acquire)) return;
    if(!allowed) fieldAudioAllowed.store(false,std::memory_order_release);
    pauseNavAudio();
    cueSerial.fetch_add(1,std::memory_order_acq_rel);hazardSerial.fetch_add(1,std::memory_order_acq_rel);
    fieldAudioRevision.fetch_add(1,std::memory_order_release);
    // Every exit also invalidates work produced while the UI owned movement.
    // Preview and independent timing cancellation are deliberately separate.
    if(allowed) fieldAudioAllowed.store(true,std::memory_order_release);
    recordLog("field-audio permission=%u gen=%u",allowed?1u:0u,generation());
}
void pauseNavRouteAudio() { if(nativeGameThread()) navAudioContextRevision.fetch_add(1,std::memory_order_acq_rel);paused=true;arrivalCancel.fetch_add(1); }
void stopNavPreview() {
    previewCancel.fetch_add(1);
}
void previewNavAudio(bool walls) {
    if(!streamReady.load()) { output(L"Navigation audio is unavailable",true,"mod-audio"); return; }
    Command command{}; command.kind=CommandKind::Preview; command.generation=generation(); command.serial=previewCancel.load(); command.flag=walls; command.enabled=true;
    if(!commands.tryPush(command)) { output(L"Navigation audio busy; try again.",true,"mod-audio"); return; }
    output(walls?L"Wall preview: ahead, left, right, behind":L"Beacon preview to the right",true,"mod-audio");
}
void navAudioArrival() {
    if(!fieldAudioAllowed.load(std::memory_order_acquire) || !setting(Setting::Beacon) || !setting(Setting::Arrival)) return;
    Command command{}; command.kind=CommandKind::Arrival; command.generation=generation();command.serial=arrivalCancel.load(); commands.tryPush(command);
}
namespace {
void publishPickupCue(float pan,float proximity,bool active,unsigned epoch,Setting gate) {
    if(epoch!=generation()) return;
    // No command per frame while the UI owns movement: reject at publication
    // as well as rendering, so a busy/stale producer cannot fill the mixer FIFO.
    if(!fieldAudioAllowed.load(std::memory_order_acquire)) {cuePaused=true;return;}
    const bool enabled=active && fieldAudioAllowed.load(std::memory_order_acquire) && setting(gate) &&
        !scene && !modMenuActive() && std::isfinite(pan) && std::isfinite(proximity);
    Command command{}; command.kind=CommandKind::Positional; command.generation=epoch;command.serial=cueSerial.fetch_add(1)+1; command.pan=std::isfinite(pan)?std::clamp(pan,-1.f,1.f):0;
    command.gain=std::isfinite(proximity)?std::clamp(proximity,0.f,1.f):0; command.enabled=enabled;command.timingGate=gate;
    if(!commands.tryPush(command)) cuePaused=true; else cuePaused=!enabled;
}
}
void publishPositionalCue(float pan,float proximity,bool active,unsigned epoch) {
    publishPickupCue(pan,proximity,active,epoch,Setting::RaceCues);
}
void publishButterflyCue(const ButterflyCue& cue) {
    if(cue.epoch!=generation()) return;
    if(!fieldAudioAllowed.load(std::memory_order_acquire)) {cuePaused=hazardPaused=true;return;}
    const bool enabled=cue.active && setting(Setting::ButterflyCues) && !scene && !modMenuActive();
    if(!cue.hazard) { publishPickupCue(cue.pan,cue.proximity,enabled,cue.epoch,Setting::ButterflyCues); return; }
    Command command{}; command.kind=CommandKind::Hazard; command.generation=cue.epoch;command.serial=hazardSerial.fetch_add(1)+1;
    command.enabled=enabled && std::isfinite(cue.pan) && std::isfinite(cue.proximity);
    command.pan=std::isfinite(cue.pan)?std::clamp(cue.pan,-1.f,1.f):0; command.gain=std::isfinite(cue.proximity)?std::clamp(cue.proximity,0.f,1.f):0;
    if(!commands.tryPush(command)) hazardPaused=true; else hazardPaused=!command.enabled;
}
void navAudioLightningWarning(unsigned epoch) {
    if(epoch!=generation() || !setting(Setting::LightningWarning)) return;
    // Device initialization belongs to bootstrap, never a native bolt callback.
    Command command{}; command.kind=CommandKind::Lightning; command.generation=epoch; command.serial=lightningCancel.load(); commands.tryPush(command);
}
void navAudioCancelLightningWarning() {
    lightningCancel.fetch_add(1);
}
void navAudioOverdriveBeep(float frequency,unsigned epoch) {
    navAudioTimingBeep(frequency,epoch,Setting::OverdriveCues);
}
void navAudioTimingBeep(float frequency,unsigned epoch,Setting gate) {
    if((gate!=Setting::OverdriveCues && gate!=Setting::GagazetCues) || epoch!=generation() || !setting(gate) || !std::isfinite(frequency) || frequency<100 || frequency>4000) return;
    Command command{}; command.kind=CommandKind::Overdrive; command.generation=epoch; command.serial=timingSerial(gate); command.frequency=frequency;command.timingGate=gate;command.stamp=GetTickCount64();commands.tryPush(command);
}
void navAudioCancelGagazetBeep() {gagazetCancel.fetch_add(1);}
void navAudioCancelOverdriveBeep() {
    overdriveCancel.fetch_add(1);
}
}
extern "C" void __cdecl FfxNative_PositionalCue(float pan,float proximity,int active,unsigned epoch) {
    ffx::publishPositionalCue(pan,proximity,active!=0,epoch);
}
extern "C" unsigned __cdecl FfxNative_Generation() { return ffx::generation(); }
