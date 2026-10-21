#pragma once
#include "runtime.hpp"
#include "copied_controls.hpp"
#include <array>
namespace ffx {
enum class CopiedTextKind { Arena, ReadDetail, IntroControls, IntroWarning };
struct CopiedTextPacket : CopiedControls {
    CopiedTextKind kind=CopiedTextKind::Arena;
    unsigned epoch=0,revision=0;
    bool interrupt=true,automatic=false;
    // Title, focused text, displayed instructions/details. No native pointers.
    std::array<std::array<uint8_t,4096>,3> text{};
    std::array<unsigned,3> length{};
};
enum class CopiedHelpKind { Begin, Part, Complete, ReadDetail };
struct CopiedHelpPacket : CopiedControls {
    CopiedHelpKind kind=CopiedHelpKind::Begin;
    unsigned epoch=0,revision=0,serial=0,batch=0,parts=0,length=0;
    std::array<uint8_t,4096> text{};
};
bool startCopiedText(HMODULE);
void prepareCopiedControls(CopiedTextPacket&);
void prepareCopiedText(CopiedTextPacket&);
bool commitCopiedText(const CopiedTextPacket&);
void resetCopiedTextDetail();
bool readCopiedTextDetail();
// One ready rendered part per native callback; the worker alone joins pages.
unsigned resetCopiedHelpCapture();
void prepareCopiedHelp(CopiedHelpPacket&);
bool commitCopiedHelp(const CopiedHelpPacket&);
#ifdef FFX_READER_TEST
void drainCopiedTextForTest();
#endif
}
