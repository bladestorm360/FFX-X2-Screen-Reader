#pragma once
#include <array>
namespace ffx {
struct Binding { unsigned first,second,label; };
unsigned configuredKeyboardScan(unsigned id);
int configuredControllerButton(unsigned id);
unsigned configuredActionKeyboardScan(unsigned action);
std::array<unsigned,2> configuredActionKeyboardScans(unsigned action);
int configuredActionControllerButton(unsigned action);
// Native action/localization IDs shared by spoken settings and movement input.
inline constexpr Binding keyboard[]={{0,49,632},{1,50,633},{2,51,634},{3,52,635},{4,53,636},{5,54,637},
    {6,55,638},{7,56,639},{8,57,897},{10,59,898},{9,58,899},{11,60,900},{38,73,901},{39,74,902},
    {36,71,903},{37,72,904},{12,61,644},{13,62,645},{28,63,646},{33,68,651},{34,69,652},{29,64,647},{31,66,649}};
inline constexpr Binding controller[]={{14,14,632},{15,15,633},{16,16,634},{17,17,635},{18,18,636},{19,19,637},
    {20,20,638},{21,21,639},{22,22,640},{23,23,641},{24,24,642},{25,25,643},{26,26,644},{27,27,645}};
}
