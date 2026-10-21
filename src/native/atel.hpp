#pragma once
#include <cstdint>
#include <vector>
#include <limits>
#include <string>

namespace ffx {
constexpr int UnknownLiteral = std::numeric_limits<int>::min();
struct AtelWorker { unsigned header = 0; std::vector<unsigned> entries, jumps; std::vector<int> ints; };
struct AtelScript { std::vector<uint8_t> code; std::vector<AtelWorker> workers;
    std::string name; unsigned checksum = 0,codeLength=0; std::vector<uint8_t> instructionStarts; };
struct Warp { unsigned worker, pc, call; int room, spawn; };
struct Request { int worker, fn; };
struct AtelResult { bool exit = false, request = false, unresolved = false, truncated = false, interactive = false;
    int room = UnknownLiteral, spawn = UnknownLiteral; unsigned steps = 0;
    std::vector<Warp> warps; std::vector<Request> targets; };
AtelResult scanWorker(const AtelScript& script, unsigned index, const std::vector<unsigned>* entries = nullptr);
AtelScript readScript(uintptr_t chunk);
std::vector<uint8_t> atelInstructionStarts(const std::vector<uint8_t>& code);
}
