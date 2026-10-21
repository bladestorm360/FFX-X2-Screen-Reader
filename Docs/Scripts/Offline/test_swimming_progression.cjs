const assert = require('node:assert/strict');
const {swimmingDepthGoal, swimmingDepthRoute} = require('../Frida/FFX/swimming_progression.js');
const state = {verifiedScript:'cdsp0000',localWorker:30,entry:6,pc:60507,room:71,
    controlled:true,enabled:true,fieldControl:true,mode:2,motion:2,gravity:2,diveEnabled:true,x:-192,y:59,z:79};
assert(swimmingDepthGoal(state));
assert.deepEqual(swimmingDepthRoute(state),[{x:-192,y:59,z:79},{x:-192,y:251,z:79}]);
for (const [key,value] of Object.entries({verifiedScript:'unknown',localWorker:29,entry:1,pc:60327,
    room:288,controlled:false,enabled:false,fieldControl:false,mode:0,motion:0,gravity:0,diveEnabled:false,y:NaN}))
    assert(!swimmingDepthGoal({...state,[key]:value}),key);
assert(!swimmingDepthRoute({...state,x:NaN}).length);
console.log('PASS: offline Frida swim goal, actual depth boundary, active producer and native control guards.');
