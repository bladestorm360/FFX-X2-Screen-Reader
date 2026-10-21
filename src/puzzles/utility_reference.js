// Opt-in Frida RE reference for the native continuation bridge. Never loaded
// by dinput8; it produces no speech and performs no action until queue() is
// explicitly invoked. Append alongside state_reader.js/live_driver.js.
function createUtilityReference() {
    const base = Process.getModuleByName('FFX.exe').base;
    const live = natives.name !== 'archive';
    const initialize = base.add(live ? 0x4690a0 : 0x469040);
    const input = base.add(live ? 0x471d70 : 0x471d10);
    let queued = null, pending = null;
    const hooks = [];
    hooks.push(Interceptor.attach(initialize, {
        onEnter(args) { this.worker=args[0]; this.signal=args[1]; },
        onLeave() {
            if (!pending || !this.worker.equals(pending.worker) || !this.signal.equals(pending.signal)) return;
            const command=pending; pending=null;
            if (command.generation !== generation) return;
            const level=this.signal.add(14).readU8()&15;
            const type=this.worker.readPointer().readU8();
            if (level>(type===5||type===6?1:8)) throw new Error('Unsupported priority');
            const chunk=this.worker.add(4).readPointer();
            const thread=this.worker.add(0x12c+level*0x4c);
            thread.add(0x18).writePointer(chunk.add(chunk.add(0x30).readU32()+command.pc));
            thread.add(0x24).writeS32(command.regY||0);
            send({type:'explicit-utility-continuation',worker:command.local,pc:command.pc});
        }
    }));
    hooks.push(Interceptor.attach(input, {
        onLeave() {
            if (!queued) return;
            const command=queued; queued=null;
            if (command.generation!==generation || pending) throw new Error('Stale/busy utility command');
            const state=inspectWorkers(true);
            const target=state.workers.find(w=>w.script===command.script&&w.local===command.local);
            if (!target) throw new Error('Worker not loaded');
            const ctx=context(target.pointer);
            if (!ctx || command.pc<0 || command.pc>=ctx.descriptor.code_length) throw new Error('Unverified script/PC');
            const entryTable=ctx.header.add(0x20).readU32();
            if (ctx.chunk.add(entryTable+command.entry*4).readU32()!==command.entryPc) throw new Error('Entry identity changed');
            pending={...command,worker:target.pointer,signal:target.pointer.add(0x88).readPointer()};
            if (pending.signal.isNull() || testRequest(state.player,target.index,0,7,command.entry)!==1) {
                pending=null; throw new Error('Request refused');
            }
        }
    }));
    return {
        queue(command) {
            if (queued||pending) throw new Error('Utility request already pending');
            queued={...command,generation};
        },
        dispose() { queued=null;pending=null;hooks.forEach(h=>h.detach()); }
    };
}

// Offline prototype of natural-interaction substitution. The caller supplies
// the audited native addresses and copied, hashed per-worker contracts. This
// factory is never called by production, and is not attached during this task.
function createPuzzleInteractionReference(config) {
    let armed = false, active = null;
    const hooks = [];
    hooks.push(Interceptor.attach(config.initialize, {
        onEnter(args) { this.worker=args[0];this.signal=args[1]; },
        onLeave() {
            if (!armed || active) return;
            const state=config.context(this.worker,this.signal);
            if (!state || !state.verified) return;
            const action=config.actions.find(a=>a.worker===state.local&&a.entry===state.entry);
            if (!action) return;
            if (state.entryPc!==action.entryPc || state.code.add(action.returnPc).readU8()!==0x3c)
                throw new Error('Changed interaction entry or original RET');
            const level=(state.type===5||state.type===6)?Math.min(state.level,1):Math.min(state.level,8);
            this.worker.add(0x12c+level*0x4c+0x18).writePointer(state.code.add(action.returnPc));
            active={worker:this.worker,signal:this.signal,generation:state.generation};
        }
    }));
    hooks.push(Interceptor.attach(config.finish, {
        onEnter(args) { this.worker=config.worker(args[0].toInt32());this.signal=this.worker.add(0x90).readPointer(); },
        onLeave() {
            if (!active||!active.worker.equals(this.worker)||!active.signal.equals(this.signal)) return;
            const finished=active;active=null;
            config.completed(finished); // queue; the native input event owns the next REQ
        }
    }));
    return {arm(value){armed=!!value;},dispose(){armed=false;active=null;hooks.forEach(h=>h.detach());}};
}
