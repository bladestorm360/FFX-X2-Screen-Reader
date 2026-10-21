#pragma once
#include <cstdio>
#include <string>
namespace ffx {
FILE* openLogWithHistory(const std::wstring& directory,const wchar_t* name);
}
