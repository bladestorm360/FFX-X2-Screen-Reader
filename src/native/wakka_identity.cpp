#include "wakka_identity.hpp"
#include "nonblocking_snapshots.hpp"
#include "profile.hpp"
#include <atomic>
#include <cstring>
namespace ffx {
namespace {
async::TryCaptureQueue<WakkaIdentity,8> requests;
async::TryCaptureQueue<WakkaIdentity,8> results;
std::atomic<unsigned> latest{0};
HANDLE wake=nullptr;
bool started=false;
DWORD WINAPI worker(void*) {
    WakkaIdentity request,reply;bool pending=false;
    for(;;) {
        if(pending && (reply.epoch!=generation() || reply.serial!=latest.load())) pending=false;
        if(pending && results.tryPush(reply)) pending=false;
        while(!pending && requests.tryPop(request)) {
            if(request.epoch!=generation() || request.serial!=latest.load()) continue;
#ifdef FFX_READER_TEST
            wakkaIdentityBeforeFileForTest();
#endif
            wchar_t path[32768]{};
#ifdef FFX_READER_TEST
            if(request.testPath[0]) memcpy(path,request.testPath.data(),sizeof(path));else
#endif
            {const auto length=GetModuleFileNameW(reinterpret_cast<HMODULE>(request.base),path,32768);
                if(!length || length>=32768) path[0]=0;}
            std::string digest;
            reply=request;reply.digest.fill(0);
            if(path[0] && nativeFileDigest(path,digest) && digest.size()==64)
                memcpy(reply.digest.data(),digest.c_str(),65);
            if(request.epoch!=generation() || request.serial!=latest.load()) continue;
            if(!results.tryPush(reply)) pending=true;
        }
        WaitForSingleObject(wake,INFINITE);
    }
}
}
bool startWakkaIdentity() {
    if(started) return true;
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&startWakkaIdentity),&pinned)) return false;
    wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!wake) return false;
    const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) return false; // Published event/queues remain process-lifetime.
    CloseHandle(thread);started=true;return true;
}
bool requestWakkaIdentity(const WakkaIdentity& value) noexcept {
    if(!wake) return false;
    latest.store(value.serial,std::memory_order_release);
    const bool captured=requests.tryPush(value);SetEvent(wake);return captured;
}
bool readWakkaIdentity(WakkaIdentity& result) noexcept {
    const bool found=results.tryPop(result);if(wake) SetEvent(wake);return found;
}
void cancelWakkaIdentity() noexcept {latest.store(0,std::memory_order_release);if(wake) SetEvent(wake);}
}
