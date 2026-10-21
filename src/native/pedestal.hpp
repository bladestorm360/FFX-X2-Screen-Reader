#pragma once
#include "navigation.hpp"
#include "utility_trials.hpp"
#include "controller_model.hpp"
#include <array>
#include <string_view>
#include <initializer_list>
#include <stdexcept>
namespace ffx {
enum class PushMode { Slide, Scripted, Single, Crossing };
struct PedestalBindings {
    struct Value {unsigned binding=0;int value=0;};
    // The largest original rule reads seven bindings (Djose). Eight retains
    // every supplied fact without an allocating map on the native owner.
    std::array<Value,8> rows{};unsigned count=0;
    PedestalBindings()=default;
    PedestalBindings(std::initializer_list<Value> values) {for(const auto& row:values) (*this)[row.binding]=row.value;}
    int& operator[](unsigned binding) {
        for(unsigned i=0;i<count;++i) if(rows[i].binding==binding) return rows[i].value;
        if(count==rows.size()) throw std::out_of_range("Pedestal binding capacity");
        rows[count].binding=binding;return rows[count++].value;
    }
    int at(unsigned binding) const {
        for(unsigned i=0;i<count;++i) if(rows[i].binding==binding) return rows[i].value;
        throw std::out_of_range("Required pedestal binding missing");
    }
};
struct PedestalSnapshot {
    Point position;
    PedestalBindings values;
    int get(unsigned binding) const { return values.at(binding); }
};
// held is the logical input consumed by the original trial script, not raw DI.
struct PedestalMove { Point goal; unsigned held=0; PushMode mode=PushMode::Slide; };
struct PedestalMoves {
    std::array<PedestalMove,2> rows{};unsigned count=0;
    bool empty() const {return !count;}size_t size() const {return count;}
    PedestalMove* begin() {return rows.data();}PedestalMove* end() {return rows.data()+count;}
    const PedestalMove* begin() const {return rows.data();}const PedestalMove* end() const {return rows.data()+count;}
    PedestalMove& front() {return rows[0];}const PedestalMove& front() const {return rows[0];}
    PedestalMove& operator[](size_t i) {return rows[i];}const PedestalMove& operator[](size_t i) const {return rows[i];}
    void push_back(PedestalMove move) {if(count==rows.size()) throw std::out_of_range("Pedestal move capacity");rows[count++]=move;}
    void erase(PedestalMove* first,PedestalMove* last) {
        const unsigned at=static_cast<unsigned>(first-begin()),removed=static_cast<unsigned>(last-first);
        if(at>count || removed>count-at) throw std::out_of_range("Pedestal move range");
        for(unsigned i=at;i+removed<count;++i) rows[i]=rows[i+removed];count-=removed;
    }
};
struct PedestalPlan {
    PedestalMoves moves;
    // Every original refusal is a static mod-owned literal. It is never a
    // borrowed game string and is copied into owned output on the worker.
    std::wstring_view blocked;
    unsigned phase=0,producer=0,entry=0;
    bool complete=false;
};
static_assert(std::is_trivially_copyable_v<PedestalSnapshot> && std::is_trivially_destructible_v<PedestalPlan>);
PedestalPlan pedestalPlan(unsigned control,unsigned local,const PedestalSnapshot&);
std::wstring describePedestalPlan(const PedestalPlan&,Point,const std::wstring& action=L"push control");
std::wstring pedestalConsentName(const PedestalPlan&);
std::wstring describePedestalSide(Point pedestal,Point player);
std::wstring describePedestalApproach(const PedestalPlan&,Point pedestal,Point player);
bool pedestalScript(unsigned control,UtilityScript&);
PedestalSnapshot pedestalSnapshot(const UtilityScript&,const Entity&);
bool capturePedestalSnapshot(const UtilityScript&,const NativeRouteTarget&,PedestalSnapshot&) noexcept;
PedestalPlan currentPedestalPlan(const Entity&);
void preparePedestalZone(Entity&,uintptr_t worker);
bool pedestalCrossingZone(Entity&);
bool pedestalActiveCrossing(const Entity&);
bool pedestalActiveContact(const Entity&);
bool pedestalActivePlan(const Entity&,PedestalPlan&);
bool pedestalContactZone(Entity&);
struct PedestalNavigationState {
    PedestalPlan plan;
    unsigned epoch=0,control=0,local=0,id=0;
    int worker=-1;
    bool armed=false,held=false,contact=false,crossing=false;
};
PedestalNavigationState capturePedestalNavigationState(); // Native owner, fixed copied state only.
void armPedestal(const Entity&);
void armPedestalOwned(const NativeRouteTarget&,const UtilityScript&,const PedestalPlan&);
void updatePedestalOwnedTarget(const NativeRouteTarget&);
void cancelPedestal();
void updatePedestalInput();
void installPedestal();
unsigned pedestalConfirmMask();
unsigned pedestalInputMask(unsigned logical);
unsigned pedestalLogicalInput(unsigned raw);
bool publishPedestalConsent(unsigned mask);
bool pedestalConsentKey(unsigned scan);
bool pedestalConsentVk(unsigned vk);
bool pedestalMovementKey(unsigned scan);
uint64_t pedestalConsentButtons();
void pedestalKeyboardInput(const uint8_t* keys);
void pedestalControllerInput(const pad::State&);
void pedestalPlayerInput(unsigned vk);
#ifdef FFX_READER_TEST
bool pedestalTargetForTest(NativeRouteTarget&);
#endif
}
