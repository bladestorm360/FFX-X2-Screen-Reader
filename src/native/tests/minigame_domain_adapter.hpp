#pragma once
#include "../speech_domains.hpp"
#include <cstring>
#include <deque>
#include <map>
#include <vector>

namespace ffx::speechq::fixture {
struct Packet { unsigned domain=0; Header header{}; std::vector<uint8_t> bytes; };
inline std::map<unsigned,DomainHandler> receivers;
inline std::map<unsigned,DomainMaintenance> maintenance;
inline std::map<unsigned,DomainReset> resets;
inline std::deque<Packet> fifo;
inline std::vector<std::wstring>* spoken=nullptr;
inline unsigned preparationNotifications=0;
inline unsigned cancellation=0;
inline bool captureDomain(unsigned domain,const void* data,unsigned size,const wchar_t*,unsigned,unsigned epoch) {
    if(!data || !size || size%sizeof(wchar_t)) return false;
    Packet packet;packet.domain=domain;packet.header.generation=epoch;packet.header.context=SpeechEnabled;packet.header.cancellation=cancellation;
    packet.header.flags=Mod;packet.bytes.resize(size);std::memcpy(packet.bytes.data(),data,size);
    fifo.push_back(std::move(packet));return true;
}
inline void drain() {
    while(!fifo.empty()) {
        auto packet=std::move(fifo.front());fifo.pop_front();
        if(packet.header.generation!=ffx::generation()) continue; // Same stale dispatch guard as production.
        auto receiver=receivers.find(packet.domain);
        if(receiver!=receivers.end() && receiver->second) {
            const auto* chars=reinterpret_cast<const wchar_t*>(packet.bytes.data());
            receiver->second(packet.header,std::wstring(chars,packet.bytes.size()/sizeof(wchar_t)));
        }
    }
    for(const auto& entry:maintenance) if(entry.second) entry.second();
}
}

namespace ffx::speechq {
bool captureDomain(unsigned domain,const void* data,unsigned size,const wchar_t* text,unsigned length,unsigned epoch) {
    return fixture::captureDomain(domain,data,size,text,length,epoch);
}
bool emitOwned(std::wstring text,const Header& anchor,const char*,bool,bool) {
    if(anchor.generation!=ffx::generation() || anchor.cancellation!=fixture::cancellation) return false;
    if(fixture::spoken) fixture::spoken->push_back(std::move(text));return true;
}
void registerDomain(unsigned domain,DomainHandler receiver,DomainReset reset) {
    fixture::receivers[domain]=receiver;fixture::resets[domain]=reset;
}
void registerDomainMaintenance(unsigned domain,DomainMaintenance callback) {fixture::maintenance[domain]=callback;}
void notifyPreparation() noexcept {++fixture::preparationNotifications;}
void resetDomains(unsigned epoch) {for(const auto& entry:fixture::resets) if(entry.second) entry.second(epoch);}
}
