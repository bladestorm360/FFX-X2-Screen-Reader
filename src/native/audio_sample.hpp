#pragma once
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <new>

namespace ffx::audio {
// Adapted from the user's Bravely Default native reader, nav_sample.h.
// Decode supplied mono PCM once, before opening SDL. The callback owns no files,
// resources, allocations or game objects. Two seconds is the bounded asset limit.
struct Sample {
    float pcm[96000]{};
    int count=0;
    bool decode(const uint8_t* bytes,size_t size) {
        count=0;
        if(!bytes || size<12 || std::memcmp(bytes,"RIFF",4) || std::memcmp(bytes+8,"WAVE",4)) return false;
        uint16_t format=0,channels=0,bits=0;
        uint32_t rate=0;
        const uint8_t* data=nullptr; size_t length=0;
        for(size_t offset=12;offset+8<=size;) {
            uint32_t chunk=0; std::memcpy(&chunk,bytes+offset+4,4);
            if(chunk>size-offset-8) return false;
            const auto* body=bytes+offset+8;
            if(!std::memcmp(bytes+offset,"fmt ",4) && chunk>=16) {
                std::memcpy(&format,body,2); std::memcpy(&channels,body+2,2);
                std::memcpy(&rate,body+4,4); std::memcpy(&bits,body+14,2);
            } else if(!std::memcmp(bytes+offset,"data",4)) { data=body; length=chunk; }
            offset+=8+chunk+(chunk&1);
        }
        if(format!=1 || channels!=1 || bits!=16 || !rate || !data || length<2 || (length&1)) return false;
        const size_t frames=length/2;
        const uint64_t output=static_cast<uint64_t>(frames)*48000/rate;
        if(!output || output>std::size(pcm)) return false;
        count=static_cast<int>(output);
        for(int i=0;i<count;++i) {
            const double at=double(i)*rate/48000;
            const auto first=std::min(static_cast<size_t>(at),frames-1),second=std::min(first+1,frames-1);
            int16_t a=0,b=0; std::memcpy(&a,data+2*first,2); std::memcpy(&b,data+2*second,2);
            pcm[i]=(a+(b-a)*static_cast<float>(at-first))/32768.f;
        }
        return true;
    }
    bool load(HMODULE module,int id) {
        const auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
        const auto handle=resource?LoadResource(module,resource):nullptr;
        return handle && decode(static_cast<const uint8_t*>(LockResource(handle)),SizeofResource(module,resource));
    }
    bool loadFile(const wchar_t* path) {
        // Decode a replaceable asset once on the bootstrap worker. Never read
        // a file or allocate memory in the SDL callback.
        const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE) return false;
        const auto size=GetFileSize(file,nullptr);
        bool valid=false;
        if(size>=44 && size<=256*1024) {
            auto bytes=new(std::nothrow) uint8_t[size]; DWORD obtained=0;
            if(bytes && ReadFile(file,bytes,size,&obtained,nullptr) && obtained==size) valid=decode(bytes,size);
            delete[] bytes;
        }
        CloseHandle(file); return valid;
    }
    float read(float at) const {
        const int index=static_cast<int>(at);
        if(index<0 || index>=count) return 0;
        return pcm[index]+(pcm[std::min(index+1,count-1)]-pcm[index])*(at-index);
    }
};
}
