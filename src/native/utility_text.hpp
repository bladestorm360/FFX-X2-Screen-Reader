#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
namespace ffx {
struct TextPlan;
// Ready literals, copied exception bytes and numeric facts. Native only copies;
// preparation owns all decoding, joins and numeric formatting.
struct UtilityText {
    struct Part {unsigned offset=0,length=0;int64_t value=0;unsigned kind=0;};
    std::array<Part,24> parts{};
    std::array<wchar_t,2048> text{};
    unsigned count=0,used=0;bool valid=true;
    UtilityText()=default;
    UtilityText(const wchar_t* literal);
    UtilityText& literal(const wchar_t*);
    UtilityText& integer(int64_t);
    UtilityText& narrow(const char*);
    UtilityText& append(const UtilityText&);
    bool empty() const {return count==0;}
#ifdef FFX_READER_TEST
    // Semantic fixture convenience; production never renders on Native.
    size_t find(const wchar_t*) const;
#endif
};
UtilityText operator+(UtilityText,const UtilityText&);
UtilityText utilityInteger(int64_t);
UtilityText utilityError(const wchar_t*,const char*,const wchar_t* suffix=L".");
unsigned appendUtilityTextPlan(TextPlan&,const UtilityText&);
void emitUtilityText(const UtilityText&,const char* producer="utility-reply");
std::wstring renderUtilityText(const UtilityText&); // Copied-data owner only.
void installUtilityText();
}
