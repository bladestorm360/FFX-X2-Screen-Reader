const assert=require('node:assert/strict');
const api=require('../Frida/FFX/opening_input_voice.js');
const opening=new api.Opening();
const slots=['Melusine','Kusariqqu','Aerouge'].map(name=>({name,active:true,actor:true,visible:false}));
assert.deepEqual(opening.message('Pre-emptive strike!'),[]);
slots[0].visible=true; assert.deepEqual(opening.appear(slots),[]);
slots.forEach(s=>s.visible=true);
slots[8]={name:"w'",active:true,actor:true,visible:true};
assert.deepEqual(opening.appear(slots),['Enemies: Melusine, Kusariqqu, Aerouge','Pre-emptive strike!']);
assert.deepEqual(opening.appear(slots),[]);
assert.deepEqual(api.pad(['left','rightTrigger','A','leftStick'],['left','rightTrigger']),['A','leftStick']);
assert.deepEqual(api.dialogue(true,false,'Caption first'),{pending:'Caption first'});
assert.deepEqual(api.dialogue(true,true,'Voice starts'),{text:'Voice starts',voiced:true});
console.log('PASS: bounded visible opening before messages, reserved pad ownership, subtitle-before-voice event prototype.');

assert.equal(api.walkable(1),false); assert.equal(api.walkable(0),true);

const guide=new api.Guide(13); assert.equal(guide.move(-1),0); assert.equal(guide.move(1),1);
assert.equal(guide.checkpoint(5),6); assert.equal(guide.selected,1);
assert.equal(guide.move(99),12); assert.equal(guide.move(-99),0);
