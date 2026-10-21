const assert=require('node:assert/strict');
const api=require('../Frida/FFX/battle_outcomes.js');
assert.equal(api.graphic(3,-1,'Larva'),'Larva: Miss');
assert.equal(api.graphic(0,0,'Larva'),'');
assert.equal(api.graphic(4,-1,'Larva'),'');
assert.equal(api.graphic(5,-1,'Larva'),'');
assert.equal(api.graphic(0,123,'Larva'),'Larva: 123 damage');
assert.equal(api.graphic(0,-1,'Tidus'),'Tidus: healed for 1');
assert.equal(api.death(false,true,'Larva'),'Larva: KO');
assert.equal(api.death(true,true,'Larva'),'');
assert.equal(api.death(false,false,'Larva'),'');
assert.equal(api.formation([{active:true,visible:true,name:'Larva'},{active:true,visible:true,name:'Larva'},
    {active:true,visible:false,name:'Hidden'},{active:false,visible:true,name:'Inactive'}]),'Enemies: Larva x2');
console.log('PASS: explicit miss graphic, numeric HP/MP, committed KO and visible formation.');

assert.equal(api.formation(Array.from({length:12},(_,i)=>({active:true,visible:true,name:i<3?['Melusine','Kusariqqu','Aerouge'][i]:i>=8?"w'":'',}))), 'Enemies: Melusine, Kusariqqu, Aerouge');
