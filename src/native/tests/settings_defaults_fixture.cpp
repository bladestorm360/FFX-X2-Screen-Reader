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
}
int main() {
    using namespace ffx;const auto path=settingsPath();
    // The executable is built into its own fixture directory, never a game.
    if(path.empty() || GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES)return 2;
    try {
        constexpr Setting assists[]={Setting::OverdriveAssist,Setting::LightningAutoDodge,Setting::JechtAssist,Setting::GagazetAssist,Setting::SinCollectionAssist,
            Setting::ChocoboAssist,Setting::RemiemAssist,Setting::ButterflyAssist,Setting::BlitzAssist};
        constexpr Setting cues[]={Setting::GagazetCues};
        loadSettings();
        for(const auto id:assists)check(settingInfo(id).initial==0 && setting(id)==0,"fresh auto-success must default Off");
        for(const auto id:cues)check(settingInfo(id).initial==1 && setting(id)==1,"fresh puzzle cues must default On");
        check(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES,"loading fresh defaults must not create a settings file");
        {
            const std::string ini="; Missing assistance keys inherit Off\r\n[Accessibility]\r\ngagazet_cues=1\r\n";
            {std::ofstream out(path,std::ios::binary);out<<ini;check(out.good(),"write missing-key fixture settings");}
            WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
            loadSettings();
            for(const auto id:assists)check(setting(id)==0,"missing assistance key must default Off");
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
        for(const auto id:{Setting::ChocoboAssist,Setting::RemiemAssist,Setting::ButterflyAssist,Setting::BlitzAssist}) {
            const auto before=settingRevision(id);setSetting(id,setting(id));
            check(settingRevision(id)==before,"unchanged values do not invalidate native actions");
            setSetting(id,1-setting(id));setSetting(id,1-setting(id));
            check(settingRevision(id)==before+2,"Off/On round trips have distinct cancellation revisions");
        }
        check(DeleteFileW(path.c_str())!=0,"remove only the isolated fixture INI");
        puts("PASS: fresh and missing-key Overdrive/Jecht/Gagazet/Lightning/Sin/Chocobo/Remiem/Butterfly/Blitz assistance Off and Gagazet cues On; all1024 saved On/Off combinations override defaults; load preserves exact INI bytes and unrelated controls.");return 0;
    } catch(const std::exception& e) {
        DeleteFileW(path.c_str());fprintf(stderr,"FAIL: %s\n",e.what());return 1;
    }
}
