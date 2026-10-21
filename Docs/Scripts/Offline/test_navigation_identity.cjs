const fs=require('fs'), vm=require('vm'), assert=require('assert/strict'), path=require('path');
const p=vm.createContext({});
vm.runInContext(fs.readFileSync(path.join(__dirname,'../Frida/FFX/navigation_identity.js'),'utf8'),p);
assert.equal(p.navigationIdentity('Talk to Yuna','Other','Shopkeeper','Priest','NPC'),'Yuna');
assert.equal(p.navigationIdentity('','Maechen','','Villager','NPC'),'Maechen');
assert.equal(p.navigationIdentity('','','Innkeeper','Al Bhed','NPC'),'Innkeeper');
assert.equal(p.navigationIdentity('','','','Al Bhed','NPC'),'Al Bhed');
assert.equal(p.navigationIdentity('','','','','NPC'),'NPC');
assert.equal(p.navigationIdentity('Interact with Door','','','Switch','Event'),'Door');
assert.equal(p.interactionPromptVisible(false,true,'Talk to Yuna'),'');
assert.equal(p.interactionPromptVisible(true,false,'Talk to Yuna'),'');
assert.equal(p.interactionPromptVisible(true,true,'Talk to Yuna'),'Talk to Yuna');
console.log('PASS: concise authored names, actor-local speakers/services/sprites, event labels and displayed/in-range prompt gate.');

assert.equal(p.navigationCategory("NPC","object"),"Object");
assert.equal(p.navigationCategory("NPC","unknown"),"Object");
assert.equal(p.navigationCategory("NPC","character"),"NPC");
assert.equal(p.navigationIdentity("Talk to Yuna","Yuna","","Book","NPC","object"),"Book");
