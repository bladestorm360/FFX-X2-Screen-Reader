#pragma once
#include "controller_model.hpp"
#include "runtime.hpp"
namespace ffx {
void installController();
void initializeControllerBindings(); // Bootstrap load before hook publication.
void pumpController();
bool controllerOwned();
void drainController();
int controllerBinding(int command);
const wchar_t* controllerControlNameLiteral(int control); // Immutable ready label, no formatting.
std::wstring controllerControlName(int control);
void learnControllerBinding(int command);
bool learningControllerBinding();
void cancelControllerBinding();
void resetControllerBindings();
#ifdef FFX_READER_TEST
void setControllerWriteSinkForTest(bool(*)(const std::wstring&,const int*,size_t));
#endif
void dispatchHotkey(unsigned vk,bool shift=false,bool control=false);
// Native input owner queues a mod command for the same post-update boundary
// used by keyboard commands. The guard is rechecked before capturing game data.
bool queueControllerHotkey(unsigned vk,bool shift,bool control,bool(*guard)());
void repeatSpeech(bool);
bool consumeKey(unsigned vk,bool shift,bool control=false);
bool capturesKey(unsigned vk,bool shift=false,bool control=false);
}
