// Context-gated speech wrapper: skip speaking the same text twice in a row
// within the same context key. Distinct contexts don't dedup against each
// other, so overlapping announcements from different features can coexist.
//
// Usage:
//   import { makeTracker } from '../utils/speechTracker.js';
//   const say = makeTracker(speech);
//   say('battle.cursor', 'Attack', true);  // speaks
//   say('battle.cursor', 'Attack', true);  // skipped (same ctx, same text)
//   say('nav.select',   'Attack', true);   // speaks (different ctx)

export function makeTracker(speech) {
    const last = {};
    return function say(contextKey, text, interrupt) {
        if (last[contextKey] === text) return;
        last[contextKey] = text;
        speech.speak(text, interrupt);
    };
}
