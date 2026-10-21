#pragma once
#include "speech_domains.hpp"
namespace ffx {
struct SelectionDetailToken {unsigned epoch=0,serial=0;};
void installSelectionDetails(); // Bootstrap before native readers publish.
SelectionDetailToken currentSelectionDetailToken(); // Native copied view, no text ownership.
void appendSelectionDetailOwned(std::wstring,SelectionDetailToken); // Preparation only.
SelectionDetailToken beginSelectionDetail(); // Serialized native owner: POD only.
// Speech preparation owner only, for producer formatting from owned packets.
void emitSelectionDetailOwned(std::wstring,SelectionDetailToken,bool automatic,const speechq::Header&);
}
