#pragma once
namespace ffx {
// Copied native request identity, never an engine pointer. Only exact original
// Winno sound/show producers create it; other messages receive an empty token.
struct JechtCaption {unsigned epoch=0,sequence=0,baseline=0,requestedAt=0;};
struct JechtCaptionState {bool ready=true,voiced=false;};
JechtCaption captureJechtCaption(unsigned window) noexcept;
JechtCaptionState jechtCaptionState(const JechtCaption&) noexcept;
}
