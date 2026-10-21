// Execute original bytes and original tables. Rendering, audio, loading,
// random values and scheduler time are controlled dependencies; no game runs.
#include "jecht_round_data.hpp"
namespace {
const RoundWorker& roundWorker(unsigned local) {
    for(const auto& w:roundWorkers)if(w.variant==which && w.local==local)return w;
    check(false,"original round worker");return roundWorkers[0];
}
struct RoundRequest {unsigned local,entry;};
struct RoundHost {
    unsigned random=0,mode=0,axis=128,shows=0,awards=0,queries=0,left=0,right=0,positions=0;
    std::vector<RoundRequest> pending;
    unsigned choose(unsigned limit) {
        check(limit==4 || limit==5,"original random selector bounds");
        if(mode==0)return limit==5?3:0; // center first: the reported stall.
        if(mode==1)return (limit==5?left++:right++)%limit;
        random=random*1664525u+1013904223u;return (random>>16)%limit;
    }
};
struct RoundVm {
    const RoundWorker& w;RoundHost& host;
    unsigned entry=0,pc=0,wait=0;bool done=false;
    int a=0,y=0;std::vector<int> stack;
    RoundVm(unsigned local,unsigned e,RoundHost& h):w(roundWorker(local)),host(h),entry(e),pc(w.entries[e]) {
        // Compare generated source tables with the actual retained EBP bytes.
        location(local,e,pc);const auto header=read<uintptr_t>(ptr(worker));
        for(unsigned i=0;i<w.intCount;++i)check(read<int>(chunk()+read<unsigned>(header+0x18)+i*4)==w.ints[i],"original EBP integer table agrees with archived round data");
        for(unsigned i=0;i<w.jumpCount;++i)check(read<unsigned>(chunk()+read<unsigned>(header+0x24)+i*4)==w.jumps[i],"original EBP jump table agrees with archived round data");
    }
    int pop() {check(!stack.empty(),"original round stack bound");const auto v=stack.back();stack.pop_back();return v;}
    unsigned format(unsigned index) {return read<unsigned>(chunk()+read<unsigned>(read<uintptr_t>(ptr(worker))+0x14)+index*8)>>28;}
    int get(unsigned index,unsigned element=0) {
        const auto f=format(index),p=variable(ptr(worker),static_cast<int>(index),0)+element*(f==5?4:1);
        return f==5?read<int>(p):read<uint8_t>(p);
    }
    void putValue(unsigned index,unsigned element,int v) {
        const auto f=format(index),p=variable(ptr(worker),static_cast<int>(index),0)+element*(f==5?4:1);
        if(f==5)write<int>(p,v);else write<uint8_t>(p,static_cast<uint8_t>(v));
    }
    int call(unsigned id,const std::vector<int>& args) {
        if(id==0){check(args[0]>0,"original positive wait");wait=static_cast<unsigned>(args[0]);return 0;}
        if(id==0xa6)return static_cast<int>(host.choose(static_cast<unsigned>(args[0])));
        if(id==0x50 || id==0x51 || id==0x122) {
            Event e;e.args[0]=ptr(worker);e.epoch=epoch;e.parentRva=0x464180;e.result=id==0x122?host.axis:0;
            for(unsigned i=0;i<std::size(callIds);++i)if(callIds[i]==id) {e.rva=callRvas[i];audioEvent(e.rva,e,false);}
            ++host.queries;return static_cast<int>(e.result);
        }
        if(id==0xdf){Event e;e.args[0]=static_cast<unsigned>(args[0]);e.epoch=epoch;e.parentRva=0x464180;audioEvent(jecht_rva::Sound,e,true);return args[0];}
        if(id==0x6a){Event e;e.args[0]=ptr(worker);e.epoch=epoch;e.parentRva=0x464180;
            audioEvent(jecht_rva::Show,e,true);audioEvent(jecht_rva::Show,e,false);++host.shows;
            host.positions|=1u<<(entry-6+(w.local==jechtScripts[which].local+2?5:0));return args[0];}
        if(id==0x25b){check(args[0]==8,"original Jecht achievement");++host.awards;return 0;}
        // Original non-input dependencies only: ready assets, visual messages,
        // effect playback/teardown and fade. Their results do not change score.
        check(id==0xdc || id==0xe0 || id==0x17a || id==0x17b || id==0x4003 ||
            id==0x64 || id==0x65 || id==0x66 || id==0x9d || id==0x6b,"unmodeled original round dependency");
        return 0;
    }
    void step() {
        if(done)return;if(wait && --wait)return;
        location(w.local,entry,pc);Event root;root.args[0]=ptr(worker);root.epoch=epoch;audioEvent(0x464180,root,true);
        struct End {Event& e;~End(){audioEvent(0x464180,e,false);}} end{root};
        const auto code=chunk()+read<unsigned>(chunk()+0x30);
        for(unsigned work=0;work<4000;++work) {
            location(w.local,entry,pc);const auto op=read<uint8_t>(code+pc);const auto arg=read<uint16_t>(code+pc+1);
            const auto next=pc+((op&0x80)?3:1);unsigned target=next;
            switch(op) {
            case 0xae:stack.push_back(static_cast<int16_t>(arg));break;
            case 0xad:check(arg<w.intCount,"original integer index");stack.push_back(w.ints[arg]);break;
            case 0x9f:stack.push_back(get(arg));break;
            case 0xa2:{const auto element=static_cast<unsigned>(pop());stack.push_back(get(arg,element));break;}
            case 0xa0:{const auto v=pop();putValue(arg,0,v);break;}
            case 0xa3:{const auto v=pop();putValue(arg,static_cast<unsigned>(pop()),v);break;}
            case 0x19:{const auto v=pop();stack.push_back(!v);break;}
            case 0x25:a=pop();break;
            case 0x29:stack.push_back(y);break;
            case 0x2b:{const auto v=pop();stack.push_back(v);stack.push_back(v);break;}
            case 0x2c:y=pop();break;
            case 0xb0:case 0xd6:case 0xd7:{
                check(arg<w.jumpCount,"original branch table index");const auto take=op==0xb0?true:op==0xd6?pop()!=0:pop()==0;
                if(take)target=w.jumps[arg];break;}
            case 0x36:{const auto e=static_cast<unsigned>(pop()),local=static_cast<unsigned>(pop()),level=static_cast<unsigned>(pop());
                check(level==1 && host.pending.size()<2,"original bounded priority1 requests");host.pending.push_back({local,e});stack.push_back(1);break;}
            case 0xb5:case 0xd8:{
                const auto sig=std::find_if(std::begin(roundCalls),std::end(roundCalls),[&](const auto& c){return c.id==arg;});
                check(sig!=std::end(roundCalls),"original call arity");std::vector<int> args(sig->count);
                for(unsigned i=sig->count;i;--i)args[i-1]=pop();const auto result=call(arg,args);
                if(op==0xb5)stack.push_back(result);else a=result;break;}
            case 0x3c:done=true;break;
            default:{const auto right=pop(),left=pop();int result=0;
                switch(op){case 1:result=left||right;break;case 2:result=left&&right;break;case 3:result=left|right;break;
                case 6:result=left==right;break;case 0xa:result=left>right;break;case 0xb:result=left<right;break;case 0x14:result=left+right;break;
                default:fprintf(stderr,"Original VM unsupported opcode=%x pc=%u\n",op,pc);check(false,"unsupported executed original opcode");}
                stack.push_back(result);break;}
            }
            pc=target;if(done || wait)return;
        }
        check(false,"original per-frame work bound");
    }
};
void fullRoundFixtures(unsigned variant) {
    const auto& s=jechtScripts[variant];
    // Different clocks, repeated centers, both slots, all random positions and
    // neutral/deflected physical axes. Run the original award/learn branches.
    for(unsigned mode=0;mode<4;++mode)for(unsigned clock:{33u,4u,0u}) {
        load(variant);std::fill(saved.begin(),saved.end(),uint8_t{0});RoundHost host;host.mode=mode;host.random=7+mode;
        host.axis=mode<2?128:mode==2?0:255;
        RoundVm game(s.local,s.entry,host);const auto began=ticks;std::vector<unsigned> accepted;
        unsigned frames=0;
        for(;frames<700 && !game.done;++frames) {
            ticks=began+frames*clock;game.step();location(s.local,s.entry,game.pc);
            const auto currentScore=game.get(roundScores[variant]);
            if(currentScore>static_cast<int>(accepted.size())) {
                check(currentScore==static_cast<int>(accepted.size()+1),"each original success contributes exactly one point");
                accepted.push_back(static_cast<unsigned>(game.get(s.timer)));
            }
            auto requests=std::move(host.pending);host.pending.clear();
            for(const auto& r:requests){RoundVm cue(r.local,r.entry,host);cue.step();check(cue.done,"original visible cue returns without a synthetic wait");}
        }
        location(s.local,s.entry,s.entryPc);
        const bool learned=(saved[0x1ec+0x204]&8)!=0;
        const auto score=game.get(roundScores[variant]);
        if(!game.done || host.awards!=1 || !learned || score!=11) {
            fprintf(stderr,"ROUND failed script=%u mode=%u msPerTick=%u score=%d frames=%u shows=%u awards=%u learned=%u timer=%d\n",
                variant,mode,clock,score,frames,host.shows,host.awards,learned,game.get(s.timer));
            check(false,"complete original Jecht round must stop at exactly eleven successes and learn the shot");
        }
        check(accepted.size()==11 && accepted.front()>=23 && accepted.back()<300,"all eleven answers fit the original ten-second timer");
        unsigned minGap=300;for(unsigned i=1;i<accepted.size();++i)minGap=std::min(minGap,accepted[i]-accepted[i-1]);
        check(minGap>=23,"original accepted successes are spaced at least23 simulation steps, about750ms");
        check(host.shows>=11 && host.queries>100,"full round crosses the original eleven-success requirement");
        check(plans==0 && sounds==0,"auto-success intentionally emits no manual control prompts");
        if(mode==1)check(host.positions==0x1ff,"complete original round covers all nine positions");
        printf("PASS: original round script=%u mode=%u msPerTick=%u score=%d shows=%u queries=%u award=1 learned=1 frames=%u first=%u last=%u minGap=%u.\n",
            variant,mode,clock,score,host.shows,host.queries,frames,accepted.front(),accepted.back(),minGap);
    }
    load(variant);std::fill(saved.begin(),saved.end(),uint8_t{0});automatic=false;
    RoundHost manual;manual.mode=1;manual.random=7;RoundVm game(s.local,s.entry,manual);const auto began=ticks;
    for(unsigned frame=0;frame<700 && !game.done;++frame) {
        ticks=began+frame*33;game.step();auto requests=std::move(manual.pending);manual.pending.clear();
        for(const auto& r:requests){RoundVm cue(r.local,r.entry,manual);cue.step();}
    }
    location(s.local,s.entry,s.entryPc);
    check(game.done && manual.awards==0 && game.get(roundScores[variant])==0,"manual mode never solves the game by itself");
    check(manual.shows==2 && plans==2 && sounds==2,"both original manual memory events produce control speech/beeps with assistance off");
    const auto axis=*std::find_if(std::begin(jechtQueries),std::end(jechtQueries),[&](const auto& q){return q.script==variant && q.call==0x122;});
    const auto physical=queryAt(axis,255);check(!physical.overrideResult && physical.result==255,"manual physical axes pass through unchanged");
    printf("PASS: original manual round script=%u, two visible control speech/beep requests, no automatic score/award, physical axis unchanged.\n",variant);
}
}
