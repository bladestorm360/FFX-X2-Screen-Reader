#pragma once
#include "events.hpp"
namespace ffx {
// Choice speech must remain accessible while this menu captures game speech.
inline constexpr char WakkaChoiceProducer[]="mod-overdrive-wakka-choice";
namespace wakka_rva {
inline constexpr unsigned EffectId=0x388eb0,GlobalBegin=0x388e20,GlobalTable=0xd33360;
}
void installWakka();
void resetWakka();
bool wakkaMenuActive();
void wakkaMenuKey(unsigned vk);
#ifdef FFX_READER_TEST
void testWakkaModule(uintptr_t base,const wchar_t* path,int(*invoke)());
int testWakkaControl();
int testWakkaBegin(unsigned actor);
uintptr_t testWakkaLoadedModule(uintptr_t candidate);
#endif
}
