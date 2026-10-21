const assert = require('node:assert/strict');
const {buttonGlyphActions, buttonGlyphControl} = require('../Frida/FFX/button_glyph_controls.js');
const names = ['Up','Down','Left','Right','A','B','X','Y','LB','LT','RB','RT','Menu','View'];
const expected = ['X','A','B','Y','LB','RB','LT','RT','Menu','View'];
for (let glyph=0x30;glyph<0x3a;glyph++) assert.equal(buttonGlyphControl(glyph,false,i=>names[i]),expected[glyph-0x30]);
assert.equal(buttonGlyphControl(0x49,false,i=>names[i]),'Up or Left');
assert.equal(buttonGlyphControl(0x31,true,i=>names[i]),'B');
assert.equal(buttonGlyphControl(0x32,true,i=>names[i]),'A');
assert.equal(buttonGlyphControl(0x32,false,()=>''),'unassigned control');
assert.equal(buttonGlyphControl(0x40,false,()=>''),'directional pad');
assert.deepEqual(buttonGlyphActions(0x33,false),[7]);
assert.deepEqual(buttonGlyphActions(0x32,false),[5]);
assert.deepEqual(buttonGlyphActions(0x31,false),[4]);
// The previous helper misread these native tokens as shoulder actions 8-10.
assert.notEqual(names[8],buttonGlyphControl(0x33,false,i=>names[i]));
assert.notEqual(names[9],buttonGlyphControl(0x32,false,i=>names[i]));
assert.notEqual(names[10],buttonGlyphControl(0x31,false,i=>names[i]));
console.log('PASS: offline Frida glyph prototype reproduces old shoulder-row error and resolves actual configured actions.');
