// Entity-label parsing. The game gives each actor a debug "asset-name" string
// at chr_name (actor+0x04), e.g. "chr_wakka_a", "barrel_01". For player-facing
// speech we want a friendly noun, not the raw slug. This module maps known
// slug stems to display names and recognises common object prefixes.
//
// KNOWN_NAMES is keyed by lower-case slug stem and produces the display name
// the user already hears elsewhere in the mod. Verified against the in-binary
// character-name tables at RVAs 0x748668 (table 1, "Tidus(0)" / "Auron(2)" /
// "Wakka(4)" / "Lulu(5)") and 0x756120 (table 2, "Tidus" / "Auron" /
// "Kimahri" / "Wakka") — see strings_dump.txt.
//
// Asset slugs sometimes diverge from display names (e.g. asset "kimari" vs
// display "Kimahri", asset "yojinbo" vs display "Yojimbo"); both spellings
// route to the canonical display name.

const KNOWN_NAMES = {
    // Party
    'tidus': 'Tidus',
    'yuna': 'Yuna',
    'auron': 'Auron',
    'kimari': 'Kimahri',
    'kimahri': 'Kimahri',
    'wakka': 'Wakka',
    'lulu': 'Lulu',
    'rikku': 'Rikku',
    'seymour': 'Seymour',
    // Aeons
    'valefor': 'Valefor',
    'ifrit': 'Ifrit',
    'ixion': 'Ixion',
    'shiva': 'Shiva',
    'bahamut': 'Bahamut',
    'anima': 'Anima',
    'yojinbo': 'Yojimbo',
    'yojimbo': 'Yojimbo',
    // Magus Sisters
    'cindy': 'Cindy',
    'sandy': 'Sandy',
    'mindy': 'Mindy'
};

const OBJECT_PREFIXES = ['barrel', 'crate', 'door', 'lamp', 'chest', 'sign', 'box'];

// Returns {kind, name} when the slug matches a recognised pattern, else null.
//   kind === 'character' — chr_<stem>[_*]; name is display name (KNOWN_NAMES
//     lookup) or the raw stem capitalised when unknown.
//   kind === 'object' — slug starts with one of OBJECT_PREFIXES (with or
//     without an _suffix); name is the prefix word.
export function parseSlug(devString) {
    if (!devString) return null;
    const s = String(devString).toLowerCase();

    const chrMatch = /^chr_([a-z0-9]+)/.exec(s);
    if (chrMatch) {
        const stem = chrMatch[1];
        const known = KNOWN_NAMES[stem];
        if (known) return { kind: 'character', name: known };
        return { kind: 'character', name: stem.charAt(0).toUpperCase() + stem.slice(1) };
    }

    for (let i = 0; i < OBJECT_PREFIXES.length; i++) {
        const p = OBJECT_PREFIXES[i];
        if (s === p || s.indexOf(p + '_') === 0) {
            return { kind: 'object', name: p };
        }
    }

    return null;
}
