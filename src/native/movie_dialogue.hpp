#pragma once
namespace ffx {
void installMovieDialogue(bool captionsOnly=false);
void resetMovieDialogue();
bool movieDialogueVoiced();
bool movieDialogueVoicedAt(unsigned epoch);
}
