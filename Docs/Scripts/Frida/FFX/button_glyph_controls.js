// Offline Frida text prototype; no timer, attachment or production producer.
// Native6E6860 maps tokens to actions, Native631770 maps actions to masks.
function buttonGlyphActions(glyph, swap) {
    if (swap && (glyph === 0x31 || glyph === 0x32)) glyph = glyph === 0x31 ? 0x32 : 0x31;
    if (glyph >= 0x30 && glyph < 0x3a) return [[6,4,5,7,8,10,9,11,12,13][glyph - 0x30]];
    if (glyph > 0x40 && glyph < 0x50) return [0,3,1,2].filter((_, bit) => glyph & (1 << bit));
    return [];
}
function buttonGlyphControl(glyph, swap, bindingName) {
    if (glyph === 0x40) return 'directional pad';
    return buttonGlyphActions(glyph, swap).map(action => bindingName(action) || 'unassigned control').join(' or ');
}
if (typeof module !== 'undefined') module.exports = {buttonGlyphActions, buttonGlyphControl};
