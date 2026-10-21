#pragma once
#include <string>
namespace ffx {
enum class BattleGraphic { Miss, Guard, Overkill };
// Exact words reconstructed from displayed locale-specific icon atlas pixels.
std::wstring battleGraphicText(BattleGraphic label);
const wchar_t* battleGraphicLabel(BattleGraphic label);
}
