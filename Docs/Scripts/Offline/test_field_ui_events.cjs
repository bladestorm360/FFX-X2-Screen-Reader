const assert=require('node:assert/strict');
const {statusPage,targetText,nextDirection}=require('../Frida/FFX/field_ui_events.js');
assert.equal(statusPage(4),0); assert.equal(statusPage(6),1);assert.equal(statusPage(8),2);
for(const phase of [0,3,5,7,15,16,19]) assert.equal(statusPage(phase),-1);
assert.equal(targetText('Tidus',0,517,517),'Tidus: HP 517/517');
assert.equal(targetText('Yuna',1,20,40),'Yuna: MP 20/40');
assert.equal(targetText('Tidus',2,517,517),'Tidus: HP 517/517');
for(const [x,y,d] of [[0,10,'down'],[0,-10,'up'],[10,0,'right'],[-10,0,'left'],[10,-10,'up-right']]) assert.equal(nextDirection(x,y),d);
console.log('PASS: native status phases, displayed HP/MP policy and arrow direction mapping.');
