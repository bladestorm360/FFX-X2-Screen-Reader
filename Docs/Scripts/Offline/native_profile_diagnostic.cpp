// Read-only deployment diagnosis with the exact production resolver.
// Map sections only: never resolve imports or execute game entry points.
#include "../../../src/native/profile.hpp"
#include <cstdio>
namespace ffx {
void log(const char*,...) {}
bool copy(uintptr_t source,void* destination,size_t size) {
    __try {memcpy(destination,reinterpret_cast<void*>(source),size);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
int wmain(int argc,wchar_t** argv) {
    if(argc!=2)return 1;
    ffx::nativeProfile=ffx::NativeProfile::Installed;
    ffx::logCaptureHandler=[](const ffx::LogCapture& packet) noexcept {
        char text[1024]{};packet.formatter(packet,text,sizeof(text));puts(text);
    };
    const auto image=LoadLibraryExW(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);
    if(!image){fprintf(stderr,"Mapping failed %lu\n",GetLastError());return 1;}
    const auto base=reinterpret_cast<uintptr_t>(image);
    const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);
    if(base!=nt->OptionalHeader.ImageBase){FreeLibrary(image);return 1;}
    const bool passed=ffx::resolveNativeExtensionsAt(base,nt->OptionalHeader.SizeOfImage);
    FreeLibrary(image);return passed?0:1;
}
