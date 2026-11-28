#include "../settings.hpp"
#include <fstream>
#include <cstdio>
#include <stdexcept>
namespace ffx {
bool testForeground=true;
void recordLog(const char*,...) {}
}
namespace {
void check(bool pass,const char* text) {if(!pass)throw std::runtime_error(text);}
std::string contents(const std::wstring& path) {
    std::ifstream in(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});
}
void replaceFixture(const std::wstring& path,const std::string& ini) {
    {std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<ini;check(out.good(),"write isolated persistence fixture");}
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
}
void checkNoStaging(const std::wstring& path) {
    const auto pattern=path.substr(0,path.find_last_of(L"\\/")+1)+L"ffx*.tmp";
    WIN32_FIND_DATAW data{};
    const auto found=FindFirstFileW(pattern.c_str(),&data);
    if(found!=INVALID_HANDLE_VALUE) FindClose(found);
    check(found==INVALID_HANDLE_VALUE,"settings write leaves no owned temporary file");
}
bool runSettingsPeer(const wchar_t* argument) {
    wchar_t exe[MAX_PATH]{};
    const auto length=GetModuleFileNameW(nullptr,exe,MAX_PATH);
    check(length && length<MAX_PATH,"settings fixture executable path");
    auto command=std::wstring(L"\"")+exe+L"\" "+argument;
    STARTUPINFOW startup{};
    startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    check(CreateProcessW(exe,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&startup,&process)!=0,"start isolated settings peer");
    CloseHandle(process.hThread);
    const auto waited=WaitForSingleObject(process.hProcess,10000);
    DWORD exitCode=1;
    const bool succeeded=waited==WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess,&exitCode) && exitCode==0;
    CloseHandle(process.hProcess);
    return succeeded;
}
void persistenceChecks(const std::wstring& path) {
    using namespace ffx;
    setSetting(Setting::AutoDetail,0);
    drainSettingsForTest();
    check(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES,"an unchanged default does not create a file");
    setSetting(Setting::AutoDetail,1);
    drainSettingsForTest();
    check(GetPrivateProfileIntW(L"Accessibility",L"auto_detail",99,path.c_str())==1 &&
          GetPrivateProfileIntW(L"Accessibility",L"speech",99,path.c_str())==1,"first save keeps the existing INI keys and defaults");
    checkNoStaging(path);

    const std::string prior="; Keep local controls\r\n[Accessibility]\r\nspeech=1\r\nauto_detail=0\r\nunknown_option=42\r\n[Controller]\r\nlayout=2\r\ncommand_1=9\r\n";
    replaceFixture(path,prior);
    loadSettings();
    setSetting(Setting::AutoDetail,0);
    drainSettingsForTest();
    check(contents(path)==prior,"unchanged settings leave the existing INI byte-for-byte intact");
    const auto held=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    check(held!=INVALID_HANDLE_VALUE,"hold original INI open without rename sharing");
    setSetting(Setting::AutoDetail,1);
    drainSettingsForTest();
    const auto intact=contents(path)==prior;
    CloseHandle(held);
    check(intact,"a refused replacement preserves the complete old INI");
    checkNoStaging(path);
    drainSettingsForTest();
    check(GetPrivateProfileIntW(L"Accessibility",L"auto_detail",99,path.c_str())==1,"failed settings remain pending for retry");
    check(contents(path).find("[Accessibility]")!=std::string::npos,"existing ANSI INI retains its encoding");
    check(GetPrivateProfileIntW(L"Controller",L"command_1",99,path.c_str())==9 &&
          GetPrivateProfileIntW(L"Accessibility",L"unknown_option",99,path.c_str())==42,"saving preserves controller bindings and unknown keys");

    // This process retains its original Speech value while a second process
    // publishes a new one. Saving a different option must carry that edit over.
    const auto mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,L"Global\\FFXScreenReader.Settings");
    check(mutex && WaitForSingleObject(mutex,0)==WAIT_OBJECT_0,"hold the shared writer lock");
    const bool respectedLock=runSettingsPeer(L"--settings-peer-blocked");
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    check(respectedLock,"peer leaves the INI unchanged while another process owns the writer lock");
    check(runSettingsPeer(L"--settings-peer"),"settings peer persisted its own option");
    check(setting(Setting::Speech)==1,"peer setting change did not replace this process's in-memory snapshot");
    setSetting(Setting::AutoDetail,0);
    drainSettingsForTest();
    check(GetPrivateProfileIntW(L"Accessibility",L"speech",99,path.c_str())==0 &&
          GetPrivateProfileIntW(L"Accessibility",L"auto_detail",99,path.c_str())==0,"independent processes preserve each other's saved values");

    const std::wstring unicode=L"[Accessibility]\r\nauto_detail=0\r\n[Custom]\r\nlabel=R\u00e9tro\r\n";
    std::string wideBytes("\xff\xfe",2);
    wideBytes.append(reinterpret_cast<const char*>(unicode.data()),unicode.size()*sizeof(wchar_t));
    replaceFixture(path,wideBytes);
    setSetting(Setting::AutoDetail,1);
    drainSettingsForTest();
    wchar_t label[20]{};
    GetPrivateProfileStringW(L"Custom",L"label",L"",label,_countof(label),path.c_str());
    check(contents(path).compare(0,2,wideBytes,0,2)==0 && std::wstring(label)==L"R\u00e9tro","existing Unicode INI keeps its encoding and custom text");
    checkNoStaging(path);
    check(DeleteFileW(path.c_str())!=0,"remove persistence fixture");
    loadSettings();
}
}
int main(int argc,char** argv) {
    using namespace ffx;const auto path=settingsPath();
    const std::string mode=argc==2?argv[1]:"";
    if(mode=="--settings-peer" || mode=="--settings-peer-blocked") {
        loadSettings();
        setSetting(Setting::Speech,0);
        drainSettingsForTest();
        const auto expected=mode=="--settings-peer"?0u:1u;
        return GetPrivateProfileIntW(L"Accessibility",L"speech",99,path.c_str())==expected?0:1;
    }
    // The executable is built into its own fixture directory, never a game.
    if(path.empty() || GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES)return 2;
    try {
        constexpr Setting assists[]={Setting::OverdriveAssist,Setting::LightningAutoDodge,Setting::JechtAssist,Setting::GagazetAssist,Setting::SinCollectionAssist,
            Setting::ChocoboAssist,Setting::RemiemAssist,Setting::ButterflyAssist,Setting::BlitzAssist};
        constexpr Setting cues[]={Setting::GagazetCues};
        loadSettings();
        check(!setting(Setting::AutoExtraInfo) && settingInfo(Setting::AutoExtraInfo).initial==0,"fresh extra information defaults Off");
        check(!setting(Setting::UnavailableCue) && settingInfo(Setting::UnavailableCue).initial==0,"fresh rejection cue defaults Off");
        check(setting(Setting::SphereNodeNumbers)==1 && settingInfo(Setting::SphereNodeNumbers).initial==1 &&
            static_cast<unsigned>(Setting::SphereNodeNumbers)>static_cast<unsigned>(Setting::AutoExtraInfo),"node numbers default On with a new appended ID");
        check(static_cast<unsigned>(Setting::AutoDetail)==2 && static_cast<unsigned>(Setting::BlitzAssist)==34 &&
            static_cast<unsigned>(Setting::AutoExtraInfo)==35 && static_cast<unsigned>(Setting::UnavailableCue)==36,
            "setting IDs match the enum after the maximum HP and MP settings were removed");
        for(const auto id:assists)check(settingInfo(id).initial==0 && setting(id)==0,"fresh auto-success must default Off");
        for(const auto id:cues)check(settingInfo(id).initial==1 && setting(id)==1,"fresh puzzle cues must default On");
        check(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES,"loading fresh defaults must not create a settings file");
        persistenceChecks(path);
        {
            const std::string ini="; Missing assistance keys inherit Off\r\n[Accessibility]\r\ngagazet_cues=1\r\n";
            {std::ofstream out(path,std::ios::binary);out<<ini;check(out.good(),"write missing-key fixture settings");}
            WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
            loadSettings();
            for(const auto id:assists)check(setting(id)==0,"missing assistance key must default Off");
            check(!setting(Setting::AutoExtraInfo),"legacy INI with no extra-info key inherits Off without rewriting");
            check(!setting(Setting::UnavailableCue),"legacy INI with no rejection cue key inherits Off without rewriting");
            check(contents(path)==ini,"missing-key loading must preserve exact file bytes");
        }
        for(unsigned mask=0;mask<1024;++mask) {
            std::string ini="; Preserve this comment and unrelated controls\r\n[Controller]\r\nrepeat=7\r\n[Accessibility]\r\n";
            for(unsigned i=0;i<_countof(assists);++i) {
                const auto key=settingInfo(assists[i]).key;
                for(const auto* c=key;*c;++c)ini.push_back(static_cast<char>(*c));
                ini+="="+std::to_string((mask>>i)&1)+"\r\n";
            }
            const auto cueKey=settingInfo(cues[0]).key;
            for(const auto* c=cueKey;*c;++c)ini.push_back(static_cast<char>(*c));
            ini+="="+std::to_string((mask>>_countof(assists))&1)+"\r\n";
            {std::ofstream out(path,std::ios::binary);out<<ini;check(out.good(),"write isolated fixture settings");}
            WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
            loadSettings();
            for(unsigned i=0;i<_countof(assists);++i)check(setting(assists[i])==static_cast<int>((mask>>i)&1),"saved On/Off must override changed defaults");
            check(setting(cues[0])==static_cast<int>((mask>>_countof(assists))&1),"saved cue On/Off must override changed defaults");
            check(contents(path)==ini,"loading settings must preserve exact file bytes");
        }
        for(unsigned mask=0;mask<4;++mask) {
            const std::string ini="; independent descriptions\r\n[Accessibility]\r\nauto_detail="+std::to_string(mask&1)+
                "\r\nauto_extra_info="+std::to_string((mask>>1)&1)+"\r\n";
            {std::ofstream out(path,std::ios::binary);out<<ini;check(out.good(),"write independent extra-info setting fixture");}
            WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());loadSettings();
            check(setting(Setting::AutoDetail)==static_cast<int>(mask&1) && setting(Setting::AutoExtraInfo)==static_cast<int>((mask>>1)&1),
                "all four saved Autodetail/extra-info combinations load independently");
            check(contents(path)==ini,"loading independent extra-info settings preserves exact bytes");
        }
        for(const auto id:{Setting::ChocoboAssist,Setting::RemiemAssist,Setting::ButterflyAssist,Setting::BlitzAssist}) {
            const auto before=settingRevision(id);setSetting(id,setting(id));
            check(settingRevision(id)==before,"unchanged values do not invalidate native actions");
            setSetting(id,1-setting(id));setSetting(id,1-setting(id));
            check(settingRevision(id)==before+2,"Off/On round trips have distinct cancellation revisions");
        }
        check(DeleteFileW(path.c_str())!=0,"remove only the isolated fixture INI");
        puts("PASS: fresh and missing-key defaults; all1024 saved On/Off combinations; unchanged saves, first save, refused replacement and retry, controller keys, independent processes and Unicode INI.");return 0;
    } catch(const std::exception& e) {
        DeleteFileW(path.c_str());fprintf(stderr,"FAIL: %s\n",e.what());return 1;
    }
}
