// Offline policy/identity checks; never loads Frida or the game.
const assert = require('node:assert/strict');
const { audibleMovie, captionIdentity } = require('../Frida/FFX/movie_subtitles.js');
const audible = { playing:true, initialized:true, paused:false, audioStarted:true,
    channel:123, audioTrack:456, playbackState:1, audioPaused:false, volume:1 };
assert.equal(audibleMovie(audible), true);
for (const [key,value] of Object.entries({ playing:false,initialized:false,
    audioStarted:false,channel:0,audioTrack:0,playbackState:3,audioPaused:true,volume:0 }))
    assert.equal(audibleMovie({ ...audible,[key]:value }), false, key);
assert.equal(audibleMovie({ ...audible,volume:NaN }), false);
assert.equal(audibleMovie({ ...audible,paused:true,playbackState:2 }), true,
    'failed FMOD pause still has audible audio despite requested native pause flags');
assert.notEqual(captionIdentity(1,10,20,0),captionIdentity(1,10,30,0));
assert.notEqual(captionIdentity(1,10,20,0),captionIdentity(2,10,20,0));
console.log('PASS: movie audio policy and native timeline identities.');
