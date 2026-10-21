#pragma once
#include "../utilities.hpp"
// Semantic fixture only: this fake string sink does not qualify Native heap
// behavior. Held production capture fixtures use the actual feedback adapter.
namespace ffx {
void utilityReply(const wchar_t* text) {utilityReply(std::wstring(text?text:L""));}
}
