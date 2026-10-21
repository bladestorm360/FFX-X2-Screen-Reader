#pragma once
#include "runtime.hpp"
#include "utility_text.hpp"
#include "utility_context.hpp"

namespace ffx {
// Console submissions cross to the existing native input callback before any
// game getter, script request or mutation runs. The UI thread owns only text.
void installUtilities();
void pumpUtilities();
bool queueUtility(const std::wstring& command);
bool queueUtilityAt(const std::wstring& command, unsigned epoch);
bool queueUtilityContext(const std::wstring& command,const UtilityContext&);
void invalidateUtilityConsole();
void openUtilityConsole();
bool utilityConsoleActive();
void utilityReply(const std::wstring& text);
void utilityReply(const wchar_t* text);
void startUtilityConsole(); // Bootstrap only; native requests never create a thread/window.
void consoleReplyReady(const wchar_t* text,size_t length);
void consoleReply(const std::wstring& text);
std::wstring executeUtility(const std::wstring& command);
UtilityText executeUtilityNative(std::wstring_view command);
}
