#pragma once
#include "atel.hpp"
#include "callback_guard_facts.hpp"
#include <optional>
#include <cstring>
namespace ffx {
// Original callbackAvailable's 256-instruction pure-guard policy, over owned
// bytecode/tables and copied values. Unknown operations retain interactions;
// no simulated game call or write occurs. The caller qualifies readiness first.
template<class SavedValue> bool callbackAvailableOwned(const AtelScript& script,unsigned local,unsigned entry,SavedValue saved,
    const std::vector<uint8_t>* guardCode=nullptr) {
    if(local>=script.workers.size()) return true;
    const auto& worker=script.workers[local];if(entry>=worker.entries.size()) return true;
    const auto& code=guardCode?*guardCode:script.code;
    const auto length=static_cast<unsigned>(code.size());if(!length) return true;
    unsigned pc=worker.entries[entry];
    using Value=std::optional<int>;std::vector<int> stack;Value a,x,y;
    const auto pop=[&]() -> Value {if(stack.empty()) return {};const int value=stack.back();stack.pop_back();return value;};
    for(unsigned n=0;n<256;++n) {
        if(pc>=length) return true;const auto op=code[pc];uint16_t operand=0;
        const unsigned size=op&0x80?3u:1u;if(size>length-pc) return true;
        if(size==3) std::memcpy(&operand,code.data()+pc+1,2);pc+=size;
        if(op==0x00) continue;
        if(op==0x34 || (op>=0x3c && op<=0x40) || op==0x54) return false;
        if(op==0xae) stack.push_back(static_cast<int16_t>(operand));
        else if(op==0xad) {if(operand>=worker.ints.size()) return true;stack.push_back(worker.ints[operand]);}
        else if(op==0x9f || op==0xa2) {
            // The original Baaj seals load the same saved byte through an
            // indexed declaration with element0. Nonzero/dynamic elements
            // remain unknown: this scalar-fact provider does not own arrays.
            if(op==0xa2) {const auto index=pop();if(!index || *index!=0) return true;}
            const auto value=saved(operand);if(!value) return true;stack.push_back(*value);
        }
        else if(op==0x25 || op==0x2a || op==0x2c) {
            const auto value=pop();if(!value) return true;(op==0x25?a:op==0x2a?x:y)=value;
        } else if(op==0x26 || op==0x28 || op==0x29) {
            const auto value=op==0x26?a:op==0x28?x:y;if(!value) return true;stack.push_back(*value);
        } else if(op==0x2b) {if(stack.empty()) return true;stack.push_back(stack.back());}
        else if(op==0x19 || op==0x1c) {const auto value=pop();if(!value) return true;stack.push_back(op==0x19?!*value:~*value);}
        else if(op>=1 && op<=0xf) {
            const auto right=pop(),left=pop();if(!left || !right) return true;
            const int l=*left,r=*right;int value=0;
            switch(op) {
            case 1:value=l || r;break;case 2:value=l && r;break;
            case 3:value=l|r;break;case 4:value=l^r;break;case 5:value=l&r;break;
            case 6:value=l==r;break;case 7:value=l!=r;break;
            case 8:value=static_cast<unsigned>(l)>static_cast<unsigned>(r);break;
            case 9:value=static_cast<unsigned>(l)<static_cast<unsigned>(r);break;
            case 10:value=l>r;break;case 11:value=l<r;break;
            case 12:value=static_cast<unsigned>(l)>=static_cast<unsigned>(r);break;
            case 13:value=static_cast<unsigned>(l)<=static_cast<unsigned>(r);break;
            case 14:value=l>=r;break;case 15:value=l<=r;break;
            }
            stack.push_back(value);
        } else if(op==0xb0 || op==0xb1 || op==0xb2 || op==0xd5 || op==0xd6 || op==0xd7) {
            bool jump=op==0xb0 || op==0xd5;
            if(!jump) {
                const auto condition=op>=0xd5?pop():x;if(!condition) return true;
                if(op>=0xd5) x=condition;jump=(op==0xb1 || op==0xd6)?*condition!=0:*condition==0;
            } else if(op==0xd5) {x=pop();if(!x) return true;}
            if(jump) {if(operand>=worker.jumps.size()) return true;pc=worker.jumps[operand];}
        } else if(op==0xd8 && unavailableCallbackEpilogue(script.name.c_str(),
            script.codeLength?script.codeLength:static_cast<unsigned>(script.code.size()),script.checksum,local,entry,pc-size,operand)) {
            // Recognize the documented no-activation branch without executing
            // or generally skipping any native call. Other calls stay unknown.
            return false;
        } else return true;
    }
    return true;
}
}
