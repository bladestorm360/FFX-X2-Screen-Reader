#pragma once
#include "events.hpp"
#include <cstring>
namespace ffx {
// Original 91AB30 identifies the airship map as room382 when positioning its
// layer26 cursor. The operand is resolved from that complete qualified body.
// This screen owns a movable search actor, but it is not the ordinary field:
// field beacons, wall rays and blocked-movement reports have no meaning here.
inline bool airshipMapActive() {
    return nativeGameThread() && read<int>(address(0xf30804),-1)==382;
}
// Use the same native script/state as M's coordinate reader. Both airship menu
// entry routes reach this search flag; the renderer's room382 special case is
// insufficient as the sole field-audio ownership test. These fixed header/name
// reads stay on the native owner, without hashing or visiting worker objects.
inline uintptr_t airshipSearchScript() {
    if(!nativeGameThread()) return 0;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const auto chunk=controller?read<uintptr_t>(controller+0x48):0;
    if(!chunk || read<unsigned>(chunk)!=24785 || read<uint16_t>(chunk+0x34)!=41 ||
        read<unsigned>(chunk+0x20)!=0x150f8 || read<unsigned>(chunk+0x30)!=0xe6c) return 0;
    const auto nameOffset=read<unsigned>(chunk+0xc);
    char name[8]{};
    return nameOffset<=0x100000-8 && copy(chunk+nameOffset,name,sizeof(name)) &&
        std::memcmp(name,"hiku2100",sizeof(name))==0?chunk:0;
}
inline bool airshipFreeSearchActive() {
    const auto chunk=airshipSearchScript();
    return chunk && read<int>(chunk+0x15248)==1;
}
inline bool airshipNavigationUiActive() {
    return airshipMapActive() || airshipFreeSearchActive();
}
}
