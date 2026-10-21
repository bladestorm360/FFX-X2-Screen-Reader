#pragma once

#include "../copied_controls.hpp"

namespace ffx {
void prepareCopiedControlSnapshot(CopiedControls& controls) {
    controls = CopiedControls{};
    controls.bindings.fill(128);
    controls.polish = textEncodingSnapshot() == 1;
}

OwnedTextContext copiedControlContext(const CopiedControls& controls) {
    OwnedTextContext context;
    context.polish = controls.polish;
    return context;
}

int textEncodingSnapshot() { return 0; }

std::vector<std::uint8_t> encodeMovieCaption(const wchar_t* text) {
    // Use the production decoder's verified single-byte table to select only
    // ordinary one-byte caption characters. The fixture strings are ASCII.
    std::vector<std::uint8_t> encoded;
    for (; *text; ++text) {
        bool found = false;
        for (unsigned byte = 0x30; byte <= 0xff; ++byte) {
            const auto candidate = static_cast<std::uint8_t>(byte);
            const auto decoded = decodeOwnedBytes(&candidate, 1, OwnedTextMode::Literal, OwnedTextContext{});
            if (!decoded.empty() && decoded.front().size() == 1 && decoded.front()[0] == *text) {
                encoded.push_back(candidate);
                found = true;
                break;
            }
        }
        if (!found) return {};
    }
    encoded.push_back(0);
    return encoded;
}
}
