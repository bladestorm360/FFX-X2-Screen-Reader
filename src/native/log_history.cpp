#include "runtime.hpp"
#include "log_history.hpp"
#include <algorithm>
#include <share.h>
namespace ffx {
namespace {
struct Archive { std::wstring path; FILETIME written; };
bool historyName(const std::wstring& name,const std::wstring& prefix) {
    if(name.compare(0,prefix.size(),prefix)!=0) return false;
    const auto suffix=name.substr(prefix.size());
    if(suffix.size()!=27 || suffix.substr(23)!=L".log") return false;
    for(size_t i=0;i<23;++i) {
        if(i==8 || i==15 || i==19) { if(suffix[i]!=L'-') return false; }
        else if(suffix[i]<L'0' || suffix[i]>L'9') return false;
    }
    return true;
}
void prune(const std::wstring& folder,const std::wstring& prefix) {
    WIN32_FIND_DATAW entry{};
    const auto search=FindFirstFileW((folder+L"*.log").c_str(),&entry);
    if(search==INVALID_HANDLE_VALUE) return;
    std::vector<Archive> files;
    do {
        const bool owned=historyName(entry.cFileName,prefix) || historyName(entry.cFileName,L"FFX_SR_Native-") || historyName(entry.cFileName,L"FFX_Launcher_SR-");
        if(!(entry.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) && owned)
            files.push_back({folder+entry.cFileName,entry.ftLastWriteTime});
    } while(FindNextFileW(search,&entry));
    FindClose(search);
    std::sort(files.begin(),files.end(),[](const Archive& a,const Archive& b) {
        const auto order=CompareFileTime(&a.written,&b.written);
        return order?order>0:a.path>b.path;
    });
    for(size_t i=20;i<files.size();++i) DeleteFileW(files[i].path.c_str());
}
}
FILE* openLogWithHistory(const std::wstring& directory,const wchar_t* name) {
    const std::wstring filename=name?name:L"";
    if(filename.size()<5 || filename.find_first_of(L"\\/")!=std::wstring::npos || filename.substr(filename.size()-4)!=L".log") return nullptr;
    const auto latest=directory+filename,folder=directory+L"logs\\";
    const auto prefix=filename.substr(0,filename.size()-4)+L"-";
    bool preserved=true;
    const auto attributes=GetFileAttributesW(latest.c_str());
    if(attributes!=INVALID_FILE_ATTRIBUTES) {
        if(attributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)) return nullptr;
        preserved=false;
        if(CreateDirectoryW((directory+L"logs").c_str(),nullptr) || GetLastError()==ERROR_ALREADY_EXISTS) {
            SYSTEMTIME now{}; GetLocalTime(&now);
            for(unsigned collision=0;collision<1000;++collision) {
                wchar_t suffix[64]{};
                swprintf_s(suffix,L"%04u%02u%02u-%02u%02u%02u-%03u-%03u.log",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,now.wMilliseconds,collision);
                if(MoveFileW(latest.c_str(),(folder+prefix+suffix).c_str())) { preserved=true; break; }
                const auto error=GetLastError();
                if(error!=ERROR_ALREADY_EXISTS && error!=ERROR_FILE_EXISTS) break;
            }
        }
    } else if(GetLastError()!=ERROR_FILE_NOT_FOUND) preserved=false;
    // If an old log cannot be archived, append or fail opening; never truncate
    // the only surviving copy. Rotation runs once at startup, off game hooks.
    auto file=_wfsopen(latest.c_str(),preserved?L"w":L"a",_SH_DENYWR);
    if(preserved && file) prune(folder,prefix);
    if(file && !preserved) { fputs("Log history archive failed; existing log preserved by appending.\n",file); fflush(file); }
    return file;
}
}
