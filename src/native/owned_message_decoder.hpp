#pragma once
#include "message_text.hpp"
#include "owned_text_decoder.hpp"
#include <cwctype>
namespace ffx {
// Instructions contain ready copied bytes, never game addresses. Native token
// widths, substitutions and Primer conversion are retained by their producer.
inline MessagePage decodeOwnedMessage(const uint8_t* bytes,unsigned length,const OwnedTextContext& context) {
    MessagePage page;
    struct Boundary {int row;size_t prefix;};
    std::array<Boundary,8> stack{};unsigned depth=0;int row=-1;
    std::array<uint8_t,2> buttons{};unsigned buttonCount=0;
    if(!bytes || length>98304) return page;
    auto text=[&]() -> std::wstring& {return row<0?page.prompt:page.choices[row];};
    auto decode=[&](const uint8_t* value,unsigned count,OwnedTextMode mode) {
        const auto parts=decodeOwnedBytes(value,count,mode,context);
        std::wstring result;
        for(const auto& part:parts) {if(!result.empty()) result+=L' ';result+=part;}
        return result;
    };
    for(unsigned offset=0;offset<length;) {
        if(length-offset<2) return {};
        const auto kind=bytes[offset++],count=bytes[offset++];
        if(count>length-offset) return {};
        const auto* value=bytes+offset;offset+=count;
        if(kind==0 && !count) text()+=L'\n';
        else if(kind==1 && count==1) {
            if(buttonCount<buttons.size()) buttons[buttonCount]=value[0];
            if(buttonCount<=buttons.size()) ++buttonCount;
            const uint8_t glyph[]={11,value[0]};text()+=decode(glyph,2,OwnedTextMode::Terminated);
        }
        else if(kind==2 && count==1) {
            if(value[0]>=0x30 && value[0]<0x50) {page.selector=true;row=value[0]-0x30;}
            else if(value[0]==0xff) row=-1;
        } else if(kind==3 && count<=3) text()+=decode(value,count,OwnedTextMode::Literal);
        else if(kind==4 && count==4) text()+=decode(value,count,OwnedTextMode::Terminated);
        else if(kind==5 && !count) {
            if(depth==stack.size()) return {};
            stack[depth++]={row,text().size()};
        } else if(kind==6 && count==1) {
            if(!depth) return {};
            const auto boundary=stack[--depth];auto& expanded=text();
            if(boundary.row==row && boundary.prefix && boundary.prefix<expanded.size() &&
                iswalnum(expanded[boundary.prefix-1]) && iswalnum(expanded[boundary.prefix])) expanded.insert(boundary.prefix,1,L' ');
            // Original parser observes just the next byte at a macro boundary.
            const auto literal=value[0]>=0x2b?decode(value,1,OwnedTextMode::Literal):std::wstring{};
            if(!expanded.empty() && iswalnum(expanded.back()) && !literal.empty() && iswalnum(literal.front())) expanded+=L' ';
        } else return {};
    }
    if(depth) return {};
    auto trim=[](const std::wstring& value) {
        const auto first=value.find_first_not_of(L" \r\n\t");
        return first==std::wstring::npos?std::wstring{}:value.substr(first,value.find_last_not_of(L" \r\n\t")-first+1);
    };
    size_t begin=0;
    for(size_t end=0;end<=page.prompt.size();++end) if(end==page.prompt.size() || page.prompt[end]==L'\n') {
        auto line=trim(page.prompt.substr(begin,end-begin));if(!line.empty()) page.lines.push_back(std::move(line));begin=end+1;
    }
    page.prompt.clear();
    for(const auto& line:page.lines) {if(!page.prompt.empty()) page.prompt+=L' ';page.prompt+=line;}
    for(auto& choice:page.choices) {for(auto& c:choice) if(c==L'\n') c=L' ';choice=trim(choice);}
    // User-approved short form of the three original Jecht practice pages.
    // Match the complete decoded instruction, including its actual configured
    // glyph names; explanatory pages, selectors and other messages keep their text.
    // Only worker-owned strings are inspected or formatted here.
    if(!page.selector && buttonCount==2 && buttons[1]==0x31 && !context.glyphs[0x31].empty()) {
        constexpr unsigned directions[]={0x43,0x48,0x46};
        for(const auto glyph:directions) {
            if(buttons[0]!=glyph || context.glyphs[glyph].empty() || page.prompt!=L"Press "+context.glyphs[glyph]+L"+"+
                context.glyphs[0x31]+L" at the same time!") continue;
            std::wstring controls;
            for(unsigned bit=0;bit<4;++bit) if(glyph&(1u<<bit)) {
                const auto& name=context.glyphs[0x40+(1u<<bit)];
                if(name.empty()) {controls.clear();break;}
                if(!controls.empty()) controls+=L' ';controls+=name;
            }
            if(!controls.empty()) {
                controls+=L' ';controls+=context.glyphs[0x31];
                page.prompt=std::move(controls);page.lines={page.prompt};
            }
            break;
        }
    }
    return page;
}
}
