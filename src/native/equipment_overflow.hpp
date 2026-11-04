#pragma once
namespace ffx {
// Called by the existing battle-results open/advance/reset owners. This reader
// observes the native capacity UI; it never selects, swaps or discards gear.
void beginEquipmentOverflow();
void observeEquipmentOverflow();
void resetEquipmentOverflow();
}
