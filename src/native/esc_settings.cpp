// Observe the native configuration setter, comparing the old accepted value to
// the requested new value. Field indices are qualified against the real config
// object; copied EscPackets carry names/values to owned formatting.
// See Docs/features/menus/index.md#escape-and-title.
#include "events.hpp"
#include "esc_details.hpp"
#include "esc_capture.hpp"

namespace ffx {
namespace {
void before(Event& e) {
    const auto config = read<uintptr_t>(address(0x1efb504));
    if (config != e.ecx || e.args[0] > 64) return;
    const int video=escVideoFieldIndex(static_cast<unsigned>(e.args[0]));
    if(video>=0) {e.before[0]=2;e.before[1]=video;e.before[2]=read<unsigned>(e.ecx+e.args[0]*4);}
    for (unsigned i = 0; i < 4; ++i) {
        const auto entry = address(0x839620) + i * 0x18;
        if (read<unsigned>(entry + 0xc, 0xffffffff) != e.args[0]) continue;
        e.before[0] = 1; e.before[1] = read<unsigned>(entry + 4);
        e.before[2] = read<unsigned>(e.ecx + e.args[0] * 4);
        return;
    }
}
void after(Event& e) {
    if (!e.before[0] || static_cast<uintptr_t>(e.before[2]) == e.args[1]) return;
    EscPacket packet;packet.value=static_cast<unsigned>(e.args[1]);
    if(e.before[0]==2) {
        const auto index=static_cast<unsigned>(e.before[1]);if(index>=_countof(escVideoNames)) return;
        prepareEscPacket(packet,EscCaptureKind::VideoChanged);packet.index=index;
        copyEscLocalized(escVideoNames[index],packet.text[0]);copyEscVideo(index,packet.value,packet.text[1]);
    } else {
        prepareEscPacket(packet,EscCaptureKind::AudioChanged);
        copyEscLocalized(static_cast<unsigned>(e.before[1]),packet.text[0],256);
    }
    commitEscPacket(packet);
}
}
void installEscSettings() { hook(0x2a00, before, after); }
}
