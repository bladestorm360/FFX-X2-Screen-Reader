#include "controller.hpp"
#include "settings.hpp"
#include "nonblocking_snapshots.hpp"
#include <algorithm>
#include <stdexcept>
namespace ffx {
namespace {
std::array<int,pad::CountCommands> bindings=[] { std::array<int,pad::CountCommands> result{}; std::copy(std::begin(pad::defaults),std::end(pad::defaults),result.begin()); return result; }();
struct BindingSnapshot {std::array<int,pad::CountCommands> values{};unsigned revision=0;};
async::AtomicSnapshot<BindingSnapshot> writes;
unsigned publishedRevision=0; // Bootstrap, then sole native owner.
unsigned persistedRevision=0; // Settings worker only.
#ifdef FFX_READER_TEST
std::atomic<bool(*)(const std::wstring&,const int*,size_t)> testWrite{nullptr};
#endif
int learning=-1;
bool loaded=false; // Bootstrap only; runtime never performs lazy disk access.
void loadBindings(const std::wstring& path) {
    if(loaded) return;
    loaded=true;
    if(GetPrivateProfileIntW(L"Controller",L"layout",0,path.c_str())!=2) return;
    std::array<int,pad::CountCommands> candidate=bindings;
    uint64_t used=0;
    for(int i=0;i<pad::CountCommands;++i) {
        const auto key=L"command_"+std::to_wstring(i);
        const int control=GetPrivateProfileIntW(L"Controller",key.c_str(),pad::defaults[i],path.c_str());
        if(control==-1) { candidate[i]=-1; continue; }
        if(!pad::allowed(control) || (used&pad::bit(control))) return;
        used|=pad::bit(control); candidate[i]=control;
    }
    bindings=candidate;
}
bool persistOwned(const std::wstring& path) {
    BindingSnapshot snapshot{};
    if(!writes.tryRead(snapshot)) return false;
    if(!snapshot.revision || snapshot.revision==persistedRevision) return true;
#ifdef FFX_READER_TEST
    if(const auto sink=testWrite.load()) {
        if(!sink(path,snapshot.values.data(),snapshot.values.size())) return false;
        persistedRevision=snapshot.revision;return true;
    }
#endif
    bool success=WritePrivateProfileStringW(L"Controller",L"layout",L"2",path.c_str())!=0;
    for(int i=0;i<pad::CountCommands;++i) {
        const auto key=L"command_"+std::to_wstring(i),value=std::to_wstring(snapshot.values[i]);
        if(!WritePrivateProfileStringW(L"Controller",key.c_str(),value.c_str(),path.c_str())) success=false;
    }
    if(success) persistedRevision=snapshot.revision;
    else recordLog("controller binding write failed error=%lu",GetLastError());
    return success;
}
void persist() {
    BindingSnapshot snapshot{bindings,++publishedRevision};writes.publish(snapshot);wakeSettingsWriter();
}
}
void initializeControllerBindings() {
    loadBindings(settingsPath());
    BindingSnapshot initial{bindings,publishedRevision};writes.publish(initial);
    if(!registerSettingsWriterExtension(0,persistOwned)) throw std::runtime_error("Controller settings writer unavailable");
}
int controllerBinding(int command) {
    return command>=0 && command<pad::CountCommands?bindings[command]:-1;
}
#ifdef FFX_READER_TEST
void setControllerWriteSinkForTest(bool(*sink)(const std::wstring&,const int*,size_t)) {testWrite=sink;}
#endif
const wchar_t* controllerControlNameLiteral(int control) {
    static const wchar_t* names[]={L"South face",L"East face",L"West face",L"North face",L"View or Select",L"Guide",L"Menu or Start",L"Left stick click",L"Right stick click",L"Left shoulder",L"Right shoulder",L"D-pad up",L"D-pad down",L"D-pad left",L"D-pad right",L"Miscellaneous one",L"Right paddle one",L"Left paddle one",L"Right paddle two",L"Left paddle two",L"Touchpad",L"Miscellaneous two",L"Miscellaneous three",L"Miscellaneous four",L"Miscellaneous five",L"Miscellaneous six",L"Left trigger",L"Right trigger",L"Right stick up",L"Right stick down",L"Right stick left",L"Right stick right",L"Left stick up",L"Left stick down",L"Left stick left",L"Left stick right"};
    return control>=0 && control<pad::Count?names[control]:L"Unassigned";
}
std::wstring controllerControlName(int control) {return controllerControlNameLiteral(control);}
void learnControllerBinding(int command) {
    if(command<0 || command>=pad::CountCommands) return;
    learning=command;
    drainController();
    output(L"Press a controller button or move a stick for "+std::wstring(pad::names[command])+L". View or Menu cancels. Existing assignments swap.",true,"mod-binding");
}
bool learningControllerBinding() { return learning>=0; }
void cancelControllerBinding() {
    learning=-1;
}
bool captureControllerBinding(uint64_t edges) {
    if(learning<0) return false;
    if(edges&(pad::bit(pad::View)|pad::bit(pad::Menu))) {
        learning=-1; output(L"Controller binding cancelled",true,"mod-binding"); return true;
    }
    for(int control=0;control<pad::Count;++control) if((edges&pad::bit(control)) && pad::allowed(control)) {
        pad::assign(bindings,learning,control); persist();
        output(std::wstring(pad::names[learning])+L": "+controllerControlName(control),true,"mod-binding");
        learning=-1; drainController(); break;
    }
    return true;
}
void resetControllerBindings() {
    loaded=true; learning=-1;
    std::copy(std::begin(pad::defaults),std::end(pad::defaults),bindings.begin()); persist();
    output(L"Controller bindings restored",true,"mod-binding");
}
}
