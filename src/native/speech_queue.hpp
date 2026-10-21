#pragma once
#include "speech_capture.hpp"
namespace ffx::speechq {
bool start(HMODULE module,HANDLE backendWake);
bool output(const wchar_t* text,size_t length,bool interrupt,const char* producer,bool voiced,unsigned epoch);
bool take(Utterance& item);
bool pending() noexcept;
bool current(const Header& header) noexcept;
unsigned generation() noexcept;
unsigned silenceRequest() noexcept;
unsigned silenceGeneration() noexcept;
void protect(bool value) noexcept;
void tutorial(bool value);
void modal(bool value) noexcept;
void interrupt() noexcept;
void repeat();
void invalidate(const char* event);
}
