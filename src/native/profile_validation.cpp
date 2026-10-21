#include "profile.hpp"

// Offline deployment check in a separate process: map executable sections,
// without resolving imports, running entry points, starting speech or hooks.
extern "C" unsigned __cdecl FfxNative_ValidateExecutable(const wchar_t* path) {
    HMODULE image=nullptr;
    try {
    if(!path || !ffx::configureNativeProfileForPath(path)) return 1;
    image=LoadLibraryExW(path,nullptr,DONT_RESOLVE_DLL_REFERENCES);
    if(!image) return 2;
    const auto base=reinterpret_cast<uintptr_t>(image);
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS32 nt{};
    const bool headers=ffx::copy(base,&dos,sizeof(dos)) && dos.e_magic==IMAGE_DOS_SIGNATURE &&
        ffx::copy(base+dos.e_lfanew,&nt,sizeof(nt)) && nt.Signature==IMAGE_NT_SIGNATURE &&
        nt.FileHeader.Machine==IMAGE_FILE_MACHINE_I386 && nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    // Executable mappings retain absolute operands at their preferred base.
    // Refuse any unexpected mapping rather than reinterpret data references.
    const bool preferred=headers && base==nt.OptionalHeader.ImageBase;
    unsigned result=4;
    if(preferred) result=!ffx::resolveNativeExtensionsAt(base,nt.OptionalHeader.SizeOfImage)?3:
        !ffx::validateNativeDependencies()?5:0;
    FreeLibrary(image);
    return result;
    } catch(const std::exception& error) {
        if(image) FreeLibrary(image);
        ffx::recordLog("native validation refused: %s",error.what()); return 6;
    } catch(...) {
        if(image) FreeLibrary(image);
        ffx::recordLog("native validation refused: unexpected exception"); return 6;
    }
}
