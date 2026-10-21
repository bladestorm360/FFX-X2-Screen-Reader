#pragma once
#include "speech_capture.hpp"
#include <type_traits>
namespace ffx::speechq {
constexpr unsigned DomainBase=256,DomainLimit=32;
enum Domain : unsigned {DialogueVoiceDomain=0,MovieVoiceDomain=1,NamingDomain=2,SelectionDetailDomain=3,SystemMenuDomain=4,EscMenuDomain=5,TextPlanDomain=6,BattleStatusDomain=7,MinigameDomain=8,ButterflyDomain=9};
enum ContextFlags : unsigned {SpeechEnabled=1,QueueProtected=2,ModalOpen=4,TutorialOpen=8};
using DomainHandler=void(*)(const Header&,const std::wstring&);
using DomainReset=void(*)(unsigned);
using DomainMaintenance=void(*)();
using DomainProtection=bool(*)(const Header&);
void registerDomainProtection(unsigned,DomainProtection,DomainMaintenance reset);
bool domainsProtectSpeech(const Header&); // Lock-free copied phase state only; Native and preparation.
void resetDomainProtection(); // Native Stop/reset/explicit protection release, atomic invalidation only.
void registerDomain(unsigned,DomainHandler,DomainReset);
void dispatchDomain(const Header&,const std::wstring&);
void resetDomains(unsigned);
// Preparation owner only: a late marker cannot clear fresh same-epoch audio.
void advanceDomains(unsigned);
void registerDomainMaintenance(unsigned,DomainMaintenance);
void maintainDomains(); // Preparation owner only, after draining captures.
void notifyPreparation() noexcept; // Copied-data dependency became ready; no native work/SDK call.
bool captureDomain(unsigned,const void* data,unsigned bytes,const wchar_t* text,unsigned length,unsigned epoch);
// Preparation-owner-only: emit already-owned output at its logical event point,
// preserving the originating native event's cancellation/modal/protection state.
bool emitOwned(std::wstring,const Header&,const char* producer,bool voiced=false,bool interrupt=false);
template<class T> bool captureDomain(unsigned domain,const T& data,unsigned epoch,const std::wstring& text={}) {
    static_assert(std::is_trivially_copyable_v<T> && sizeof(T)%sizeof(wchar_t)==0);
    return captureDomain(domain,&data,sizeof(T),text.data(),static_cast<unsigned>(text.size()),epoch);
}
}
