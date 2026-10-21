#pragma once
#include "../jecht_caption.hpp"
namespace ffx {
inline JechtCaption testJechtCaption;
inline JechtCaptionState testJechtCaptionState;
JechtCaption captureJechtCaption(unsigned window) noexcept {return window==0?testJechtCaption:JechtCaption{};}
JechtCaptionState jechtCaptionState(const JechtCaption&) noexcept {return testJechtCaptionState;}
}
