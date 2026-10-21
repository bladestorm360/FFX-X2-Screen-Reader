// Port of navigation/atel.js. Worker-local tables and literal ownership are
// preserved. Fahrenheit and Karifean/FFXDataParser are layout references.
#include "atel.hpp"
#include <set>
#include <string>
#include <stdexcept>

namespace ffx {
std::vector<uint8_t> atelInstructionStarts(const std::vector<uint8_t>& code) {
    std::vector<uint8_t> starts((code.size()+7)/8);
    for(unsigned pc=0;pc<code.size();) {
        const unsigned size=code[pc]&0x80?3:1;
        if(pc+size>code.size()) throw std::runtime_error("Truncated ATEL instruction");
        starts[pc/8]|=static_cast<uint8_t>(1u<<(pc%8)); pc+=size;
    }
    return starts;
}
AtelResult scanWorker(const AtelScript& script, unsigned index, const std::vector<unsigned>* entries) {
    struct Instruction { unsigned op, arg, next; };
    const auto localStarts=script.instructionStarts.empty()?atelInstructionStarts(script.code):std::vector<uint8_t>{};
    const auto& starts=script.instructionStarts.empty()?localStarts:script.instructionStarts;
    auto validPC=[&](unsigned pc) { return pc<script.code.size() && pc/8<starts.size() && (starts[pc/8]&(1u<<(pc%8))); };
    AtelResult result;
    if(script.code.empty() && script.codeLength) { result.unresolved=true; return result; }
    if (index >= script.workers.size()) return result;
    struct Path { unsigned worker, pc; };
    std::vector<Path> pending;
    auto enqueue = [&](unsigned worker, unsigned pc) {
        if (validPC(pc)) pending.push_back({worker, pc}); else result.unresolved = true;
    };
    if (entries) {
        for (auto entry : *entries) if (entry < script.workers[index].entries.size()) enqueue(index, script.workers[index].entries[entry]);
    } else for (auto entry : script.workers[index].entries) enqueue(index, entry);
    std::set<std::string> visited, sites;
    const std::set<unsigned> returns{0x34,0x3c,0x3d,0x3e,0x3f,0x40,0x54};
    const std::set<unsigned> requests{0x36,0x37,0x38,0x39,0x3a,0x3b,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x77,0x78,0x79};
    while (!pending.empty() && result.steps < 200000) {
        const auto path = pending.back(); pending.pop_back();
        const auto& worker = script.workers[path.worker];
        unsigned pc = path.pc;
        std::vector<int> literals;
        while (validPC(pc) && result.steps < 200000) {
            auto key = std::to_string(path.worker) + ":" + std::to_string(pc);
            for (auto literal : literals) key += ":" + std::to_string(literal);
            if (!visited.insert(key).second) break;
            ++result.steps;
            const unsigned op=script.code[pc],size=op&0x80?3:1;
            const unsigned arg=size==3?script.code[pc+1]|(script.code[pc+2]<<8):0;
            const Instruction ins{op,arg,pc+size};
            if (op == 0xae || op == 0xad) {
                literals.push_back(op == 0xae ? static_cast<int16_t>(arg) : arg < worker.ints.size() ? worker.ints[arg] : UnknownLiteral);
                if (literals.size() > 8) literals.erase(literals.begin());
            } else if (op == 0xb5 || op == 0xd8) {
                if (arg == 0x64 || arg == 0x136 || arg == 0x13b || arg == 0x15b || arg == 0x15f || arg == 0x1a7 || arg == 0x401d || arg == 0x225 || arg==0x7002) result.interactive = true;
                if (arg == 0x11 || arg == 0x10b) {
                    result.exit = true;
                    const int room = literals.size() >= 2 ? literals[literals.size() - 2] : UnknownLiteral;
                    const int spawn = literals.size() >= 2 ? literals.back() : UnknownLiteral;
                    auto site = std::to_string(path.worker) + ":" + std::to_string(pc) + ":" + std::to_string(room) + ":" + std::to_string(spawn);
                    if (sites.insert(site).second) result.warps.push_back({path.worker, pc, arg, room, spawn});
                }
                literals.clear();
            } else if (op == 0xb3) {
                if (arg < script.workers.size() && !script.workers[arg].entries.empty()) enqueue(arg, script.workers[arg].entries[0]);
                else result.unresolved = true;
                literals.clear();
            } else if (op == 0xb0 || op == 0xd5 || op == 0xb1 || op == 0xb2 || op == 0xd6 || op == 0xd7) {
                if (arg < worker.jumps.size()) enqueue(path.worker, worker.jumps[arg]); else result.unresolved = true;
                literals.clear(); if (op == 0xb0 || op == 0xd5) break;
            } else if (returns.count(op)) break;
            else if (requests.count(op)) {
                result.request = true;
                if (literals.size() >= 3) {
                    auto target = literals[literals.size() - 2], fn = literals.back();
                    if (target != UnknownLiteral && fn != UnknownLiteral) result.targets.push_back({target, fn});
                }
                literals.clear();
            } else if (op != 0) literals.clear();
            pc = ins.next;
        }
    }
    result.truncated = !pending.empty() || result.steps >= 200000;
    std::set<int> rooms, spawns;
    bool known = !result.warps.empty();
    for (auto warp : result.warps) {
        if (warp.room == UnknownLiteral || warp.spawn == UnknownLiteral || warp.room < 0) known = false;
        rooms.insert(warp.room); spawns.insert(warp.spawn);
    }
    if (known && !result.truncated && !result.unresolved && rooms.size() == 1) {
        result.room = *rooms.begin(); if (spawns.size() == 1) result.spawn = *spawns.begin();
    }
    return result;
}
}
