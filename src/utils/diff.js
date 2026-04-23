// "Has this value changed since last tick?" helper. Each diff instance holds
// its own key→lastValue map so unrelated features don't collide.

export function makeDiff() {
    const last = {};
    return {
        changed(key, value) {
            const prev = last[key];
            last[key] = value;
            return prev !== value;
        },
        reset(key) {
            if (key === undefined) {
                for (const k in last) delete last[k];
            } else {
                delete last[key];
            }
        }
    };
}
