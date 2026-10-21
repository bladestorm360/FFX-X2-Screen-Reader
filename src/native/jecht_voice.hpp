#pragma once
namespace ffx {
namespace jecht_voice_rva {
inline constexpr unsigned Start=0x30d970,Events=0x30f3f0,Dispose=0x30e930;
inline constexpr unsigned dependencies[]={Start,Events,Dispose,0x307b40,0x548c9a};
}
struct JechtVoiceState {unsigned serial=0;bool ready=false,playing=false,voiced=false;};
unsigned jechtVoiceSerial(unsigned sequence) noexcept;
JechtVoiceState jechtVoiceState(unsigned sequence,unsigned epoch) noexcept;
void installJechtVoice();
}
