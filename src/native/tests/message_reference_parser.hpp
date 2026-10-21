#pragma once
#include "../message_text.hpp"
#include <cwctype>
namespace ffx::fixture_reference {
// Retained pre-worker parser logic for semantic comparison only. Never linked
// into the native mod; getters/address reads below are synthetic fixture APIs.
inline void parse(unsigned slot,uintptr_t source,MessagePage& page,int& row,unsigned depth,size_t& budget) {
    if(!source || depth>=8) return;
    for(size_t offset=0;offset<0x1000 && budget;) {
        uint8_t code=0,payload=0;const auto token=source+offset;
        if(!copy(token,&code,1) || code<=2) break;
        size_t width=1;
        if(code==4 || (code>=6 && code<=14) || code==16 || (code>=18 && code<=47)) {
            if(!copy(token+1,&payload,1)) break;width=code==4 && payload<0x30?3:2;
        }
        uint8_t bytes[3]{};
        if(width>budget || offset+width>0x1000 || !copy(token,bytes,width)) break;
        budget-=width;auto& text=row<0?page.prompt:page.choices[row];
        if(code==3) text+=L'\n';
        else if(code==11) text+=buttonGlyph(payload);
        else if(code==16 && payload>=0x30 && payload<0x50) {page.selector=true;row=payload-0x30;}
        else if(code==16 && payload==0xff) row=-1;
        else if(code>=18 && code<=35 && code!=20 && code!=21 && code!=24) {
            using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t);
            const auto resolved=reinterpret_cast<Getter>(address(0x4b6610))(slot,token);
            if(resolved && resolved!=token) {
                const auto prefix=text.size();parse(slot,resolved,page,row,depth+1,budget);
                auto& expanded=row<0?page.prompt:page.choices[row];
                if(&expanded==&text && prefix && prefix<text.size() && iswalnum(text[prefix-1]) && iswalnum(text[prefix])) text.insert(prefix,1,L' ');
                uint8_t next=0;copy(token+width,&next,1);
                const auto literal=next>=0x2b?join(decode(token+width,1,TextMode::Literal),L""):L"";
                if(!expanded.empty() && iswalnum(expanded.back()) && !literal.empty() && iswalnum(literal.front())) expanded+=L' ';
            }
        } else if(code>=0x2b) {
            const auto window=read<uintptr_t>(address(0x14676b0)+slot*4);
            if(read<uint16_t>(window+0x10)==0x4000) {
                uint8_t converted[4]{};using Convert=int(__cdecl*)(uintptr_t,uint8_t*);
                reinterpret_cast<Convert>(address(0x506780))(token,converted);
                text+=join(decode(reinterpret_cast<uintptr_t>(converted),sizeof(converted)),L"");
            } else text+=join(decode(token,width,TextMode::Literal),L"");
        }
        offset+=width;
    }
}
inline MessagePage messagePage(unsigned slot,uintptr_t source) {
    MessagePage page;if(slot>=8) return page;int row=-1;size_t budget=0x4000;parse(slot,source,page,row,0,budget);
    size_t begin=0;
    for(size_t end=0;end<=page.prompt.size();++end) if(end==page.prompt.size() || page.prompt[end]==L'\n') {
        auto line=clean(page.prompt.substr(begin,end-begin));if(!line.empty()) page.lines.push_back(std::move(line));begin=end+1;
    }
    page.prompt=join(page.lines,L" ");
    for(auto& choice:page.choices) {for(auto& c:choice) if(c==L'\n') c=L' ';choice=clean(choice);}
    return page;
}
}
