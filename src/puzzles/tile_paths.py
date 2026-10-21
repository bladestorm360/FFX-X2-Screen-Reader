"""Trace floor-control branches from original bytecode with explicit stage state.

This is a bounded offline interpreter for these control entries, not a game VM.
Unknown instructions/signatures fail rather than guessing a permitted tile.
"""
from atel_model import instructions, signatures, binding, REQS, ENDS

def trace(event, worker, stage):
    code=instructions(bytes.fromhex(event['code']))
    calls,_=signatures(); w=event['workers'][worker]
    stack=[]; regs={0x25:0,0x2a:0,0x2c:0}; values={}; requests=[]; writes=[]; native_calls=[]
    def pop():
        if not stack: raise ValueError('Unknown tile operand stack')
        return stack.pop()
    pc=w['entries'][2]
    for _ in range(1000):
        op,arg,nxt=code[pc]
        if op in (0xae,0xad,0xaf):
            stack.append((arg if arg<32768 else arg-65536) if op==0xae else w['ints' if op==0xad else 'floats'][arg])
        elif op in (0x9f,0xa2):
            element=pop() if op==0xa2 else 0; key=binding(dict(w,index=worker),arg)
            stack.append(stage if key=='L6:map:00f6a8:T5' else values.get((key,element),0))
        elif op in (0xa0,0xa1,0xa3,0xa4):
            value=pop(); element=pop() if op in (0xa3,0xa4) else 0; key=binding(dict(w,index=worker),arg)
            writes.append((key,element,value)); values[key,element]=value
        elif 1<=op<=24:
            b,a=pop(),pop()
            operations={1:lambda:a or b,2:lambda:a and b,3:lambda:a|b,4:lambda:a^b,5:lambda:a&b,
             6:lambda:a==b,7:lambda:a!=b,8:lambda:a>b,9:lambda:a<b,10:lambda:a>b,11:lambda:a<b,
             12:lambda:a>=b,13:lambda:a<=b,14:lambda:a>=b,15:lambda:a<=b,16:lambda:a|b,
             17:lambda:a&~b,18:lambda:a<<b,19:lambda:a>>b,20:lambda:a+b,21:lambda:a-b,
             22:lambda:a*b,23:lambda:a/b,24:lambda:a%b}
            stack.append(operations[op]())
        elif op in (0x25,0x2a,0x2c): regs[op]=pop()
        elif op in (0x26,0x28,0x29): stack.append(regs[{0x26:0x25,0x28:0x2a,0x29:0x2c}[op]])
        elif op==0x2b: stack.append(stack[-1])
        elif op in (0xb0,0xb1,0xb2,0xd5,0xd6,0xd7):
            condition=pop() if op in (0xd5,0xd6,0xd7) else regs[0x2a]
            if op in (0xb0,0xd5) or bool(condition)==(op in (0xb1,0xd6)):
                nxt=w['jumps'][arg]
        elif op in (0xb5,0xd8):
            signature=calls.get(arg)
            if not signature or signature['agree']!='yes': raise ValueError('Unverified tile native call')
            arguments=[pop() for _ in range(int(signature['arg_count_kari']))][::-1]
            native_calls.append(dict(pc=pc,id=arg,args=arguments))
            if op==0xb5: stack.append(0)
            else: regs[0x25]=0
        elif op in REQS:
            entry,target,level=pop(),pop(),pop(); requests.append((target,entry,level));stack.append(1)
        elif op in ENDS: return dict(requests=requests,writes=writes,calls=native_calls,endPc=pc)
        elif op in (0,0x9d,0x9e,0x55,0x77): pass # native request wait, no stack mutation
        else: raise ValueError('Unsupported tile opcode %02x at %04x'%(op,pc))
        pc=nxt
    raise ValueError('Tile trace exceeded its bound')

def compile_tiles(event):
    output=[]
    for stage in range(6):
        tiles=[]
        for local in (range(57,66) if stage==0 else range(66,84) if stage<5 else range(57,84)):
            result=trace(event,local,stage)
            req=result['requests']
            # Entry 19 increments the first-section count; 20 increments the
            # main pattern count. Stage 5 writes the native revisit panel count.
            success=any(w==38 and e=={0:19,1:23,2:24,3:22,4:25,5:20}[stage] for w,e,_ in req)
            if success: tiles.append(dict(worker=local,trace=result))
        if not tiles: raise ValueError('No permitted tiles for stage %d'%stage)
        output.append(dict(stage=stage,tiles=tiles))
    return output
