#include "external_fixture.hpp"
#include <type_traits>
namespace {
const auto retainedStorage=ffx_fixture::load<29>("native/airshipCountBody.bin",0xda0631fd212972e6ull);
const auto& retainedBody=retainedStorage.bytes;
}
template<size_t N> size_t arraySize(const unsigned char (&)[N]) {return N;}
int main() try {
    // The original airship body is separately supplied external evidence. Its
    // type/length/fingerprint, not its implementation, belongs in this host.
    const auto storage=ffx_fixture::load<29>("native/airshipCountBody.bin",0xda0631fd212972e6ull);
    const auto& body=storage.bytes;
    static_assert(sizeof(body)==29,"externalization preserves sizeof on C arrays");
    if(arraySize(body)!=29) return 1;
    bool rejected=false;
    try {ffx_fixture::load<28>("native/airshipCountBody.bin",0);} catch(const std::runtime_error&) {rejected=true;}
    if(!rejected) return 2;
    rejected=false;
    try {ffx_fixture::load<29>("native/airshipCountBody.bin",1);} catch(const std::runtime_error&) {rejected=true;}
    if(!rejected) return 3;
    rejected=false;
    try {ffx_fixture::load<29>("../escape.bin",1);} catch(const std::runtime_error&) {rejected=true;}
    if(!rejected) return 4;
    uint64_t retainedHash=14695981039346656037ull;
    for(const auto byte:retainedBody) retainedHash=(retainedHash^byte)*1099511628211ull;
    if(retainedHash!=0xda0631fd212972e6ull || std::memcmp(body,retainedBody,sizeof(body))) return 6;
    std::puts("PASS: external fixture C-array lifetime/type/size, original fingerprint, wrong size/fingerprint and path escape rejection.");
} catch(const std::exception& e) {std::fprintf(stderr,"FAIL: %s\n",e.what());return 5;}
