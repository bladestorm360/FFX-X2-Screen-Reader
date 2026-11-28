#pragma once
#include "navigation.hpp"
#include "nonblocking_snapshots.hpp"
#include <array>

namespace ffx::nav {
// The destination auto walk was heading to when a battle stopped it.
struct ResumeRequest {std::array<wchar_t,2050> name{};unsigned id=0,pushControl=0;Point position;};
static_assert(std::is_trivially_copyable_v<ResumeRequest> && sizeof(ResumeRequest)%4==0);
// Written on the game thread when the resume fires, read by the preparation owner.
// Publishing copies into a fixed buffer: no lock and no allocation on the game thread.
class ResumeMailbox {
public:
    void publish(const wchar_t* name,unsigned id,unsigned pushControl,Point position) noexcept {
        pending_.name={};unsigned i=0;
        for(;name && name[i] && i+1<pending_.name.size();++i) pending_.name[i]=name[i];
        pending_.id=id;pending_.pushControl=pushControl;pending_.position=position;
        snapshot_.publish(pending_);
    }
    // One reader; a read overlapping a new publish fails and the caller reports it.
    bool take(ResumeRequest& out) const noexcept {return snapshot_.tryRead(out);}
private:
    ResumeRequest pending_; // publisher only
    async::AtomicSnapshot<ResumeRequest> snapshot_;
};
}
