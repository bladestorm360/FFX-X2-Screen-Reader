// Battle reward announcement (AP / items / gil after a win).

import { btlRewardAddr, btlItemBinAddr, CHARACTER_NAMES } from '../core/addresses.js';
import { resolveCommandText } from '../core/charset.js';

export function announceRewardsPhase1(speak) {
    const apParts = [];
    for (let i = 0; i < 18; i++) {
        if (!btlRewardAddr.add(i).readU8()) continue;
        const ap = btlRewardAddr.add(0x80 + i * 4).readU32();
        apParts.push((i < CHARACTER_NAMES.length ? CHARACTER_NAMES[i] : 'chr'+i) + ' ' + ap + ' AP');
    }
    if (apParts.length > 0) speak(apParts.join(', '), false);
}

export function announceRewardsPhase2(speak) {
    const itemCnt = btlRewardAddr.add(0xD0).readU8();
    if (itemCnt > 0) {
        const parts = [];
        for (let ii = 0; ii < itemCnt && ii < 8; ii++) {
            const itemId = btlRewardAddr.add(0xD4 + ii * 2).readU16();
            const itemAmt = btlRewardAddr.add(0xE4 + ii).readU8();
            const itemName = resolveCommandText(itemId, btlItemBinAddr);
            parts.push((itemName || 'item') + ' x' + itemAmt);
        }
        speak(parts.join(', '), false);
    }
    const gearCnt = btlRewardAddr.add(0xD2).readU8();
    for (let gi = 0; gi < gearCnt && gi < 8; gi++) {
        speak('Equipment obtained', false);
    }
    const gil = btlRewardAddr.add(0xCC).readU32();
    if (gil > 0) speak(gil + ' Gil', false);
}
