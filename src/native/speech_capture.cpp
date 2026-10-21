#include "speech_capture.hpp"
#include <array>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>

namespace ffx::speechq {
namespace {
constexpr unsigned Slots=2048, BlockSize=512, Blocks=4096, BudgetBanks=32;
constexpr unsigned MaxBlocks=CharacterLimit/BlockSize;
constexpr unsigned Free=0, Writing=1, Ready=2;
constexpr unsigned Initializing=std::numeric_limits<unsigned>::max();
static_assert(std::atomic<unsigned>::is_always_lock_free,"speech capture atomics must not wait");
struct Bank {
    std::atomic<unsigned> epoch{0}, leases{0}, events{0}, characters{0};
};
struct Record { Header header; unsigned short blocks[MaxBlocks]; };
struct Store {
    std::array<std::atomic<unsigned>,Slots/32> slotBits;
    std::array<std::atomic<unsigned>,Blocks/32> blockBits;
    std::array<std::atomic<unsigned>,Slots> states{};
    std::array<Record,Slots> records;
    wchar_t text[Blocks][BlockSize];
    std::atomic<unsigned> slotHint{0},blockHint{0},serial{1},pending{0};
    // Only small control arrays are initialized; large POD storage has no
    // constructors, allocation or eager producer initialization.
    Store() {
        for(auto& bits:slotBits) bits.store(~0u,std::memory_order_relaxed);
        for(auto& bits:blockBits) bits.store(~0u,std::memory_order_relaxed);
    }
} store;
std::array<Bank,BudgetBanks> banks;
std::atomic<unsigned> budgetEpoch{1};
unsigned firstBit(unsigned mask) noexcept {
    unsigned bit=0;while((mask&1u)==0) {mask>>=1;++bit;}return bit;
}
bool older(unsigned left,unsigned right) noexcept {return static_cast<int>(left-right)<0;}
bool copyReady(wchar_t* destination,const wchar_t* source,size_t length) noexcept {
#if defined(_MSC_VER) && defined(_WIN32)
    __try {std::memcpy(destination,source,length*sizeof(wchar_t));return true;}
    __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
#else
    std::memcpy(destination,source,length*sizeof(wchar_t));return true;
#endif
}
bool prepareBank(unsigned epoch) noexcept {
    if(epoch==Initializing) return false;
    auto& bank=banks[epoch%BudgetBanks];
    auto previous=bank.epoch.load(std::memory_order_acquire);
    if(previous==epoch) return true;
    if(previous==Initializing || bank.leases.load()!=0 || bank.events.load(std::memory_order_acquire)!=0 ||
        bank.characters.load(std::memory_order_acquire)!=0) return false;
    if(!bank.epoch.compare_exchange_strong(previous,Initializing)) return false;
    // Close the lease-registration/reuse race. A producer registers before
    // verifying the epoch; a late old-epoch producer sees Initializing/new
    // and rejects. An already registered producer prevents reuse.
    if(bank.leases.load()!=0) {bank.epoch.store(previous);return false;}
    bank.epoch.store(epoch);
    return true;
}
int slot() noexcept {
    const auto start=store.slotHint.fetch_add(1,std::memory_order_relaxed);
    for(unsigned offset=0;offset<store.slotBits.size();++offset) {
        const auto word=(start+offset)%store.slotBits.size();
        auto mask=store.slotBits[word].load(std::memory_order_acquire);
        if(!mask) continue;
        const unsigned bit=firstBit(mask),claim=1u<<bit;
        if(store.slotBits[word].compare_exchange_strong(mask,mask&~claim,std::memory_order_acq_rel))
            return static_cast<int>(word*32+bit);
    }
    return -1;
}
void freeBlocks(const Record& record,unsigned count) noexcept {
    // Group releases by bitmap word: owned POD retirement, never a string
    // destructor, allocation or a wait. Producer uses this only on rejection.
    std::array<unsigned,Blocks/32> released{};
    for(unsigned i=0;i<count;++i) released[record.blocks[i]/32]|=1u<<(record.blocks[i]%32);
    for(unsigned word=0;word<released.size();++word)
        if(released[word]) store.blockBits[word].fetch_or(released[word],std::memory_order_release);
}
void freeSlot(unsigned index) noexcept {
    store.states[index].store(Free,std::memory_order_release);
    store.slotBits[index/32].fetch_or(1u<<(index%32),std::memory_order_release);
}
unsigned claimBlocks(Record& record,unsigned needed) noexcept {
    unsigned claimed=0;
    const auto start=store.blockHint.fetch_add(1,std::memory_order_relaxed);
    for(unsigned offset=0;offset<store.blockBits.size() && claimed<needed;++offset) {
        const auto word=(start+offset)%store.blockBits.size();
        auto mask=store.blockBits[word].load(std::memory_order_acquire);
        unsigned take=0,available=mask;
        unsigned count=0;
        while(available && count<needed-claimed) {
            const unsigned bit=firstBit(available),claim=1u<<bit;
            take|=claim;available&=~claim;++count;
        }
        if(!take || !store.blockBits[word].compare_exchange_strong(mask,mask&~take,std::memory_order_acq_rel)) continue;
        while(take) {
            const unsigned bit=firstBit(take);take&=~(1u<<bit);
            record.blocks[claimed++]=static_cast<unsigned short>(word*32+bit);
        }
    }
    return claimed;
}
}
unsigned currentBudgetEpoch() noexcept {return budgetEpoch.load(std::memory_order_acquire);}
unsigned nextBudgetEpoch() noexcept {return budgetEpoch.fetch_add(1,std::memory_order_acq_rel)+1;}
bool reserve(Header& header,unsigned length) {
    header.charged=false;
    if(length>CharacterLimit || !prepareBank(header.budgetEpoch)) return false;
    auto& bank=banks[header.budgetEpoch%BudgetBanks];
    bank.leases.fetch_add(1);
    if(bank.epoch.load()!=header.budgetEpoch) {bank.leases.fetch_sub(1);return false;}
    if(bank.events.fetch_add(1,std::memory_order_acq_rel)>=EventLimit) {
        bank.events.fetch_sub(1,std::memory_order_release);bank.leases.fetch_sub(1);return false;
    }
    if(bank.characters.fetch_add(length,std::memory_order_acq_rel)>CharacterLimit-length) {
        bank.characters.fetch_sub(length,std::memory_order_release);
        bank.events.fetch_sub(1,std::memory_order_release);bank.leases.fetch_sub(1);return false;
    }
    header.length=length;header.charged=true;return true;
}
void release(const Header& header) noexcept {
    if(!header.charged) return;
    auto& bank=banks[header.budgetEpoch%BudgetBanks];
    // A bank cannot be reused while any reserved event remains.
    bank.characters.fetch_sub(header.length,std::memory_order_release);
    bank.events.fetch_sub(1,std::memory_order_release);
    bank.leases.fetch_sub(1);
}
bool capture(Header header,const wchar_t* text,size_t length) {return captureParts(header,nullptr,0,text,length);}
bool captureParts(Header header,const wchar_t* prefix,size_t prefixLength,const wchar_t* text,size_t textLength) {
    if(prefixLength>CharacterLimit || textLength>CharacterLimit-prefixLength ||
        (prefixLength && !prefix) || (textLength && !text)) return false;
    const auto length=prefixLength+textLength;
    const bool charged=(header.flags&(Deferred|Repeat|Control))==0;
    header.charged=false;header.length=static_cast<unsigned>(length);
    if(charged && !reserve(header,static_cast<unsigned>(length))) return false;
    const int taken=slot();
    if(taken<0) {release(header);return false;}
    const auto index=static_cast<unsigned>(taken);
    store.states[index].store(Writing,std::memory_order_release);
    header.sequence=store.serial.fetch_add(1,std::memory_order_acq_rel);
    auto& record=store.records[index];record.header=header;
    const auto needed=static_cast<unsigned>((length+BlockSize-1)/BlockSize);
    const auto claimed=claimBlocks(record,needed);
    if(claimed!=needed) {freeBlocks(record,claimed);freeSlot(index);release(header);return false;}
    for(unsigned i=0;i<needed;++i) {
        const auto offset=static_cast<size_t>(i)*BlockSize;
        const auto count=(length-offset)<BlockSize?length-offset:BlockSize;
        const auto first=offset<prefixLength?std::min(count,prefixLength-offset):0;
        const auto rest=count-first;
        auto* destination=store.text[record.blocks[i]];
        if((first && !copyReady(destination,reinterpret_cast<const wchar_t*>(reinterpret_cast<const unsigned char*>(prefix)+offset*sizeof(wchar_t)),first)) ||
            (rest && !copyReady(destination+first,text+(offset+first-prefixLength),rest))) {
            freeBlocks(record,claimed);freeSlot(index);release(header);return false;
        }
    }
    store.pending.fetch_add(1,std::memory_order_release);
    store.states[index].store(Ready,std::memory_order_release);
    return true;
}
bool capturesPending() noexcept {return store.pending.load(std::memory_order_acquire)!=0;}
bool pop(Header& header,std::wstring& text) {
    // Snapshot the ticket ceiling before scanning. Writers publish Writing
    // before taking a ticket; an unfinished earlier writer is an ordering
    // barrier. Tickets taken during the scan belong to the next pass.
    const unsigned ceiling=store.serial.load(std::memory_order_acquire);
    int selected=-1;unsigned first=0;
    for(unsigned index=0;index<Slots;++index) {
        const auto state=store.states[index].load(std::memory_order_acquire);
        if(state==Writing) return false;
        if(state!=Ready) continue;
        const auto sequence=store.records[index].header.sequence;
        if(!older(sequence,ceiling)) continue;
        if(selected<0 || older(sequence,first)) {selected=static_cast<int>(index);first=sequence;}
    }
    if(selected<0) return false;
    const auto index=static_cast<unsigned>(selected);auto& record=store.records[index];
    header=record.header;
    const unsigned count=(header.length+BlockSize-1)/BlockSize;
    try {
        text.resize(header.length);
        for(unsigned i=0;i<count;++i) {
            const auto offset=static_cast<size_t>(i)*BlockSize;
            const auto length=(header.length-offset)<BlockSize?header.length-offset:BlockSize;
            std::memcpy(&text[offset],store.text[record.blocks[i]],length*sizeof(wchar_t));
        }
    } catch(...) {
        freeBlocks(record,count);freeSlot(index);store.pending.fetch_sub(1,std::memory_order_release);
        release(header);throw;
    }
    freeBlocks(record,count);freeSlot(index);store.pending.fetch_sub(1,std::memory_order_release);
    return true;
}
}
