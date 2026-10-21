#pragma once
#include <cstdint>
#include <cmath>
#include <array>
namespace ffx::pad {
enum Control { South=0,East=1,West=2,North=3,View=4,Guide=5,Menu=6,L3=7,R3=8,
    LB=9,RB=10,Up=11,Down=12,Left=13,Right=14,LT=26,RT=27,
    RSUp=28,RSDown=29,RSLeft=30,RSRight=31,LSUp=32,LSDown=33,LSLeft=34,LSRight=35,Count=36 };
constexpr uint64_t bit(int control) { return uint64_t{1}<<control; }
struct State {
    uint64_t held=0;
    int16_t lx=0,ly=0,rx=0,ry=0;
    uint8_t lt=0,rt=0;
    bool analogActive() const { return std::abs(int(lx))>=5000 || std::abs(int(ly))>=5000 || std::abs(int(rx))>=5000 || std::abs(int(ry))>=5000 || lt>=30 || rt>=30; }
    bool neutral() const { return !held && !analogActive(); }
};
struct Gesture {
    int current=-1;
    int update(int x,int y) {
        if(std::abs(x)<11000 && std::abs(y)<11000) current=-1;
        else if(current<0) {
            if(std::abs(y)>21000 && std::abs(x)<11000) current=y>0?0:1;
            else if(std::abs(x)>21000 && std::abs(y)<11000) current=x<0?2:3;
        }
        return current;
    }
};
struct Result { uint64_t edges=0; bool exclusive=false,changed=false,input=false; };
struct Router {
    bool mod=false,draining=true,focused=false,menu=false;
    uint64_t previous=0;
    bool previousAnalog=false;
    Result update(const State& state,bool focus,bool settings) {
        Result out;
        if(!focus) mod=false;
        if(!focus || focus!=focused || settings!=menu) draining=true;
        focused=focus; menu=settings;
        out.edges=state.held&~previous; previous=state.held;
        const bool analog=state.analogActive();
        out.input=out.edges!=0 || (analog && !previousAnalog); previousAnalog=analog;
        if(!focus || draining) {
            out.edges=0; out.input=false; out.exclusive=true;
            if(focus && state.neutral()) draining=false;
            return out;
        }
        if(!settings && (out.edges&bit(View))) {
            mod=!mod; out.changed=true; out.edges=0; draining=true;
        }
        out.exclusive=settings || mod || draining;
        return out;
    }
    void complete() { mod=false; draining=true; }
};
enum Command { Scan,PreviousTarget,NextTarget,PreviousCategory,NextCategory,Distance,Route,
    Beacon,Turn,Party,Description,PreviousDetail,NextDetail,SphereLevels,PuzzleStep,PuzzlePath,Repeat,GameSelect,Gil,Map,Filter,CountCommands };
inline constexpr int defaults[]={-1,Left,Right,Up,Down,-1,RT,L3,-1,-1,RSUp,-1,-1,-1,South,East,-1,-1,West,North,R3};
inline constexpr const wchar_t* names[]={L"Scan navigation targets",L"Previous target",L"Next target",L"Previous category",L"Next category",
    L"Target distance",L"Read route",L"Toggle beacon",L"Whose turn",L"Active character HP, MP and status",L"Native description",
    L"Previous status detail",L"Next status detail",L"Sphere levels",L"Previous puzzle step",L"Next puzzle step",L"Repeat speech",L"Send game Select or Back",L"Gil / active battle character",L"Current map name",L"Toggle unreachable filter"};
inline constexpr int bindingCommands[]={PreviousTarget,NextTarget,PreviousCategory,NextCategory,Route,Beacon,Filter,Gil,Map,PuzzleStep,PuzzlePath,Description};
inline constexpr unsigned BindingCount=sizeof(bindingCommands)/sizeof(bindingCommands[0]);
inline bool context(int command,bool mod,bool battle,bool menu) {
    if(mod) return command==Gil || (!battle && (command==Map || command==PuzzleStep || command==PuzzlePath));
    if(command==Description) return menu;
    return !battle && !menu && (command==PreviousTarget || command==NextTarget || command==PreviousCategory || command==NextCategory || command==Route || command==Beacon || command==Filter);
}
inline bool allowed(int control) { return control>=0 && control<Count && control!=View && control!=Menu && control!=Guide; }
inline bool assign(std::array<int,CountCommands>& bindings,int command,int control) {
    if(command<0 || command>=CountCommands || !allowed(control)) return false;
    const int old=bindings[command];
    for(int i=0;i<CountCommands;++i) if(i!=command && bindings[i]==control) bindings[i]=old;
    bindings[command]=control; return true;
}
}
