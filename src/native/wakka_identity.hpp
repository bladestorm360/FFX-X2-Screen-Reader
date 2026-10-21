#pragma once
#include "runtime.hpp"
#include <array>
namespace ffx {
struct WakkaIdentity {
    uintptr_t base=0,table=0;
    unsigned epoch=0,serial=0;
    std::array<char,65> digest{};
#ifdef FFX_READER_TEST
    std::array<wchar_t,32768> testPath{};
#endif
};
bool startWakkaIdentity();
bool requestWakkaIdentity(const WakkaIdentity&) noexcept;
bool readWakkaIdentity(WakkaIdentity&) noexcept;
void cancelWakkaIdentity() noexcept;
#ifdef FFX_READER_TEST
void wakkaIdentityBeforeFileForTest();
#endif
}
