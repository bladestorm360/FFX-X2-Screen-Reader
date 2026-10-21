#include "profile.hpp"
#include "large_address_image.hpp"
#include <wincrypt.h>

namespace ffx {
namespace {
bool fileDigest(const wchar_t* path,std::string& result,int laaMode) {
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    pe::LaaHeader laa;
    if(laaMode) {
        BYTE header[4096]{}; DWORD length=0;
        if(!ReadFile(file,header,sizeof(header),&length,nullptr) || !pe::inspectLaa(header,length,laa) ||
            SetFilePointer(file,0,nullptr,FILE_BEGIN)==INVALID_SET_FILE_POINTER) { CloseHandle(file); return false; }
    }
    unsigned long long position=0;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    bool ok = CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) != 0;
    if (ok) ok = CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash) != 0;
    BYTE buffer[65536]; DWORD count = 0;
    while (ok) {
        if (!ReadFile(file, buffer, sizeof(buffer), &count, nullptr)) { ok = false; break; }
        if (!count) break;
        if(laaMode && laa.characteristicsOffset>=position && laa.characteristicsOffset<position+count) {
            auto& flags=buffer[static_cast<size_t>(laa.characteristicsOffset-position)];
            flags=laaMode>0?static_cast<BYTE>(flags|0x20):static_cast<BYTE>(flags&~0x20);
        }
        position+=count;
        ok = CryptHashData(hash, buffer, count, 0) != 0;
    }
    BYTE digest[32]; DWORD length = sizeof(digest);
    if (ok) ok = CryptGetHashParam(hash, HP_HASHVAL, digest, &length, 0) && length == sizeof(digest);
    if (ok) {
        static const char hex[] = "0123456789abcdef";
        result.clear(); result.reserve(64);
        for (auto value : digest) { result += hex[value >> 4]; result += hex[value & 15]; }
    }
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider, 0);
    CloseHandle(file);
    return ok;
}
}
bool nativeFileDigest(const wchar_t* path,std::string& result) { return fileDigest(path,result,0); }
bool configureNativeProfileForPath(const wchar_t* path) {
    std::string digest;
    if (!nativeFileDigest(path,digest)) { recordLog("native profile refused: cannot hash executable"); return false; }
    if (digest == "3d13e5ed53821df8db204cd3c27d470c5c220b18e16849fc4440996c15f277e1") {
        nativeProfile = NativeProfile::Reference;
        recordLog("native address profile=reference sha256=%s", digest.c_str()); return true;
    }
    if (digest == "0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d") {
        nativeProfile = NativeProfile::Installed;
        recordLog("native address profile=installed sha256=%s", digest.c_str()); return true;
    }
    if (digest == "18e1b899a091a1a21f0732eab8a49f7067e5347cb442dc35d5910bb23652d123") {
        nativeProfile = NativeProfile::MovieCaptions;
        recordLog("native address profile=Will captions sha256=%s", digest.c_str()); return true;
    }
    // The launcher may change exactly IMAGE_FILE_LARGE_ADDRESS_AWARE. Preserve
    // exact image identity: every other byte must match a verified FFX profile.
    std::string normalized;
    if(fileDigest(path,normalized,-1) && normalized=="3d13e5ed53821df8db204cd3c27d470c5c220b18e16849fc4440996c15f277e1") {
        nativeProfile=NativeProfile::Reference; recordLog("native address profile=reference flag-only LAA normalization"); return true;
    }
    if(fileDigest(path,normalized,1) && normalized=="0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d") {
        nativeProfile=NativeProfile::Installed; recordLog("native address profile=installed flag-only LAA normalization"); return true;
    }
    recordLog("native profile refused: unsupported executable sha256=%s", digest.c_str());
    return false;
}
bool configureNativeProfile() {
    wchar_t path[MAX_PATH]{};
    return GetModuleFileNameW(nullptr,path,MAX_PATH) && configureNativeProfileForPath(path);
}
}
