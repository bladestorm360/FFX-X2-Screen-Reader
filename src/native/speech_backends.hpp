#pragma once
#include "runtime.hpp"
#include <vector>
#include <array>
namespace ffx {
struct SpeechBackendChoice { int preference; std::wstring name; };
struct SpeechBackendSnapshot {
    int count=0;unsigned ready=0;
    struct Row {int preference=0;wchar_t name[128]{};};
    std::array<Row,16> rows{};wchar_t status[128]{};
};
bool copySpeechBackendSnapshot(SpeechBackendSnapshot&); // Bounded read-only Native facts.
std::vector<SpeechBackendChoice> speechBackendChoices();
std::wstring speechBackendStatus();
void selectSpeechBackend(int preference);
}
