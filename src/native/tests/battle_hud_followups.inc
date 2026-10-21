#include "battle_hud_bytes.inc"
namespace {
void battleHudFixtures() {
    check(listeners.count(0x4a0520) && listeners.count(0x4f4b20),"CTB and plain-quad owners are installed, so nearest-hook parent qualification matches production");
    auto nativeClip=[&](const unsigned char* body,size_t bytes,const HudFixup* fix,size_t count) {
        auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
        check(code!=nullptr,"native HUD clip replay allocation");std::memcpy(code,body,bytes);
        for(size_t i=0;i<count;++i){const auto value=fix[i].rva==0x7368c0?reinterpret_cast<uintptr_t>(&hudClipOutside):address(fix[i].rva);std::memcpy(code+fix[i].offset,&value,4);}
        FlushInstructionCache(GetCurrentProcess(),code,bytes);return code;
    };
    auto* texture=nativeClip(hudTextureClipBody,sizeof(hudTextureClipBody),hudTextureClipFixups,_countof(hudTextureClipFixups));
    auto* plain=nativeClip(hudPlainClipBody,sizeof(hudPlainClipBody),hudPlainClipFixups,_countof(hudPlainClipFixups));
    auto enter=[&](unsigned rva,Event& e){e.rva=rva;e.epoch=generation();for(const auto& l:listeners[rva])if(l.enter)l.enter(e);};
    auto leave=[&](unsigned rva,Event& e){for(const auto& l:listeners[rva])if(l.leave)l.leave(e);};
    auto clip=[&](float x,float y,float w,float h,bool textured,unsigned parent=0,uintptr_t caller=0) {
        float x1=x+w,y1=y+h,u=0,v=0,u1=1,v1=1;
        using Textured=int(__cdecl*)(float*,float*,float*,float*,float*,float*,float*,float*);
        using Plain=int(__cdecl*)(float*,float*,float*,float*);
        Event e;e.parentRva=textured?0x503bb0:0x4f4b20;e.args[0]=reinterpret_cast<uintptr_t>(&x);e.args[1]=reinterpret_cast<uintptr_t>(&y);
        if(parent)e.parentRva=parent;e.caller=caller;
        e.args[2]=reinterpret_cast<uintptr_t>(&x1);e.args[3]=reinterpret_cast<uintptr_t>(&y1);
        e.result=textured?reinterpret_cast<Textured>(texture)(&x,&y,&x1,&y1,&u,&v,&u1,&v1):reinterpret_cast<Plain>(plain)(&x,&y,&x1,&y1);
        leave(textured?0x4e5a20:0x4e58e0,e);return e.result!=0 && x1>x && y1>y;
    };
    auto bounds=[&](float x0,float y0,float x1,float y1){
        write<int>(address(0x1471690),1);write<uint8_t>(address(0x1471694),0);
        write<float>(address(0x14716b8),x0);write<float>(address(0x14716bc),y0);
        write<float>(address(0x14716c0),x1);write<float>(address(0x14716c4),y1);
    };
    auto bits=[](float f){uintptr_t value=0;std::memcpy(&value,&f,4);return value;};
    Event frame;
    auto row=[&](unsigned sprite,float y,bool letterVisible=true) {
        Event card;card.parentRva=0x4a0520;card.args[3]=bits(y);enter(0x4a1660,card);
        Event icon;icon.parentRva=0x4a1660;icon.args[0]=sprite;enter(0x49e5d0,icon);
        Event quad;quad.parentRva=0x49e5d0;quad.args[0]=400;
        enter(0x503bb0,quad);clip(440,y,25,18,true);leave(0x503bb0,quad);
        if(sprite>=0x1c3 && sprite<=0x1d2) {enter(0x503bb0,quad);clip(445,letterVisible?y+6:999,10,8,true);leave(0x503bb0,quad);}
        leave(0x49e5d0,icon);leave(0x4a1660,card);
    };
    auto order=[&]{chords['T'|0x10000].first(true);drainSpeechDomainsForTest();};
    auto all=[&]{chords['O'|0x10000].first(true);drainSpeechDomainsForTest();};
    bounds(430,75,472,186);enter(0x49b2d0,frame);
    // Thirty-two native forecast draw attempts, only five identifying quads
    // survive clipping. Duplicate Tidus turns and actual vertical order remain.
    for(unsigned i=0;i<27;++i)row(0x1b8,-500-static_cast<float>(i)*20);
    row(0x1b8,150);row(0x1cb,100);row(0x1b8,80);row(0x1d4,125);row(0x1c0,175);
    leave(0x49b2d0,frame);order();expect(L"Turn order: Tidus, Enemy A, Unknown, Tidus, Aeon","only five rendered CTB identities, sorted vertically, with repeated turns retained");
    enter(0x49b2d0,frame);row(0x1cc,100,false);leave(0x49b2d0,frame);order();
    expect(L"No turn order displayed","enemy backdrop cannot reveal a clipped identifying letter");
    // Partly visible identifying cards count; exact edge contact does not.
    enter(0x49b2d0,frame);row(0x1b8,58);row(0x1b8,57);row(0x1b8,186);leave(0x49b2d0,frame);order();
    expect(L"Turn order: Tidus","original textured clip preserves only positive visible area");
    enter(0x49b2d0,frame);leave(0x49b2d0,frame);order();expect(L"No turn order displayed","hidden HUD frame retires earlier CTB list");
    bounds(0,0,512,416);write<uint16_t>(address(0xf3f76c),0);write<uint16_t>(address(0xf3f76c)+2,255);write<uint16_t>(address(0xf3f76c)+4,8);
    write<int16_t>(address(0x21d0aa4),100);write<uint16_t>(address(0x21d0aa4)+2,0xabcd); // Native maximum is a signed word.
    enter(0x49b2d0,frame);Event status;status.parentRva=0x49b2d0;enter(0x4963c0,status);
    Event gauge;gauge.parentRva=0x4963c0;gauge.args[0]=33;enter(0x496050,gauge);clip(400,325,80,10,false);leave(0x496050,gauge);
    gauge.args[0]=150;enter(0x496050,gauge);clip(400,390,80,10,false);leave(0x496050,gauge);
    leave(0x4963c0,status);leave(0x49b2d0,frame);all();
    expect(L"Tidus Overdrive 33%, Valefor Overdrive 100%","Shift+O uses only drawn valid native rows and displayed interpolated fill");
    set<uint8_t>(0x2120000+0x5bc,0);set<uint8_t>(0x2120000+0x5bd,150);chords['O'].first(false);drainSpeechDomainsForTest();
    expect(L"Tidus Overdrive 33%","active O follows displayed fill when its meter is present");
    enter(0x49b2d0,frame);enter(0x4963c0,status);gauge.args[0]=50;enter(0x496050,gauge);
    check(!clip(400,500,80,10,false),"native plain clip offscreen sentinel has zero area despite true return");
    leave(0x496050,gauge);leave(0x4963c0,status);leave(0x49b2d0,frame);all();
    expect(L"No Overdrive meters displayed","offscreen meter and stale earlier rows are excluded");
    auto vitals=[&]{chords['H'|0x10000].first(true);drainSpeechDomainsForTest();};
    auto name=[&](unsigned id){Event e;e.parentRva=0x4963c0;e.args[0]=id;enter(0x39af20,e);leave(0x39af20,e);};
    auto digits=[&](int value,float y=330,bool foreground=true){
        Event e;e.parentRva=0x4963c0;e.args[0]=static_cast<uintptr_t>(value);enter(0x505890,e);
        clip(430,y,20,8,true,0x505890,address(0x4fb940)+(foreground?0x154:0x150));leave(0x505890,e);
    };
    auto panelFrame=[&](unsigned mode){write<uint8_t>(address(0xf3d742),static_cast<uint8_t>(mode));enter(0x49b2d0,frame);enter(0x4963c0,status);};
    auto panelEnd=[&]{leave(0x4963c0,status);leave(0x49b2d0,frame);vitals();};
    panelFrame(3);name(0);digits(275);digits(17);name(8);digits(400,390);digits(45,390);panelEnd();
    expect(L"Tidus HP 275 MP 17, Valefor HP 400 MP 45","Shift+H reads exact rendered rows/animated values, including aeon, excluding reserve/empty slots");
    panelFrame(0);name(0);digits(0);digits(500);panelEnd();expect(L"Tidus HP 0/500","HP-only HUD keeps displayed maximum and zero without inventing MP");
    panelFrame(1);name(0);digits(12);digits(30);panelEnd();expect(L"Tidus MP 12/30","MP-only HUD keeps displayed current and maximum without inventing HP");
    panelFrame(3);name(0);digits(275);digits(17,500);panelEnd();expect(L"Tidus HP 275","unrendered MP digits stay excluded even when its value is available");
    panelFrame(0);name(0);digits(275,500);digits(500);panelEnd();expect(L"Tidus Maximum HP 500","a clipped current value is not reconstructed from hidden actor data");
    panelFrame(3);name(0);digits(275,330,false);digits(17,330,false);panelEnd();expect(L"No HP or MP panels displayed","nonforeground/shadow clip cannot qualify number preparation");
    panelFrame(2);name(0);digits(275);digits(17);panelEnd();expect(L"No HP or MP panels displayed","status-only HUD cannot reveal hidden HP/MP");
    enter(0x49b2d0,frame);leave(0x49b2d0,frame);vitals();expect(L"No HP or MP panels displayed","hidden frame retires all prior character panels");
    resetBattle();order();expect(L"No turn order displayed","battle reset clears HUD snapshot");
    write<int>(address(0x1471690),0);VirtualFree(texture,0,MEM_RELEASE);VirtualFree(plain,0,MEM_RELEASE);
}
}
