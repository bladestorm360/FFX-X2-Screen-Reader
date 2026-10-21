#pragma once
#include "../runtime.hpp"

// Fake semantic sink only. This fixture adapter does not model the native
// ready-wide queue path and provides no native-heap or performance evidence.
namespace ffx {
bool output(const wchar_t* text,bool interrupt,const char* producer) {
    return output(std::wstring(text ? text : L""),interrupt,producer);
}
}
