#include "log_capture.hpp"
#include <algorithm>
namespace ffx {
void formatLogCapture(LogCapture& packet,char* output,size_t capacity) {
    unsigned argument=0;
    // Format interpretation is worker-only. A pointer conversion keeps its
    // opaque original address; string conversions use only copied storage.
    for(unsigned i=0;packet.format[i] && i+1<packet.format.size();++i) {
        if(packet.format[i]!='%') continue;
        if(packet.format[++i]=='%') continue;
        while(packet.format[i] && std::strchr("-+ #0",packet.format[i])) ++i;
        if(packet.format[i]=='*') {++argument;++i;}else while(packet.format[i]>='0' && packet.format[i]<='9') ++i;
        if(packet.format[i]=='.') {++i;if(packet.format[i]=='*') {++argument;++i;}else while(packet.format[i]>='0' && packet.format[i]<='9') ++i;}
        while(packet.format[i] && std::strchr("hlLjztIw1234567890",packet.format[i])) ++i;
        if(!packet.format[i] || argument>=packet.count) break;
        auto& value=packet.arguments[argument++];
        if(packet.format[i]=='n') {output[0]=0;return;}
        const bool string=packet.format[i]=='s' || packet.format[i]=='S';
        if(string) {
            if(value.kind==LogArgumentKind::Pointer && value.bits) {output[0]=0;return;}
            if((value.kind==LogArgumentKind::Narrow || value.kind==LogArgumentKind::Wide) && !value.owned && value.bits) {output[0]=0;return;}
        } else value.owned=false;
    }
    if(packet.formatter) packet.formatter(packet,output,capacity);
}
}
