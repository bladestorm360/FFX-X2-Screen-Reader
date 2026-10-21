// Decode bounded copied bytes as strict UTF-8, then ACP on invalid UTF-8.
// This is separate from native game glyph decoding.
// See ../../Docs/features/runtime/index.md#native-text-and-localization.
#include "ready_text.hpp"
namespace ffx {
std::wstring decodeReadyAscii(const uint8_t* source,unsigned length) {
    if(!source || !length || length>0x10000) return {};
    const auto* bytes=reinterpret_cast<const char*>(source);
    const int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes,length,nullptr,0);
    const auto codepage=size?CP_UTF8:CP_ACP;
    const int count=size?size:MultiByteToWideChar(codepage,0,bytes,length,nullptr,0);
    if(!count) return {};
    std::wstring result(count,0);MultiByteToWideChar(codepage,0,bytes,length,result.data(),count);return result;
}
}
