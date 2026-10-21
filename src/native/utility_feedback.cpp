// Send the same ready reply to console and central output. Raw wchar input is
// guarded and bounded before either consumer is called.
// See ../../Docs/features/runtime/index.md#output-and-text.
#include "utilities.hpp"
#include "speech_capture.hpp"
namespace ffx {
namespace {
bool readyLength(const wchar_t* text,size_t& length) noexcept {
    length=0;if(!text) return false;
    __try {
        for(;length<=speechq::CharacterLimit;++length) if(!text[length]) return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}
void replyReady(const wchar_t* text,size_t length) {
    consoleReplyReady(text,length);
    output(text,true,"utility-reply");
}
}
void utilityReply(const std::wstring& text) {replyReady(text.c_str(),text.size());}
void utilityReply(const wchar_t* text) {
    size_t length=0;
    if(readyLength(text,length)) replyReady(text,length);
}
}
