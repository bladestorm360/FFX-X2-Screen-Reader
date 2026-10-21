#pragma once
namespace ffx::auron_rva {
inline constexpr unsigned Initialize=0x498a40, State=0xf3d6b4;
inline constexpr unsigned Count=0xf3d6c0, Sequence=0xf3d6c4;
inline constexpr unsigned dependencies[]={Initialize,Count,Sequence};
}
namespace ffx { void resetOverdrive(); }
