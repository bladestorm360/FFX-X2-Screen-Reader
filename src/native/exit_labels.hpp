#pragma once
#include "atel.hpp"
#include <functional>
namespace ffx {
std::wstring nativeAreaName(int index);
std::wstring navigationExitName(const AtelScript&,unsigned worker,int room,const std::wstring& authored);
std::wstring navigationExitName(const AtelScript&,unsigned worker,int room,const std::wstring& authored,const std::function<std::wstring(int)>& names);
}
