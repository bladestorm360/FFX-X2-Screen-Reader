#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace ffx {
constexpr unsigned MainMenu = 1;
uintptr_t address(unsigned rva);
bool copy(uintptr_t source, void* destination, size_t length);
// Whole ready captures with a separately proven physical/typed extent. Keep
// ordinary read/copy's64KiB cap; these bounded chunks are copied in one native
// observation, never accumulated across input events or read on a worker.
inline bool copyReadyRegion(uintptr_t source,void* destination,size_t length,size_t extent) {
    if(!destination || source<0x10000 || length>extent || extent>0x800000 || source>UINTPTR_MAX-extent) return false;
    auto* target=static_cast<uint8_t*>(destination);
    for(size_t at=0;at<length;) {
        const auto remaining=length-at;const size_t bytes=remaining<0x10000?remaining:0x10000;
        if(!copy(source+at,target+at,bytes)) return false;at+=bytes;
    }
    return true;
}
template<class T> T read(uintptr_t source, T fallback = {}) {
    T result;
    return copy(source, &result, sizeof(result)) ? result : fallback;
}
using ButtonGlyphReader=std::wstring(*)(uint8_t);
void setButtonGlyphReader(ButtonGlyphReader);
std::wstring buttonGlyph(uint8_t);
std::wstring nativeBindingName(unsigned action,bool controller);
std::wstring nativeButtonMaskName(uint16_t mask);
uint8_t equippedItem(unsigned character,bool armor);
std::wstring equippedItemName(unsigned character,bool armor);
enum class TextMode { Terminated, Records, LegacyRecords, Literal };
std::vector<std::wstring> decode(uintptr_t source, size_t length = 256, TextMode mode = TextMode::Terminated);
std::wstring playerName(int character);
int textEncodingSnapshot();
bool outputAt(const std::wstring&,bool,const char*,unsigned epoch);
void prepareSpeechStartup();
void cancelSpeechStartup();
bool startSpeech(HMODULE module, const wchar_t* logName = L"FFX_SR_Native.log");
bool output(const wchar_t* text, bool interrupt, const char* producer);
bool output(const std::wstring& text, bool interrupt, const char* producer);
bool outputDialogue(const std::wstring& text, bool interrupt, const char* producer, bool voiced);
void invalidate(const char* event);
void captureSpeech(bool);
void protectSpeech(bool);
void protectTutorialSpeech(bool);
void interruptSpeechForInput();
void repeatSpeech(bool);
unsigned generation();
unsigned startReader();
}

extern "C" unsigned __cdecl FfxNative_Features();
extern "C" int __cdecl FfxNative_Output(const wchar_t* text, int interrupt);
extern "C" int __cdecl FfxNative_SpeechReady();

#include "log_capture.hpp"
