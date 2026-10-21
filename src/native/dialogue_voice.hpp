#pragma once
#include <string>
namespace ffx {
namespace speechq {struct Header;}
void submitDialogueOwned(std::wstring,const speechq::Header&,const char* producer,bool explicitMovie=false,bool movie=false); // Preparation owner only.
void installDialogueVoice();
void resetDialogueVoice();
bool voicedDialogue();
bool voicedDialogueAt(unsigned epoch);
void submitDialogue(const std::wstring&,const char* producer,bool movieVoiced);
void submitDialogue(const std::wstring&,const char* producer);
}
