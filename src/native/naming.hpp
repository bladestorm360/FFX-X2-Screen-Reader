#pragma once
namespace ffx {
void installNaming();
void resetNaming();
bool readEnteredName();
// Read-only availability for input ownership; never queues speech.
bool namingReadbackAvailable();
}
