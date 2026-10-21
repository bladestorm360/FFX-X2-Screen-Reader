#include "../runtime.hpp"
#include "../log_history.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
void check(bool ok,const char* why) { if(!ok) { fprintf(stderr,"FAIL: %s\n",why); exit(1); } }
void save(const std::wstring& path,const char* text,unsigned age=0) {
    FILE* file=nullptr; _wfopen_s(&file,path.c_str(),L"wb"); check(file!=nullptr,"fixture log creation");
    fputs(text,file); fclose(file);
    if(age) {
        auto handle=CreateFileW(path.c_str(),FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        check(handle!=INVALID_HANDLE_VALUE,"fixture timestamp handle");
        FILETIME time{age,0}; check(SetFileTime(handle,nullptr,nullptr,&time)!=FALSE,"fixture timestamp"); CloseHandle(handle);
    }
}
std::string contents(const std::wstring& path) {
    FILE* file=nullptr; _wfopen_s(&file,path.c_str(),L"rb"); check(file!=nullptr,"fixture log read");
    std::string text; int c; while((c=fgetc(file))!=EOF) text+=static_cast<char>(c); fclose(file); return text;
}
std::vector<std::wstring> files(const std::wstring& folder,const wchar_t* pattern) {
    std::vector<std::wstring> result; WIN32_FIND_DATAW found{};
    auto search=FindFirstFileW((folder+pattern).c_str(),&found);
    if(search==INVALID_HANDLE_VALUE) return result;
    do { if(!(found.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) result.push_back(folder+found.cFileName); } while(FindNextFileW(search,&found));
    FindClose(search); return result;
}
}
int main() {
    wchar_t module[MAX_PATH]{}; check(GetModuleFileNameW(nullptr,module,MAX_PATH)!=0,"fixture module directory");
    std::wstring root=module; root.resize(root.find_last_of(L"\\")+1);
    root+=L"log-history-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"\\";
    check(CreateDirectoryW(root.c_str(),nullptr)!=FALSE,"isolated log fixture directory");
    const auto history=root+L"logs\\";
    check(CreateDirectoryW(history.c_str(),nullptr)!=FALSE,"isolated history directory");
    for(unsigned i=0;i<22;++i) {
        wchar_t name[80]{}; swprintf_s(name,L"FFX_SR_Native-20261001-000000-000-%03u.log",i);
        save(history+name,("older "+std::to_string(i)).c_str(),i+1);
    }
    save(history+L"notes.log","foreign file"); save(history+L"FFX_SR_Native-important.log","foreign similar name");
    save(history+L"FFX_Launcher_SR-20261001-000000-000-000.log","launcher archive");
    const auto latest=root+L"FFX_SR_Native.log";
    save(latest,"previous session\nall lines\n");
    auto file=ffx::openLogWithHistory(root,L"FFX_SR_Native.log"); check(file!=nullptr,"rotated latest opened");
    fputs("new session\n",file); fclose(file);
    const auto archives=files(history,L"FFX_SR_Native-????????-??????-???-???.log");
    check(archives.size()==19 && files(history,L"FFX_Launcher_SR-????????-??????-???-???.log").size()==1,"retains twenty older reader logs in total");
    unsigned previous=0; for(const auto& path:archives) if(contents(path)=="previous session\nall lines\n") ++previous;
    check(previous==1 && contents(latest)=="new session\r\n","old log preserved byte-for-byte and latest remains beside DLL");
    check(GetFileAttributesW((history+L"FFX_SR_Native-20261001-000000-000-000.log").c_str())==INVALID_FILE_ATTRIBUTES,"oldest log removed");
    check(GetFileAttributesW((history+L"FFX_SR_Native-20261001-000000-000-004.log").c_str())!=INVALID_FILE_ATTRIBUTES,"newer old log retained");
    check(contents(history+L"notes.log")=="foreign file" && contents(history+L"FFX_SR_Native-important.log")=="foreign similar name","unrelated files untouched");
    check(contents(history+L"FFX_Launcher_SR-20261001-000000-000-000.log")=="launcher archive","recent launcher history retained within shared limit");
    // Multiple startups in one timestamp must use unique archive names.
    for(unsigned i=0;i<3;++i) { file=ffx::openLogWithHistory(root,L"FFX_SR_Native.log"); check(file!=nullptr,"rapid restart"); fputs(("restart "+std::to_string(i)).c_str(),file); fclose(file); }
    check(files(history,L"FFX_SR_Native-????????-??????-???-???.log").size()+files(history,L"FFX_Launcher_SR-????????-??????-???-???.log").size()==20,"rapid restarts keep shared retention cap");
    const auto blocked=root+L"blocked\\"; check(CreateDirectoryW(blocked.c_str(),nullptr)!=FALSE,"blocked fixture directory");
    save(blocked+L"logs","blocks history directory"); save(blocked+L"FFX_SR_Native.log","must survive\n");
    file=ffx::openLogWithHistory(blocked,L"FFX_SR_Native.log"); check(file!=nullptr,"append fallback opened"); fputs("after failure\n",file); fclose(file);
    auto text=contents(blocked+L"FFX_SR_Native.log"); check(text.find("must survive\n")==0 && text.find("after failure\r\n")!=std::string::npos,"archive failure never truncates old log");
    // Delete only explicit files created in this uniquely named fixture folder.
    for(const auto& path:files(history,L"*")) check(DeleteFileW(path.c_str())!=FALSE,"fixture archive cleanup");
    check(RemoveDirectoryW(history.c_str())!=FALSE,"fixture history cleanup");
    check(DeleteFileW((blocked+L"logs").c_str())!=FALSE,"fixture blocker cleanup");
    check(DeleteFileW((blocked+L"FFX_SR_Native.log").c_str())!=FALSE,"fixture append cleanup");
    check(RemoveDirectoryW(blocked.c_str())!=FALSE,"fixture blocked directory cleanup");
    check(DeleteFileW(latest.c_str())!=FALSE && RemoveDirectoryW(root.c_str())!=FALSE,"fixture root cleanup");
    puts("PASS: previous log preservation, latest location, twenty-log retention, oldest removal, rapid restarts and append fallback.");
}
