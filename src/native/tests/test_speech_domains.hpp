#pragma once
#include "../speech_domains.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include <deque>
#include <map>
#include <cstring>

namespace ffx {
bool hookSnapshot(unsigned id,Handler enter,Handler leave) {return hook(id,enter,leave);}
namespace speechq {
namespace fixture {
struct Registration { DomainHandler handler; DomainReset reset; };
struct Pending { unsigned domain; Header header; std::wstring text; };
inline std::map<unsigned,Registration> registrations;
inline std::map<unsigned,DomainMaintenance> maintenance;
inline std::map<unsigned,std::pair<DomainProtection,DomainMaintenance>> protection;
inline std::deque<Pending> pending;
inline unsigned cancellation=0;
}
void registerDomainProtection(unsigned id,DomainProtection fn,DomainMaintenance reset) {fixture::protection[id]={fn,reset};}
bool domainsProtectSpeech(const Header& header) {
    for(const auto& entry:fixture::protection) if(entry.second.first && entry.second.first(header)) return true;return false;
}
void resetDomainProtection() {for(const auto& entry:fixture::protection) if(entry.second.second) entry.second.second();}
void registerDomainMaintenance(unsigned id,DomainMaintenance fn) {fixture::maintenance[id]=fn;}
bool capturesPending() noexcept {return !fixture::pending.empty();}
void registerDomain(unsigned id,DomainHandler handler,DomainReset reset) {
    fixture::registrations[id]={handler,reset};
}
bool captureDomain(unsigned id,const void* data,unsigned bytes,const wchar_t* text,unsigned length,unsigned) {
    if(bytes%sizeof(wchar_t)) return false;
    Header header{};
    header.generation=generation();header.cancellation=fixture::cancellation;
    header.context=setting(Setting::Speech)?SpeechEnabled:0;
    std::wstring owned(bytes/sizeof(wchar_t),L'\0');
    if(bytes) std::memcpy(owned.data(),data,bytes);
    if(text && length) owned.append(text,length);
    fixture::pending.push_back({id,header,std::move(owned)});
    return true;
}
bool emitOwned(std::wstring text,const Header& header,const char* producer,bool voiced,bool interrupt) {
    if(header.generation!=generation() || header.cancellation!=fixture::cancellation) return false;
    return outputDialogue(text,interrupt,producer,voiced);
}
}
void drainSpeechDomainsForTest() {
    while(!speechq::fixture::pending.empty()) {
        auto item=std::move(speechq::fixture::pending.front()); speechq::fixture::pending.pop_front();
        auto found=speechq::fixture::registrations.find(item.domain);
        if(found!=speechq::fixture::registrations.end() && found->second.handler)
            found->second.handler(item.header,item.text);
    }
    for(const auto& entry:speechq::fixture::maintenance) if(entry.second) entry.second();
}
}
