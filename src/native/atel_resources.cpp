#include "atel_resources.hpp"
#include "events.hpp"
#include "owned_atel_decoder.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include "native_worker_index.hpp"
#include "owned_callback_proof.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <vector>
namespace ffx {
namespace {
constexpr unsigned Loader=0x48d230,Allocator=0x2871c0,FreeResource=0x48d490,AllocationReturn=0x74;
constexpr unsigned Writing=0x80000000u;
constexpr unsigned NativeReceipts=64; // Bounded lifetime transport; no engine maximum inferred.
constexpr unsigned PreparedSlots=NativeReceipts;
constexpr size_t PreparedBudget=64u*1024u*1024u;
struct Parts {unsigned count=0;uintptr_t parent=0;std::array<AtelSpan,AtelResourceParts> spans{};};
struct Capture {Parts parts;unsigned fileBytes=0,receipt=0;uintptr_t parent=0;std::array<uint8_t,AtelResourceLimit> bytes;};
ProcessStorage<std::array<Capture,AtelResourceSlots>> captureStorage;
std::atomic<unsigned> claimed{0},serial{0},rejected{0};
// Capture buffers are transient queue slots; physical receipts remain until
// the originating engine allocation is freed. Reusing a buffer cannot erase
// another still-live file's bounds or metadata identity.
std::array<async::AtomicSnapshot<Parts>,NativeReceipts> nativeSpans;
std::array<std::atomic<uintptr_t>,NativeReceipts> aliveParents{};
std::array<std::atomic<unsigned>,NativeReceipts> rejectedSerial{};
struct Completion {Parts parts;unsigned validMask=0;};
std::array<async::AtomicSnapshot<Completion>,NativeReceipts> completions;
async::TryCaptureQueue<unsigned,AtelResourceSlots> requests;
std::atomic<HANDLE> wake{nullptr};
struct Declaration {unsigned header,low,high,index;};
struct HeaderDeclarations {unsigned header,begin,count;};
struct PreparedPart {
    AtelIdentity identity;AtelScript script;std::atomic<unsigned> state{0}; // 0 pending,1 ready,2 rejected.
    // Catalogued large scripts deliberately omit general navigation scanning,
    // but their original callback guards still require actual owned bytecode.
    std::vector<uint8_t> guardCode;
    std::vector<Declaration> declarations,orderedDeclarations;
    std::vector<HeaderDeclarations> declarationHeaders;
};
struct Prepared {Parts parts;std::array<PreparedPart,AtelResourceParts> decoded;};
ProcessStorage<std::array<Prepared,PreparedSlots>> preparedStorage;
std::array<std::atomic<unsigned>,PreparedSlots> readers{};
std::array<async::AtomicSnapshot<Parts>,PreparedSlots> readyParts;
std::array<std::atomic<unsigned>,PreparedSlots> readySerial{};
std::array<size_t,PreparedSlots> bankBytes{};size_t retainedBytes=0;
unsigned nextPrepared=0;
std::array<AtelPreparedHandler,8> preparedHandlers{};std::atomic<unsigned> preparedHandlerCount{0};
struct Receipt {unsigned token=0,group=0,logical=0,allocated=0;uintptr_t stack=0,parent=0;};
thread_local std::array<Receipt,16> frames{};
thread_local unsigned depth=0,nextToken=0;
unsigned claimCapture() {
    auto observed=claimed.load(std::memory_order_relaxed);
    for(unsigned slot=0;slot<AtelResourceSlots;++slot) if(!(observed&(1u<<slot)))
        if(claimed.compare_exchange_strong(observed,observed|(1u<<slot),std::memory_order_acquire,std::memory_order_relaxed)) return slot;
    return AtelResourceSlots;
}
void releaseCapture(unsigned slot) {claimed.fetch_and(~(1u<<slot),std::memory_order_release);}
unsigned word(const uint8_t* bytes,unsigned at) {unsigned value;std::memcpy(&value,bytes+at,4);return value;}
unsigned claimReceipt(uintptr_t parent) {
    // 1 is a private reservation marker, never a readable Win32 file address.
    // Old metadata at a reused address cannot become alive before the new
    // serial/physical bounds are published.
    for(unsigned i=0;i<NativeReceipts;++i) {
        auto expected=parent;if(aliveParents[i].compare_exchange_strong(expected,1,std::memory_order_acq_rel,std::memory_order_relaxed)) return i;
    }
    for(unsigned i=0;i<NativeReceipts;++i) {
        uintptr_t empty=0;if(aliveParents[i].compare_exchange_strong(empty,1,std::memory_order_acq_rel,std::memory_order_relaxed)) return i;
    }
    return NativeReceipts;
}
void capture(const Receipt& receipt) {
    if(!captureStorage.ready() || !receipt.parent || receipt.logical<0x30 || receipt.logical>AtelResourceLimit ||
       receipt.allocated<receipt.logical || receipt.allocated!=((receipt.logical+0x7ffu) & ~0x7ffu) ||
       receipt.parent>UINTPTR_MAX-receipt.logical) {++rejected;return;}
    const auto slot=claimCapture();if(slot==AtelResourceSlots) {++rejected;return;}
    auto& owned=captureStorage.get()[slot];owned.parts={};owned.parent=receipt.parent;owned.fileBytes=receipt.logical;
    // Complete ready EV file, bounded by the exact logical size in native EDI,
    // not its rounded allocation or any assumed 1/8MiB readable region.
    if(!copyReadyRegion(receipt.parent,owned.bytes.data(),receipt.logical,receipt.logical) || std::memcmp(owned.bytes.data(),"EV01",4)!=0) {releaseCapture(slot);++rejected;return;}
    unsigned token=serial.fetch_add(1,std::memory_order_relaxed)+1;
    if(!token) token=serial.fetch_add(1,std::memory_order_relaxed)+1;
    const unsigned epoch=generation();
    owned.parts.parent=receipt.parent;
    for(unsigned i=0;i<AtelResourceParts;++i) {
        const unsigned start=word(owned.bytes.data(),4+i*4);if(!start) continue;
        // EV01 ends its chunk offsets with the logical file end and FFFFFFFF.
        if(start==receipt.logical || start==UINT32_MAX) break;
        if(start<0x30 || start>=receipt.logical) {owned.parts={};break;}
        unsigned end=receipt.logical;
        for(unsigned j=i+1;j<AtelResourceParts;++j) {
            const unsigned after=word(owned.bytes.data(),4+j*4);if(!after) continue;
            if(after==UINT32_MAX) break;
            if(after<start || after>receipt.logical) {end=0;break;}
            end=after;break;
        }
        if(end<=start) {owned.parts={};break;}
        owned.parts.spans[owned.parts.count++]={receipt.parent+start,epoch,token,end-start};
    }
    if(!owned.parts.count) {releaseCapture(slot);++rejected;return;}
    owned.receipt=claimReceipt(receipt.parent);
    if(owned.receipt==NativeReceipts) {releaseCapture(slot);++rejected;return;}
    nativeSpans[owned.receipt].publish(owned.parts);
    aliveParents[owned.receipt].store(receipt.parent,std::memory_order_release);
    if(!requests.tryPush(slot)) {
        rejectedSerial[owned.receipt].store(token,std::memory_order_release);releaseCapture(slot);++rejected;
        if(const auto event=wake.load(std::memory_order_acquire)) SetEvent(event);
        return;
    }
    if(const auto event=wake.load(std::memory_order_acquire)) SetEvent(event);
}
void workerConstructed(Event&) {invalidateNativeWorkerIndex();}
void retireParent(uintptr_t identity) {
    if(!identity) return;
    for(auto& parent:aliveParents) {
        auto expected=identity;
        parent.compare_exchange_strong(expected,0,std::memory_order_acq_rel,std::memory_order_relaxed);
    }
}
void resourceFreed(Event& e) {
    // Before the original free, so a later allocation at this address cannot
    // be invalidated by a late after-return observer. No engine dereference,
    // metadata container mutation or destructor belongs on this owner.
    retireParent(static_cast<uintptr_t>(e.args[0]));
    invalidateNativeWorkerIndex();
    if(const auto event=wake.load(std::memory_order_acquire)) SetEvent(event);
}
void loaderBefore(Event& e) {
    // The native hook stack grows downward. Discard abandoned loader receipts
    // after exception unwind before accepting a sibling/outer loader.
    while(depth && e.nativeStack && frames[depth-1].stack<=e.nativeStack) --depth;
    if(depth==frames.size()) {++rejected;return;}
    const unsigned token=++nextToken;e.before[0]=token;
    frames[depth++]={token,static_cast<unsigned>(e.args[0]),0,0,e.nativeStack,0};
}
void allocationBefore(Event& e) {
    if(!depth || e.caller!=address(Loader)+AllocationReturn || frames[depth-1].group!=12) return;
    e.before[0]=frames[depth-1].token;e.before[1]=e.args[0];e.before[2]=e.edi;
}
void allocationAfter(Event& e) {
    if(!depth || !e.before[0] || frames[depth-1].token!=static_cast<unsigned>(e.before[0])) return;
    auto& receipt=frames[depth-1];receipt.parent=e.result;
    receipt.allocated=static_cast<unsigned>(e.before[1]);receipt.logical=static_cast<unsigned>(e.before[2]);
    // Even a malformed/over-capacity newly allocated file cannot inherit an
    // old checksum at this address. The new receipt is published after read.
    retireParent(receipt.parent);
}
void loaderAfter(Event& e) {
    if(!e.before[0]) return;
    unsigned found=depth;while(found && frames[found-1].token!=static_cast<unsigned>(e.before[0])) --found;
    if(!found) return;const auto receipt=frames[found-1];depth=found-1;
    if(receipt.group==12 && receipt.parent==e.result) {
        capture(receipt);
        // A malformed/over-capacity new receipt must also wake retained
        // consumers; there may be no accepted item in the resource FIFO.
        if(const auto event=wake.load(std::memory_order_acquire)) SetEvent(event);
    }
}
struct Lease {
    unsigned slot=PreparedSlots,part=0;
    ~Lease() {if(slot<PreparedSlots) readers[slot].fetch_sub(1,std::memory_order_release);}
};
bool acquire(uintptr_t chunk,Lease& lease) {
    if(!preparedStorage.ready()) return false;
    AtelSpan current;if(!nativeAtelSpan(chunk,current)) return false;
    for(unsigned slot=0;slot<PreparedSlots;++slot) {
        if(readySerial[slot].load(std::memory_order_acquire)!=current.serial) continue;
        Parts ready;if(!readyParts[slot].tryRead(ready)) continue;
        for(unsigned i=0;i<ready.count && i<AtelResourceParts;++i) {
            if(ready.spans[i].chunk!=chunk || ready.spans[i].serial!=current.serial) continue;
            const auto count=readers[slot].fetch_add(1,std::memory_order_acquire);
            if(count>=Writing-1) {readers[slot].fetch_sub(1,std::memory_order_release);return false;}
            lease.slot=slot;lease.part=i;
            const auto& actual=preparedStorage.get()[slot].parts;
            return i<actual.count && actual.spans[i].chunk==chunk && actual.spans[i].epoch==ready.spans[i].epoch && actual.spans[i].serial==ready.spans[i].serial;
        }
    }
    return false;
}
void notifyPrepared() noexcept {
    const auto count=preparedHandlerCount.load(std::memory_order_acquire);
    for(unsigned i=0;i<count;++i) preparedHandlers[i]();
}
size_t partBytes(const PreparedPart& part) {
    size_t size=part.script.code.capacity()+part.script.instructionStarts.capacity()+part.script.name.capacity()+part.guardCode.capacity()+
        part.script.workers.capacity()*sizeof(AtelWorker)+
        (part.declarations.capacity()+part.orderedDeclarations.capacity())*sizeof(Declaration)+
        part.declarationHeaders.capacity()*sizeof(HeaderDeclarations);
    for(const auto& worker:part.script.workers)
        size+=(worker.entries.capacity()+worker.jumps.capacity())*sizeof(unsigned)+worker.ints.capacity()*sizeof(int);
    return size;
}
void clearPart(PreparedPart& part) {
    part.script={};std::vector<uint8_t>{}.swap(part.guardCode);std::vector<Declaration>{}.swap(part.declarations);
    std::vector<Declaration>{}.swap(part.orderedDeclarations);std::vector<HeaderDeclarations>{}.swap(part.declarationHeaders);
    part.identity={};part.state.store(0,std::memory_order_relaxed);
}
bool bankAlive(const Prepared& bank) {
    if(!bank.parts.count) return false;
    AtelSpan live;const auto& first=bank.parts.spans[0];
    return nativeAtelSpan(first.chunk,live) && live.serial==first.serial;
}
void collectRetired() {
    for(unsigned at=0;at<PreparedSlots;++at) {
        auto& bank=preparedStorage.get()[at];if(!bank.parts.count || bankAlive(bank)) continue;
        unsigned expected=0;
        if(!readers[at].compare_exchange_strong(expected,Writing,std::memory_order_acquire,std::memory_order_relaxed)) continue;
        readySerial[at].store(0,std::memory_order_release);readyParts[at].publish({});bank.parts={};
        for(auto& part:bank.decoded) clearPart(part);
        retainedBytes-=bankBytes[at];bankBytes[at]=0;
        // Preserve a rejected reader that briefly entered the writing state;
        // its bounded rollback may finish after this publication.
        readers[at].fetch_and(~Writing,std::memory_order_release);
    }
}
unsigned consume(unsigned slot) {
    collectRetired();
    const auto& owned=captureStorage.get()[slot];
    unsigned destination=PreparedSlots;
    for(unsigned i=0;i<PreparedSlots;++i) {
        const auto at=(nextPrepared+i)%PreparedSlots;unsigned expected=0;
        // A still-live allocation owns its decoded bank. Transient capture
        // reuse must never evict a resident file's prepared metadata.
        if(bankAlive(preparedStorage.get()[at])) continue;
        if(readers[at].compare_exchange_strong(expected,Writing,std::memory_order_acquire,std::memory_order_relaxed)) {destination=at;nextPrepared=at+1;break;}
    }
    if(destination==PreparedSlots) {++rejected;return 0;}
    auto& prepared=preparedStorage.get()[destination];
    readySerial[destination].store(0,std::memory_order_release);
    retainedBytes-=bankBytes[destination];bankBytes[destination]=0;
    for(auto& part:prepared.decoded) clearPart(part);
    prepared.parts=owned.parts;
    // Reset every part before releasing this bank. Its physical receipt list
    // stays immutable until a future writer claims the whole bank. Only one
    // preparation owner builds parts; release publication protects each part's
    // own immutable payload while consumers may read already completed peers.
    for(auto& part:prepared.decoded) part.state.store(0,std::memory_order_relaxed);
    readyParts[destination].publish(prepared.parts);
    readers[destination].fetch_and(~Writing,std::memory_order_release);
    readySerial[destination].store(owned.parts.spans[0].serial,std::memory_order_release);
    unsigned mask=0;
    for(unsigned i=0;i<owned.parts.count;++i) {
        const auto& span=owned.parts.spans[i];const auto offset=static_cast<unsigned>(span.chunk-owned.parent);
        auto& part=prepared.decoded[i];
        if(span.size<0x38) {part.state.store(2,std::memory_order_release);notifyPrepared();continue;}
        try {
            const auto* bytes=owned.bytes.data()+offset;
            auto decoded=decodeOwnedAtelScript(bytes,span.size);
            part.script=std::move(decoded);part.declarations.clear();part.orderedDeclarations.clear();part.declarationHeaders.clear();
            part.identity={};part.identity.span=span;part.identity.checksum=part.script.checksum;
            part.identity.length=part.script.codeLength;part.identity.codeOffset=word(bytes,0x30);
            if(part.script.code.empty()) part.guardCode.assign(bytes+part.identity.codeOffset,bytes+part.identity.codeOffset+part.identity.length);
            part.identity.workers=static_cast<unsigned>(part.script.workers.size());
            std::memcpy(part.identity.name,part.script.name.data(),std::min(size_t{8},part.script.name.size()));
            for(const auto& worker:part.script.workers) {
                uint16_t count;std::memcpy(&count,bytes+worker.header+2,2);
                const unsigned begin=word(bytes,worker.header+0x14);
                if(count>2048 || begin>span.size || count>(span.size-begin)/8) throw std::runtime_error("Invalid exact ATEL declarations");
                const uint64_t declarationBytes=2ull*(part.declarations.size()+count)*sizeof(Declaration)+
                    (part.declarationHeaders.size()+1ull)*sizeof(HeaderDeclarations);
                if(declarationBytes>0x200000u) throw std::runtime_error("ATEL declaration cache budget exceeded");
                part.declarationHeaders.push_back({worker.header,static_cast<unsigned>(part.declarations.size()),count});
                for(unsigned j=0;j<count;++j) part.declarations.push_back({worker.header,word(bytes,begin+j*8),word(bytes,begin+j*8+4),j});
            }
            part.orderedDeclarations=part.declarations;
            std::sort(part.declarationHeaders.begin(),part.declarationHeaders.end(),[](const HeaderDeclarations& a,const HeaderDeclarations& b) {return a.header<b.header;});
            std::sort(part.declarations.begin(),part.declarations.end(),[](const Declaration& a,const Declaration& b) {
                if(a.header!=b.header) return a.header<b.header;if(a.low!=b.low) return a.low<b.low;
                if(a.high!=b.high) return a.high<b.high;return a.index<b.index;
            });
            const auto bytesUsed=partBytes(part);
            if(bytesUsed>PreparedBudget-retainedBytes) throw std::runtime_error("ATEL aggregate prepared cache budget exceeded");
            bankBytes[destination]+=bytesUsed;retainedBytes+=bytesUsed;
            part.state.store(1,std::memory_order_release);mask|=1u<<i;
        } catch(...) {clearPart(part);part.state.store(2,std::memory_order_release);}
        // First eligible consumer no longer waits for unrelated EV parts.
        notifyPrepared();
    }
    return mask;
}

void drain() {
    preparedStorage.prepare();collectRetired();unsigned slot=0;
    while(requests.tryPop(slot)) {
        unsigned mask=0;
        try {mask=consume(slot);} catch(...) {++rejected;recordLog("owned ATEL preparation unavailable");}
        const auto& capture=captureStorage.get()[slot];
        completions[capture.receipt].publish({capture.parts,mask});
        releaseCapture(slot);
        notifyPrepared();
    }
    notifyPrepared(); // Also resolves a Native-rejected capture with no FIFO item.
}
DWORD WINAPI worker(void*) {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    for(;;) {WaitForSingleObject(wake.load(std::memory_order_acquire),INFINITE);drain();}
}
}
bool nativeAtelSpan(uintptr_t chunk,AtelSpan& span) {
    bool found=false;
    for(unsigned slot=0;slot<NativeReceipts;++slot) {
        const auto alive=aliveParents[slot].load(std::memory_order_acquire);if(alive<=1) continue;
        Parts parts;if(!nativeSpans[slot].tryRead(parts) || !parts.parent || parts.parent!=alive ||
            aliveParents[slot].load(std::memory_order_acquire)!=parts.parent) continue;
        for(unsigned i=0;i<parts.count && i<AtelResourceParts;++i)
            if(parts.spans[i].chunk==chunk &&
                (!found || static_cast<int32_t>(parts.spans[i].serial-span.serial)>0)) {span=parts.spans[i];span.epoch=generation();found=true;}
    }
    return found;
}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& result) {
    Lease lease;if(!acquire(chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];if(part.state.load(std::memory_order_acquire)!=1) return false;
    result=part.identity;result.span.epoch=generation();return true;
}
AtelPreparation atelPreparation(uintptr_t chunk) {
    AtelSpan span;if(!nativeAtelSpan(chunk,span)) return AtelPreparation::Missing;
    for(unsigned i=0;i<NativeReceipts;++i) if(rejectedSerial[i].load(std::memory_order_acquire)==span.serial) return AtelPreparation::Rejected;
    Lease lease;if(acquire(chunk,lease)) {
        const auto state=preparedStorage.get()[lease.slot].decoded[lease.part].state.load(std::memory_order_acquire);
        return state==1?AtelPreparation::Ready:state==2?AtelPreparation::Rejected:AtelPreparation::Pending;
    }
    for(const auto& published:completions) {
        Completion completed;if(!published.tryRead(completed)) continue;
        for(unsigned i=0;i<completed.parts.count && i<AtelResourceParts;++i) {
            const auto& item=completed.parts.spans[i];
            if(item.chunk==chunk && item.serial==span.serial)
                return completed.validMask&(1u<<i)?AtelPreparation::Retired:AtelPreparation::Rejected;
        }
    }
    return AtelPreparation::Pending;
}
bool onAtelPrepared(AtelPreparedHandler handler) {
    const auto count=preparedHandlerCount.load(std::memory_order_relaxed);
    if(!handler || count==preparedHandlers.size()) return false;
    preparedHandlers[count]=handler;preparedHandlerCount.store(count+1,std::memory_order_release);return true;
}
bool readyAtelWorkerHeader(uintptr_t chunk,unsigned local,unsigned& header) {
    Lease lease;if(!acquire(chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];if(part.state.load(std::memory_order_acquire)!=1) return false;
    const auto& workers=part.script.workers;
    if(local>=workers.size()) return false;header=workers[local].header;return true;
}
bool readyAtelDeclaration(uintptr_t chunk,unsigned header,unsigned low,unsigned high,unsigned& index) {
    Lease lease;if(!acquire(chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];if(part.state.load(std::memory_order_acquire)!=1) return false;
    const auto& entries=part.declarations;
    const Declaration key{header,low,high,0};
    const auto at=std::lower_bound(entries.begin(),entries.end(),key,[](const Declaration& a,const Declaration& b) {
        if(a.header!=b.header) return a.header<b.header;if(a.low!=b.low) return a.low<b.low;return a.high<b.high;
    });
    if(at==entries.end() || at->header!=header || at->low!=low || at->high!=high) return false;
    index=at->index;return true;
}
bool readyAtelDeclarationAt(uintptr_t chunk,unsigned header,unsigned index,unsigned& low,unsigned& high) {
    Lease lease;if(!acquire(chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];if(part.state.load(std::memory_order_acquire)!=1) return false;
    const auto& headers=part.declarationHeaders;
    const auto at=std::lower_bound(headers.begin(),headers.end(),header,[](const HeaderDeclarations& item,unsigned value) {return item.header<value;});
    if(at==headers.end() || at->header!=header || index>=at->count) return false;
    const auto& declaration=part.orderedDeclarations[at->begin+index];low=declaration.low;high=declaration.high;return true;
}
bool OwnedAtelGuardView::callbackAvailable(unsigned local,unsigned entry,const AtelSavedFacts& saved) const {
    const auto& part=*static_cast<const PreparedPart*>(context);
    if(local>=part.script.workers.size()) return true;
    const auto header=part.script.workers[local].header;
    const auto& headers=part.declarationHeaders;
    const auto row=std::lower_bound(headers.begin(),headers.end(),header,[](const HeaderDeclarations& item,unsigned value) {return item.header<value;});
    const auto value=[&](unsigned index) -> std::optional<int> {
        if(!saved.valid || row==headers.end() || row->header!=header || index>=row->count) return {};
        const auto& declaration=part.orderedDeclarations[row->begin+index];
        const auto low=declaration.low,format=low>>28,offset=low&0xffffff;
        if(((low>>25)&7)!=0 || !(declaration.high&0xffff) || format>5) return {};
        const auto size=format<2?1u:format<4?2u:4u;
        if(offset>saved.bytes.size() || size>saved.bytes.size()-offset) return {};
        uint32_t raw=0;std::memcpy(&raw,saved.bytes.data()+offset,size);
        return format==0?static_cast<int>(static_cast<uint8_t>(raw)):format==1?static_cast<int>(static_cast<int8_t>(raw)):
            format==2?static_cast<int>(static_cast<uint16_t>(raw)):format==3?static_cast<int>(static_cast<int16_t>(raw)):static_cast<int>(raw);
    };
    return callbackAvailableOwned(part.script,local,entry,value,part.script.code.empty()?&part.guardCode:nullptr);
}
bool readyAtelCallbackAvailable(const AtelSpan& span,unsigned local,unsigned entry,const AtelSavedFacts& saved,bool& available) {
    if(nativeGameThread()) return false;
    Lease lease;if(!acquire(span.chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];
    if(part.state.load(std::memory_order_acquire)!=1 || part.identity.span.serial!=span.serial || local>=part.script.workers.size()) return false;
    available=OwnedAtelGuardView{part.script,&part}.callbackAvailable(local,entry,saved);return true;
}
bool visitReadyAtelGuardedScript(const AtelSpan& span,OwnedAtelGuardConsumer consumer,void* context) {
    if(nativeGameThread() || !consumer) return false;
    Lease lease;if(!acquire(span.chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];
    if(part.state.load(std::memory_order_acquire)!=1 || part.identity.span.serial!=span.serial) return false;
    const OwnedAtelGuardView view{part.script,&part};consumer(view,context);return true;
}
bool visitReadyAtelScript(const AtelSpan& span,OwnedAtelConsumer consumer,void* context) {
    if(nativeGameThread() || !consumer) return false;
    Lease lease;if(!acquire(span.chunk,lease)) return false;
    const auto& part=preparedStorage.get()[lease.slot].decoded[lease.part];
    if(part.state.load(std::memory_order_acquire)!=1 || part.identity.span.serial!=span.serial) return false;
    consumer(part.script,context);return true;
}
bool startAtelResources() {
    captureStorage.prepare();
#ifndef FFX_READER_TEST
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&worker),&pin)) return false;
    const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event) return false;
    wake.store(event,std::memory_order_release);
    const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) {wake.store(nullptr,std::memory_order_release);CloseHandle(event);return false;}
    CloseHandle(thread);
#endif
    return true;
}
void installAtelResources() {hookSnapshot(Loader,loaderBefore,loaderAfter);hookSnapshot(Allocator,allocationBefore,allocationAfter);hookSnapshot(FreeResource,resourceFreed,nullptr);hookSnapshot(0x46ca90,nullptr,workerConstructed);}
#ifdef FFX_READER_TEST
void drainAtelResourcesForTest() {drain();}
unsigned rejectedAtelResourcesForTest() {return rejected.load();}
size_t preparedAtelBytesForTest() {return retainedBytes;}
#endif
}
