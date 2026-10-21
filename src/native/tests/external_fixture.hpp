#pragma once
// Original test adapter only. Game implementations are separately supplied local
// evidence outside every Git tree; never generate/copy their bytes into sources.
// Match the production Windows boundary even when this header is the first
// include in an offline host; SDK min/max macros must not corrupt STL calls.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <cstdio>

namespace ffx_fixture {
inline std::filesystem::path repositoryRoot() {
    auto root=std::filesystem::weakly_canonical(std::filesystem::path(__FILE__));
    for(unsigned i=0;i<4;++i) root=root.parent_path();
    if(!std::filesystem::exists(root/L"src"/L"native"/L"tests"/L"external_fixture.hpp"))
        throw std::runtime_error("The fixture's separately supplied mod-source root is unavailable");
    return root;
}
inline std::filesystem::path evidenceRoot() {
    wchar_t value[32768]{};
    const auto length=GetEnvironmentVariableW(L"FFX_EVIDENCE_ROOT",value,32768);
    if(length>=32768) throw std::runtime_error("FFX_EVIDENCE_ROOT exceeds the supported path length");
    std::filesystem::path root;
    if(length) root=value;
    else {
        // Canonical test builds pass absolute source paths, so __FILE__ stays
        // valid even when the executable runs in an external fixture directory.
        root=repositoryRoot().parent_path()/L"Analysis"/L"FFX"/L"RepositoryEvidence";
    }
    root=std::filesystem::weakly_canonical(root);
    for(auto p=root;!p.empty();p=p.parent_path()) {
        if(std::filesystem::exists(p/L".git") ||
            (std::filesystem::is_regular_file(p/L"HEAD") && std::filesystem::is_directory(p/L"objects")))
            throw std::runtime_error("Game fixture input root must be outside every Git tree");
        if(p==p.parent_path()) break;
    }
    return root;
}

// Keep the actual C array type/size: existing replay templates, relocations and
// sizeof contracts must retain their original semantics after externalization.
// Bind adapters to a named Bytes owner, then its .bytes member; replay inputs
// must outlive static initialization and all later fixture setup/stack reuse.
template<size_t N> struct Bytes {unsigned char bytes[N]{};};
template<size_t N> Bytes<N> load(const char* relative,uint64_t expectedHash) {
    const std::filesystem::path name(relative);
    if(name.is_absolute()) throw std::runtime_error("Fixture name must be relative to the external evidence root");
    for(const auto& part:name) if(part==L"..") throw std::runtime_error("Fixture name cannot escape its external evidence root");
    const auto file=evidenceRoot()/name;
    std::ifstream input(file,std::ios::binary);
    Bytes<N> result;
    if(!input.read(reinterpret_cast<char*>(result.bytes),N) || input.peek()!=std::char_traits<char>::eof()) {
        std::fprintf(stderr,"Missing/wrong-sized external game fixture: %s\n",file.string().c_str());
        throw std::runtime_error("Supply the separately retained local game evidence; no installed-game read is attempted");
    }
    // FNV-1a detects wrong local inputs without a new crypto/runtime dependency.
    // The external migration manifest separately retains SHA256 provenance.
    uint64_t hash=14695981039346656037ull;
    for(const auto byte:result.bytes) {hash^=byte;hash*=1099511628211ull;}
    if(hash!=expectedHash) throw std::runtime_error("External game fixture fingerprint does not match its qualified original");
    return result;
}
}
