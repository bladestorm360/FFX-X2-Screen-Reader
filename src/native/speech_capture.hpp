#pragma once
#include "runtime.hpp"
#include <cstddef>

namespace ffx::speechq {
constexpr unsigned EventLimit=1024, CharacterLimit=512*1024;
enum Flags : unsigned { Interrupt=1, Mod=2, Voiced=4, Protected=8, Deferred=16, Detail=32, Repeat=64, Control=128 };
enum ControlKind : unsigned { None, TutorialEnd, GenerationRecord };
struct Header {
    unsigned flags, generation, cancellation, modal, tutorial, budgetEpoch, sequence, length, control, context;
    DWORD owner;
    ULONGLONG queuedAt;
    char producer[64];
    bool charged;
};
struct Utterance { std::wstring text; Header header; };
// Native producer: one bounded pass over owned storage; no heap, mutex or wait.
bool capture(Header header,const wchar_t* text,size_t length);
bool captureParts(Header,const wchar_t* prefix,size_t prefixLength,const wchar_t* text,size_t length);
// One preparation worker owns pop and all string construction.
bool pop(Header& header,std::wstring& text);
bool reserve(Header& header,unsigned length);
void release(const Header& header) noexcept;
unsigned nextBudgetEpoch() noexcept;
unsigned currentBudgetEpoch() noexcept;
bool capturesPending() noexcept;
}
