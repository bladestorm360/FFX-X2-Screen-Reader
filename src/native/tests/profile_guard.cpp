#include "../profile.hpp"
#include <cstdio>

namespace ffx {
void log(const char*, ...) {}
}
int main() {
    // Hash this real fixture executable with the production Win32 routine.
    // It must be refused before native hook installation can begin.
    if (ffx::configureNativeProfile()) {
        std::fprintf(stderr, "Unsupported executable passed the native profile guard.\n");
        return 1;
    }
    if (ffx::nativeProfile != ffx::NativeProfile::Reference) return 1;
    std::puts("Native executable hash guard passed: unsupported fixture refused.");
    return 0;
}
