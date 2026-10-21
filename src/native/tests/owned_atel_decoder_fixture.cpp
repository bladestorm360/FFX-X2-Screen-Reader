#include "../atel.hpp"
#include "../owned_atel_decoder.hpp"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace { std::vector<uint8_t> source; }
namespace ffx {
bool copy(uintptr_t address, void* destination, size_t length) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(source.data());
    if (address < base || address - base > source.size() || length > source.size() - (address - base)) return false;
    std::memcpy(destination, reinterpret_cast<const void*>(address), length);
    return true;
}
}
static void put(unsigned offset, uint32_t value) { std::memcpy(source.data() + offset, &value, 4); }
static void put16(unsigned offset, uint16_t value) { std::memcpy(source.data() + offset, &value, 2); }
static void put(std::vector<uint8_t>& bytes, unsigned offset, uint32_t value) { std::memcpy(bytes.data() + offset, &value, 4); }
static void put16(std::vector<uint8_t>& bytes, unsigned offset, uint16_t value) { std::memcpy(bytes.data() + offset, &value, 2); }
static void check(bool ok, const char* what) { if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); } }
static bool rejects(const std::vector<uint8_t>& input) {
    try { (void)ffx::decodeOwnedAtelScript(input.data(), static_cast<unsigned>(input.size())); return false; }
    catch (const std::runtime_error&) { return true; }
}
static bool equal(const ffx::AtelScript& a, const ffx::AtelScript& b) {
    if (a.code != b.code || a.name != b.name || a.checksum != b.checksum || a.codeLength != b.codeLength ||
        a.instructionStarts != b.instructionStarts || a.workers.size() != b.workers.size()) return false;
    for (size_t i = 0; i < a.workers.size(); ++i) {
        const auto& x = a.workers[i]; const auto& y = b.workers[i];
        if (x.header != y.header || x.entries != y.entries || x.jumps != y.jumps || x.ints != y.ints) return false;
    }
    return true;
}
static void compareAsset(const char* relativePath, const char* label) {
    std::ifstream file(relativePath, std::ios::binary);
    check(file.good(), "retained EBP asset open");
    const std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    check(raw.size() >= 0x30, "retained EBP header span");
    auto rawU32 = [&](size_t offset) {
        check(offset <= raw.size() && 4 <= raw.size() - offset, "retained EBP offset read");
        uint32_t value; std::memcpy(&value, raw.data() + offset, 4); return value;
    };
    const unsigned start = rawU32(4);
    unsigned end = static_cast<unsigned>(raw.size());
    for (unsigned i = 1; i < 11; ++i) {
        const unsigned value = rawU32(4 + i * 4);
        if (value && value >= start && value <= raw.size()) { end = value; break; }
    }
    check(start < end && end <= raw.size(), "retained primary EBP chunk bounds");
    source.assign(raw.begin() + start, raw.begin() + end);
    const unsigned codeLength = static_cast<unsigned>(source[0] | (uint32_t(source[1]) << 8) | (uint32_t(source[2]) << 16) | (uint32_t(source[3]) << 24));
    const unsigned codeOffset = [&] { uint32_t value; std::memcpy(&value, source.data() + 0x30, 4); return value; }();
    const unsigned nameOffset = [&] { uint32_t value; std::memcpy(&value, source.data() + 0xc, 4); return value; }();
    check(nameOffset <= source.size() && 8 <= source.size() - nameOffset, "retained identity name bounds");
    check(codeOffset <= source.size() && codeLength <= source.size() - codeOffset, "retained identity code bounds");
    ffx::AtelScript identity; identity.codeLength = codeLength; identity.checksum = 2166136261u;
    char scriptName[9]{}; std::memcpy(scriptName, source.data() + nameOffset, 8); identity.name = scriptName;
    for (unsigned i = 0; i < codeLength; ++i) identity.checksum = (identity.checksum ^ source[codeOffset + i]) * 16777619u;
    const bool identityCatalog = ffx::navigationCatalog(identity) != nullptr;
    ffx::AtelScript expected, actual;
    std::string expectedError, actualError;
    try { expected = ffx::readScript(reinterpret_cast<uintptr_t>(source.data())); }
    catch (const std::exception& error) { expectedError = error.what(); }
    try { actual = ffx::decodeOwnedAtelScript(source.data(), static_cast<unsigned>(source.size())); }
    catch (const std::exception& error) { actualError = error.what(); }
    check(expectedError == actualError, "retained asset decoder rejection parity");
    if (!expectedError.empty()) {
        std::printf("%s span=%u name=%s codeLength=%u checksum=%08X codeEmptyIfCataloged=%d identityCatalog=%d rejected by both: %s\n",
            label, end - start, identity.name.c_str(), codeLength, identity.checksum, codeLength > 0x100000 ? 1 : 0,
            identityCatalog ? 1 : 0, expectedError.c_str());
    } else {
        check(equal(expected, actual), "retained asset decoder parity");
        size_t tableBytes = 0;
        for (const auto& worker : actual.workers)
            tableBytes += 4 * (worker.entries.size() + worker.jumps.size() + worker.ints.size());
        if (std::string(label) == "bltz0000") {
            check(actual.codeLength > 0x100000 && actual.code.empty() && ffx::navigationCatalog(actual), "large Blitz code-empty catalog behavior");
            check(tableBytes == 672552 && tableBytes <= 0x200000, "Blitz table byte count and budget");
        }
        std::printf("%s span=%u name=%s codeLength=%u checksum=%08X workers=%zu tableBytes=%zu codeEmpty=%d catalog=%d\n",
            label, end - start, actual.name.c_str(), actual.codeLength, actual.checksum, actual.workers.size(),
            tableBytes, actual.code.empty() ? 1 : 0, ffx::navigationCatalog(actual) ? 1 : 0);
    }
}
int main() {
    source.assign(256, 0);
    put(0, 4); put(0xc, 0x90); put(0x30, 0x80); put16(0x34, 1); put(0x38, 0x40);
    std::memcpy(source.data() + 0x90, "fixture", 7);
    source[0x80] = 0x80; source[0x81] = 0x34; source[0x82] = 0x12; source[0x83] = 0x34;
    put16(0x44, 1); put16(0x48, 1); put16(0x4a, 1);
    put(0x58, 0xc0); put(0x60, 0xc4); put(0x64, 0xc8);
    put(0xc0, 0xfffffffeu); put(0xc4, 2); put(0xc8, 3);
    const auto expected = ffx::readScript(reinterpret_cast<uintptr_t>(source.data()));
    const auto actual = ffx::decodeOwnedAtelScript(source.data(), static_cast<unsigned>(source.size()));
    check(equal(expected, actual), "owned decoder parity with address adapter");
    check(ffx::scanWorker(expected, 0).steps == ffx::scanWorker(actual, 0).steps, "worker scan parity");

    const auto synthetic = source;
    compareAsset("..\\..\\Docs\\Scripts\\Offline\\output\\FFX\\20261003_audit\\files\\ffx_ps2\\ffx\\master\\jppc\\event\\obj\\bl\\bltz0000\\bltz0000.ebp", "bltz0000");
    compareAsset("..\\..\\Docs\\Scripts\\Offline\\output\\FFX\\20261003_audit\\files\\ffx_ps2\\ffx\\master\\jppc\\event\\obj\\na\\nagi0000\\nagi0000.ebp", "nagi0000");
    source = synthetic;
    auto bad = source; put(bad, 0x30, 0xfffffff0u); check(rejects(bad), "code outside exact span");
    bad = source; put(bad, 0xc, 252); check(rejects(bad), "name outside exact span");
    bad = source; put(bad, 0x38, 250); check(rejects(bad), "worker header outside exact span");
    bad = source; put(bad, 0x60, 255); check(rejects(bad), "entry table truncated");
    bad = source; put16(bad, 0x34, 1025); check(rejects(bad), "oversized worker count");
    bad = source; put16(bad, 0x48, 4097); check(rejects(bad), "truncated table span with oversized table count");
    bad = source; put(bad, 0x60, 255); check(rejects(bad), "entry table outside exact span");
    std::vector<uint8_t> budget(0x30000, 0);
    auto budgetPut = [&](unsigned offset, uint32_t value) { std::memcpy(budget.data() + offset, &value, 4); };
    auto budgetPut16 = [&](unsigned offset, uint16_t value) { std::memcpy(budget.data() + offset, &value, 2); };
    budgetPut(0, 4); budgetPut(0xc, 0x12000); budgetPut(0x30, 0x10000); budgetPut16(0x34, 43);
    std::memcpy(budget.data() + 0x12000, "budget", 6);
    budget[0x10000] = 0x34;
    for (unsigned i = 0; i < 43; ++i) {
        const unsigned header = 0x200 + i * 0x30;
        budgetPut(0x38 + i * 4, header);
        budgetPut16(header + 4, 4096); budgetPut16(header + 8, 4096); budgetPut16(header + 0xa, 4096);
        budgetPut(header + 0x18, 0x1000); budgetPut(header + 0x20, 0x5000); budgetPut(header + 0x24, 0x9000);
    }
    try { (void)ffx::decodeOwnedAtelScript(budget.data(), static_cast<unsigned>(budget.size())); check(false, "copied-table budget"); }
    catch (const std::runtime_error& error) { check(std::strstr(error.what(), "copied tables exceed") != nullptr, "copied-table budget reason"); }
    bad = source; bad[0x80] = 0x80; bad[0x81] = 1; bad[0x82] = 0; put(bad, 0, 2); check(rejects(bad), "truncated instruction");
    std::puts("owned ATEL decoder fixture passed");
}
