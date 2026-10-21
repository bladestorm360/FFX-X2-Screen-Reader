// Explicit RE test controls. Appended only with --test-controls; never production.
// Commands execute once on the game's verified input event, not the RPC thread.
const liveProfile = natives.name !== 'archive';
const inputRva = liveProfile ? 0x471d70 : 0x471d10;
const controllerRva = liveProfile ? 0xf26b28 : 0xf26ae8;
const heldRva = liveProfile ? 0xf270c0 : 0xf27080;
const pressedRva = heldRva + 4;
const allowedRooms = new Set([23,122,108,214,239,306,320,318,103,45,90,80,226]);
if(!callsTo(liveProfile ? 0x46e990 : 0x46e920,68,liveProfile ? 0x46ea00 : 0x46e990))throw new Error('Test request body mismatch');
const testRequest = new NativeFunction(nativeAddress(liveProfile ? 0x46e990 : 0x46e920),'int',
    ['int','int','int','int','int'],{abi:'mscdecl',scheduling:'exclusive'});
const testPosition = new NativeFunction(nativeAddress(liveProfile ? 0x470b20 : 0x470ac0),'void',
    ['pointer','float','float','float','int'],{abi:'mscdecl',scheduling:'exclusive'});
const testWarp = new NativeFunction(nativeAddress(natives.map),'void',
    ['int','int','int'],{abi:'mscdecl',scheduling:'exclusive'});
if (!callsTo(inputRva,240,liveProfile ? 0x487dc0 : 0x487d30)) {
    throw new Error('Test input-event body differs from the verified source');
}
let pendingCommand = null, commandSequence = 0, inspection = null, testQueue=[], waitEvents=0;
let pendingLogicalMask=0, dialogOnly=false, ackEvents=0;
const messageMode = new NativeFunction(nativeAddress(liveProfile ? 0x46c8b0 : 0x46c840),'int',[],
    {abi:'mscdecl',scheduling:'exclusive'});
for(const offset of [liveProfile ? 0x488d70 : 0x488ce0,liveProfile ? 0x488e80 : 0x488df0]) {
 listeners.push(Interceptor.attach(nativeAddress(offset),{
  onLeave(result){if(dialogOnly&&pendingLogicalMask&&messageMode()===2)result.replace(result.toUInt32()|pendingLogicalMask);}
 }));
}
listeners.push(Interceptor.attach(nativeAddress(liveProfile ? 0x488c40 : 0x488bb0),{
    onEnter(args) {this.main=args[0].toInt32()===0 && args[1].toInt32()===0 &&
        nativeAddress(liveProfile ? 0xf26b30 : 0xf26af0).readU32()<2;},
    onLeave(result) {if(this.main&&pendingLogicalMask){
        const mode=dialogOnly?messageMode():1;
        if(mode===1){result.replace(result.toUInt32()|pendingLogicalMask);pendingLogicalMask=0;}
        else if(mode===0){pendingLogicalMask=0;}
    }}
}));
const testInteger = new NativeFunction(nativeAddress(natives.integer),'void',
    ['pointer','int','int','int','int'],{abi:'mscdecl',scheduling:'exclusive'});

function scriptName(chunk) {
    const name = new Uint8Array(chunk.add(chunk.add(0x0c).readU32()).readByteArray(8));
    const end = name.indexOf(0);
    return String.fromCharCode(...name.slice(0,end < 0 ? 8 : end));
}

function inspectWorkers(includePointers=false) {
    const controller = nativeAddress(controllerRva).readPointer();
    if (!range(controller,0x58)) throw new Error('No readable ATEL controller');
    const count = controller.add(0x0c).readU16();
    const groups = [controller.add(0x16).readU16(),controller.add(0x14).readU16(),
        controller.add(0x18).readU16()];
    const normal = count - groups.reduce((a,b)=>a+b,0);
    if (normal < 0 || count > 500) throw new Error('Invalid controller worker counts');
    const lengths = [normal,...groups];
    const strides = [0xb58,0x558,0x5b8,0x2e8];
    const result = [];
    let worker = controller.add(0x1c).readPointer(), index = 0;
    for(let group=0;group<lengths.length;++group) {
        for(let j=0;j<lengths[group];++j,++index,worker=worker.add(strides[group])) {
            if (!range(worker,0x34)) throw new Error('Unreadable live worker table');
            const header=worker.readPointer(), chunk=worker.add(4).readPointer();
            if (!range(header,0x34)||!range(chunk,0x38)) continue;
            const name=scriptName(chunk);
            const local=worker.add(0x2e).readU16();
            const type=header.readU16();
            const geometry = type===5||type===6 ? 0x284 : 0x558;
            let position=null;
            if([1,2,3,5,6].includes(type)&&range(worker.add(geometry),12)) {
                const p=[0,4,8].map(o=>worker.add(geometry+o).readFloat());
                if(p.every(Number.isFinite))position=p;
            }
            result.push({index,local,type,script:name,position,yaw:position?worker.add(geometry+0x20).readFloat():null,interaction:worker.add(0xab).readU8(),
                ...(includePointers?{pointer:worker}:{} )});
            if (!snapshot && manifest.scripts[name]?.fields.length) {
                const ctx=context(worker);
                if(ctx)publish(ctx,{type:'explicit-test-inspection',pc:sourcePc(ctx)});
            }
        }
    }
    inspection={generation,thread:Process.getCurrentThreadId(),messageMode:messageMode(),controllerFlags:controller.readU8(),player:controller.add(10).readU16(),workers:result};
    send({type:'test-inspection',inspection});
    return inspection;
}

listeners.push(Interceptor.attach(nativeAddress(inputRva),{
    onEnter(){if(ackEvents>0&&--ackEvents===0)pendingLogicalMask=0;},
    onLeave() {
        if(waitEvents>0){--waitEvents;return;}
        if(!pendingCommand&&testQueue.length)pendingCommand=testQueue.shift();
        if(!pendingCommand)return;
        const command=pendingCommand;
        pendingCommand=null;
        waitEvents=command.delay??60;
        try {
            if(command.action==='warp') {
                const controller=nativeAddress(controllerRva).readPointer();
                if(!range(controller,0x214))throw new Error('No warp-ready ATEL controller');
                const flags=controller.readU8();
                // The native dispatcher consumes bit 80 for a requested jump.
                // This temporary RE warp is not a test of a completion gate.
                controller.writeU8(flags|0x80);
                invalidate('explicit-test-warp');
                try { testWarp(command.room,command.spawn,0); }
                finally { if(controller.readU8()&0x80)controller.writeU8(flags); }
            }
            else if(command.action==='request') {
                const data=inspectWorkers(true);
                const target=data.workers.find(w=>w.local===command.worker);
                if(!target||!manifest.scripts[target.script])throw new Error('No trial target');
                const ctx=context(target.pointer);
                const count=ctx.header.add(8).readU16();
                if(command.entry>=count)throw new Error('Entry outside worker table');
                const result=testRequest(data.player,target.index,0,7,command.entry);
                send({type:'test-request-result',result,worker:command.worker,entry:command.entry});
                inspection=null;
            }
            else if(command.action==='values') {
                const data=inspectWorkers(true);
                const target=data.workers.find(w=>w.local===command.worker);
                if(!target||!manifest.scripts[target.script])throw new Error('No trial target');
                const ctx=context(target.pointer), values=[];
                for(let i=0;i<ctx.variableCount;++i) {
                    const declaration=ctx.variables.add(i*8), low=declaration.readU32(), high=declaration.add(4).readU32();
                    const count=high&0xffff,format=low>>>28,scope=(low>>>25)&7;
                    if(count>200||format>6||scope>6)continue;
                    const array=[];
                    for(let j=0;j<count;++j)array.push(readScalar(getAddress(ctx.worker,i,j),format));
                    values.push({index:i,low,high,value:count===1?array[0]:array});
                }
                send({type:'test-values',script:ctx.name,worker:command.worker,values});
                inspection=null;
            }
            else if(command.action==='move') {
                const data=inspectWorkers(true);
                const player=data.workers.find(w=>w.local===data.player);
                if(!player||!manifest.scripts[player.script])throw new Error('No trial player');
                if(player.type===4)throw new Error('Player has no native position');
                testPosition(player.pointer,...command.position,1);
                inspection=null;
            }
            else if(command.action==='story') {
                const data=inspectWorkers(true);
                const target=data.workers.find(w=>w.local===0 && manifest.scripts[w.script]);
                if(!target)throw new Error('No trial fixture worker');
                const ctx=context(target.pointer);
                let index=-1;
                for(let i=0;i<ctx.variableCount;++i) {
                    const d=ctx.variables.add(i*8);
                    if(d.readU32()===0x20000a00&&d.add(4).readU32()===1){index=i;break;}
                }
                if(index<0)throw new Error('No verified story declaration');
                testInteger(ctx.worker,index,0,command.value,0);
                send({type:'test-story-seed',value:command.value,status:'Temporary fixture context; autosaves possible'});
            }
            else if(command.action==='press'||command.action==='ack') {
                pendingLogicalMask=command.mask; dialogOnly=command.action==='ack'; ackEvents=60;
            }
            else if(command.action==='inspect')inspectWorkers();
            else throw new Error('Unknown test command');
            send({type:'test-command-complete',id:command.id,command});
        }catch(e){send({type:'test-command-error',id:command.id,error:String(e)});}
    }
}));
function validateTestCommand(command) {
    if(command.action==='warp' && (!allowedRooms.has(command.room)||command.spawn!==0)) {
        throw new Error('Only registered trial/title rooms and entrance zero are allowed');
    }
    if(['press','ack'].includes(command.action) && (!Number.isInteger(command.mask)||command.mask<0||command.mask>65535)) {
        throw new Error('Invalid one-event controller mask');
    }
    if(command.action==='move' && (!Array.isArray(command.position)||command.position.length!==3||
        !command.position.every(x=>Number.isFinite(x)&&Math.abs(x)<5000)))throw new Error('Invalid test position');
    if(['request','values'].includes(command.action)&&(!Number.isInteger(command.worker)||command.worker<0||command.worker>200))throw new Error('Invalid trial worker');
    if(command.action==='request'&&(!Number.isInteger(command.entry)||command.entry<1||command.entry>10))throw new Error('Only explicit trial entries allowed');
    if(command.action==='story'&&(!Number.isInteger(command.value)||command.value<0||command.value>2999))throw new Error('Invalid unsaved story fixture');
    if(!['warp','press','inspect','move','request','values','story','ack'].includes(command.action))throw new Error('Unknown test action');
    if(command.delay!==undefined&&(!Number.isInteger(command.delay)||command.delay<0||command.delay>3600))throw new Error('Invalid native-event delay');
    return {...command,id:++commandSequence};
}
rpc.exports.testCommand=function(command){
    if(pendingCommand||testQueue.length)throw new Error('A test command is already pending');
    pendingCommand=validateTestCommand(command);return pendingCommand.id;
};
rpc.exports.testCommands=function(commands){
    if(pendingCommand||testQueue.length)throw new Error('A test sequence is already pending');
    if(!Array.isArray(commands)||commands.length<1||commands.length>800)throw new Error('Invalid explicit test sequence');
    testQueue=commands.map(validateTestCommand);return testQueue.map(c=>c.id);
};
rpc.exports.testInspection = function(){return inspection;};
send({type:'test-controls-ready',profile:natives.name,status:'Temporary RE test controls; autosaves possible'});
