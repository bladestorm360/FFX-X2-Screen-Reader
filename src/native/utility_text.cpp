#include "utility_text.hpp"
#include "text_plan.hpp"
#include "utilities.hpp"
#include <cstring>
namespace ffx {
UtilityText::UtilityText(const wchar_t* value) {literal(value);}
UtilityText& UtilityText::literal(const wchar_t* value) {
    if(!value || !*value) return *this;
    if(count==parts.size()) {valid=false;return *this;}
    auto& p=parts[count++];p.offset=used;
    while(*value && used+1<text.size()) {text[used++]=*value++;++p.length;}
    text[used]=0;if(*value) valid=false;return *this;
}
UtilityText& UtilityText::integer(int64_t value) {
    if(count==parts.size()) valid=false;
    else {auto& p=parts[count++];p.kind=1;p.value=value;}
    return *this;
}
UtilityText& UtilityText::narrow(const char* value) {
    if(!value || !*value) return *this;
    if(count==parts.size()) {valid=false;return *this;}
    auto& p=parts[count++];p.kind=2;p.offset=used;
    // Store each ready byte unchanged in one cell. Preparation performs the
    // original unsigned-byte widening; this is not game text decoding.
    while(*value && used+1<text.size()) {text[used++]=static_cast<unsigned char>(*value++);++p.length;}
    text[used]=0;if(*value) valid=false;return *this;
}
UtilityText& UtilityText::append(const UtilityText& from) {
    if(!from.valid || from.count>from.parts.size() || from.used>=from.text.size() || count+from.count>parts.size() || used+from.used>=text.size()) {valid=false;return *this;}
    const unsigned base=used;std::memcpy(text.data()+used,from.text.data(),from.used*sizeof(wchar_t));used+=from.used;text[used]=0;
    for(unsigned i=0;i<from.count;++i) {auto p=from.parts[i];p.offset+=base;parts[count++]=p;}
    return *this;
}
UtilityText operator+(UtilityText left,const UtilityText& right) {left.append(right);return left;}
UtilityText utilityInteger(int64_t value) {UtilityText result;result.integer(value);return result;}
UtilityText utilityError(const wchar_t* prefix,const char* reason,const wchar_t* suffix) {UtilityText result(prefix);result.narrow(reason).literal(suffix);return result;}
std::wstring renderUtilityText(const UtilityText& value) {
    std::wstring result;if(!value.valid || value.count>value.parts.size() || value.used>=value.text.size()) return result;
    for(unsigned i=0;i<value.count;++i) {const auto& p=value.parts[i];if(p.kind==1) result+=std::to_wstring(p.value);
        else {if(p.kind>2 || p.offset>value.used || p.length>value.used-p.offset) return {};result.append(value.text.data()+p.offset,p.length);}}
    return result;
}
#ifdef FFX_READER_TEST
size_t UtilityText::find(const wchar_t* value) const {return renderUtilityText(*this).find(value);}
#endif
}
