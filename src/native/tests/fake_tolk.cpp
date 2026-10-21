#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mutex>
#include <string>
#include <vector>
struct Record { std::wstring text; DWORD thread; bool interrupt; bool braille; };
static std::mutex guard;
static std::vector<Record> records;
struct Silence { DWORD thread; int outputCount; };
static std::vector<Silence> silences;
static bool brailleAvailable=true;
static HANDLE blocked=nullptr,release=nullptr;
extern "C" void __cdecl Tolk_Load() {}
extern "C" void __cdecl Tolk_TrySAPI(bool) {}
extern "C" bool __cdecl Tolk_IsLoaded() { return true; }
extern "C" bool __cdecl Tolk_Output(const wchar_t* text, bool interrupt) {
    if(blocked && release) { SetEvent(blocked); WaitForSingleObject(release,INFINITE); }
    std::lock_guard<std::mutex> lock(guard);
    records.push_back({text, GetCurrentThreadId(), interrupt,false}); return true;
}
extern "C" bool __cdecl Tolk_Silence() {
    std::lock_guard<std::mutex> lock(guard);
    silences.push_back({GetCurrentThreadId(),static_cast<int>(records.size())}); return true;
}
extern "C" int __cdecl Test_SilenceCount() { std::lock_guard<std::mutex> lock(guard); return static_cast<int>(silences.size()); }
extern "C" DWORD __cdecl Test_SilenceThread() { std::lock_guard<std::mutex> lock(guard); return silences.empty()?0:silences.back().thread; }
extern "C" int __cdecl Test_SilenceOutputCount() { std::lock_guard<std::mutex> lock(guard); return silences.empty()?-1:silences.back().outputCount; }
extern "C" void __cdecl Test_Block(HANDLE started,HANDLE resume) { blocked=started; release=resume; }
extern "C" bool __cdecl Tolk_Braille(const wchar_t* text) {
    std::lock_guard<std::mutex> lock(guard);
    if(!brailleAvailable) return false;
    records.push_back({text, GetCurrentThreadId(), false,true}); return true;
}
extern "C" void __cdecl Test_BrailleAvailable(bool value) {
    std::lock_guard<std::mutex> lock(guard); brailleAvailable=value;
}
extern "C" bool __cdecl Test_Braille(int index) {
    std::lock_guard<std::mutex> lock(guard);
    return index>=0 && index<static_cast<int>(records.size()) && records[index].braille;
}
extern "C" bool __cdecl Test_Interrupt(int index) {
    std::lock_guard<std::mutex> lock(guard);
    return index>=0 && index<static_cast<int>(records.size()) && records[index].interrupt;
}
extern "C" int __cdecl Test_Count() {
    std::lock_guard<std::mutex> lock(guard); return static_cast<int>(records.size());
}
extern "C" DWORD __cdecl Test_Read(int index, wchar_t* text, unsigned length) {
    std::lock_guard<std::mutex> lock(guard);
    if (index < 0 || index >= static_cast<int>(records.size())) return 0;
    wcsncpy_s(text, length, records[index].text.c_str(), _TRUNCATE); return records[index].thread;
}
