#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

int wmain(int argc,wchar_t** argv) {
    if(argc!=3) { fputs("Usage: profile_preflight <staged proxy> <game executable>\n",stderr); return 1; }
    const auto proxy=LoadLibraryW(argv[1]);
    if(!proxy) { fprintf(stderr,"Staged proxy cannot load; Win32 error=%lu\n",GetLastError()); return 1; }
    using Validate=unsigned(__cdecl*)(const wchar_t*);
    const auto validate=reinterpret_cast<Validate>(GetProcAddress(proxy,"FfxNative_ValidateExecutable"));
    if(!validate) { fputs("Staged proxy lacks the mandatory executable validation export.\n",stderr); return 1; }
    const auto result=validate(argv[2]);
    if(result) {
        fprintf(stderr,"Native deployment refused: %s (code=%u). No game code was executed.\n",
            result==1?"unsupported executable hash":result==2?"executable mapping failed":
            result==3?"compiled extension signatures or operands do not match":
            result==5?"compiled feature dependency is unverified":result==6?"validation exception":"unexpected executable image mapping",result);
        return 1;
    }
    puts("Compiled native profile/signatures/operands and feature dependencies verified; executable entry point, speech and hooks were not run.");
    return 0;
}
