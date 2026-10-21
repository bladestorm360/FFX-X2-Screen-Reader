#pragma once
namespace ffx {
inline constexpr const wchar_t* hiddenAirshipNames[]={L"Baaj Temple",L"Omega Ruins",L"Besaid Falls",L"Mi’ihen Ruins",L"Battle Site",L"Sanubia Sands"};
// Both entry and activation require the setting and native search context.
bool hiddenAirshipMenuAvailable();
bool beginHiddenAirshipSearch(unsigned destination);
void openHiddenAirshipMenu();
bool readAirshipCoordinates();
void installAirshipSearch();
void resetAirshipSearch();
#ifdef FFX_READER_TEST
void drainAirshipSearchForTest();
#endif
}
