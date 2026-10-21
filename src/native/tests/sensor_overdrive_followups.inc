namespace {
bool sensorParty=false,sensorAllowed=true;
unsigned sensorHelpCalls=0;
int __cdecl sensorPartyGetter() { return sensorParty?-1:0; }
int __cdecl sensorAllowedGetter(unsigned id) { return sensorAllowed && id>=20 && id<28?1:0; }
uintptr_t __cdecl sensorHelp(unsigned id,int kind) {
    ++sensorHelpCalls;
    check(id==20 && kind==1,"Sensor reads native monster description rather than full Scan stats");
    return address(0x2310700);
}
uintptr_t __cdecl sensorStatus(int space,unsigned id,unsigned languageId) {
    check(space==1 && languageId==0,"Sensor labels use the native status text space and current language");
    return address(id==0x1009?0x2310720:0x2310730);
}
void sensorFixtures() {
    scene=0; set<uint8_t>(0x1fcc092,0);
    stub(0x38fa30,reinterpret_cast<void*>(sensorPartyGetter));
    stub(0x38f9e0,reinterpret_cast<void*>(sensorAllowedGetter));
    stub(0x38f810,reinterpret_cast<void*>(sensorHelp));
    const unsigned monster=0x2120000+20*0xf90;
    set<uintptr_t>(0xd34460,address(monster));
    encoded(monster+0x540,"Gold Element"); encoded(0x2310700,"Resists magic.\nUses Thunder.");
    set<uint8_t>(monster+0xdc8,1); set<int>(monster+0x594,1000);
    set<int>(monster+0x6e4,250); set<int>(monster+0x5d0,777);
    set<uint8_t>(monster+0x5dd,1|4); set<uint8_t>(monster+0x5dc,8);
    set<uint8_t>(monster+0x5da,2); set<uint8_t>(monster+0x5db,4);
    set<uint8_t>(0xf3c911,4); set<uint8_t>(0xf3c912,1); set<uint8_t>(0xf3c918,0);
    set<uint16_t>(0xf3c9e8,0); set<uint16_t>(0xf3c9ea,16);
    set<uint16_t>(0xf3d1b4,20); set<int>(0xf3d6a0,0); set<uint8_t>(0xf3c8fe,0);
    resetBattle(); event(0x49d300,address(0xf3c910));
    expect(L"Gold Element: 78%","without Sensor the gauge uses committed HP rather than the action calculator's forecast");
    check(!battleSensorVisible(20) && !battleSensorAvailable(),"hidden Sensor information is unavailable");
    sensorParty=true; sensorHelpCalls=0; event(0x49d300,address(0xf3c910));
    expect(L"Gold Element: HP 777/1000","Sensor availability changes on the same target and uses displayed HP numbers");
    check(sensorHelpCalls==1,"Sensor buffer is assembled at enemy highlight before the pane visibility callback");
    set<uint8_t>(0xf3c8fe,1); event(0x4913f0,0);
    check(battleSensorAvailable() && battleSensorCapturesKey(VK_LEFT,true) && !battleSensorCapturesKey(VK_LEFT,false),"Sensor buffer owns only control arrows");
    dispatchBattleSensorKey(VK_LEFT,true); expect(L"Weakness: Fire, Lightning","native weakness fields and icon order");
    dispatchBattleSensorKey(VK_DOWN,true); expect(L"Absorbs: Water","absorption is separate from weakness");
    dispatchBattleSensorKey(VK_DOWN,true); expect(L"Nullifies: Ice","nullification is separate from resistance");
    dispatchBattleSensorKey(VK_DOWN,true); expect(L"Resists: Lightning","native resistance mask");
    dispatchBattleSensorKey(VK_DOWN,true); expect(L"Resists magic.","native description retains its first rendered line");
    dispatchBattleSensorKey(VK_RIGHT,true);
    expect(L"Uses Thunder.","Right selects and reads the last buffer line");
    dispatchBattleSensorKey(VK_UP,true); expect(L"Resists magic.","Up moves from the last line to its predecessor");
    check(sensorHelpCalls==1,"pane confirmation and all reading keys reuse the highlight snapshot");
    set<uint8_t>(monster+0x5dd,8); dispatchBattleSensorKey(VK_LEFT,true);
    expect(L"Weakness: Fire, Lightning","reading keys do not reconstruct the highlight snapshot");
    encoded(0x2310720,"Poison"); encoded(0x2310730,"Haste"); stub(0x38fd40,reinterpret_cast<void*>(sensorStatus));
    set<uint16_t>(monster+0x606,8); set<uint8_t>(monster+0x613,3);
    set<uint8_t>(0xf3c918,1); event(0x49d300,address(0xf3c910));
    check(sensorHelpCalls==2,"new target owner assembles fresh native information");
    dispatchBattleSensorKey(VK_LEFT,true); expect(L"Weakness: Water","new highlight reflects changed native affinities");
    for(unsigned i=0;i<4;++i) dispatchBattleSensorKey(VK_DOWN,true);
    expect(L"Status: Poison, Haste","native visible status labels retain the actual hexadecimal text IDs");
    dispatchBattleSensorKey(VK_RIGHT,true); expect(L"Uses Thunder.","Right selects the end when status rows are present");
    dispatchBattleSensorKey(VK_DOWN,true); expect(L"Uses Thunder.","Down stays on the last line at the buffer boundary");
    check(sensorHelpCalls==2,"review keys never call the native description getter");
    set<uint16_t>(monster+0x606,0); set<uint8_t>(monster+0x613,0); stub(0x38fd40,reinterpret_cast<void*>(menu));
    set<uint8_t>(0xf3c918,2); check(!battleSensorAvailable(),"turn ownership invalidates stale Sensor review");
    set<uint8_t>(0xf3c918,0); set<uint16_t>(0xf3c9ea,32); check(!battleSensorAvailable(),"target mask invalidates stale review before redisplay");
    set<uint16_t>(0xf3c9ea,16); sensorAllowed=false;
    check(!battleSensorVisible(20),"native Sensor immune enemies do not leak numbers");
    event(0x4913f0,0); check(!battleSensorAvailable(),"native visibility gate expires the buffer");
    sensorParty=false; set<uint16_t>(monster+0x616,1);
    check(battleSensorVisible(20),"native Scan flag reveals the same pane independently of equipment Sensor");
    set<uint16_t>(monster+0x616,0); sensorAllowed=true;
    set<uint8_t>(0xf3c8fe,0); set<uint16_t>(0xf3d1b4,255); set<uint8_t>(0xf3c912,0);
    set<uint16_t>(0xf3c9e8,1); set<uint16_t>(0xf3c9ea,0); resetBattle();
}
void overdriveFixtures() {
    auronSequenceFixtures();
    setSetting(Setting::OverdriveAssist,0); setSetting(Setting::OverdriveCues,1);
    const auto base=address(0xf3d6f4);
    write<uint16_t>(base,4); write<int16_t>(base+2,123);
    write<int16_t>(base+8,95); write<int16_t>(base+10,105); write<int16_t>(base+14,94);
    const auto tones=overdriveBeeps;
    event(0x492290,0); check(overdriveBeeps==tones,"Tidus remains silent outside native success zone");
    write<int16_t>(base+14,95); event(0x492290,0); check(overdriveBeeps==tones+1,"Tidus beeps at the first accepted Confirm position");
    write<int16_t>(base+14,105); event(0x492290,0); check(overdriveBeeps==tones+1,"a single native success traversal has one cue");
    write<int16_t>(base+14,106); event(0x492290,0); write<int16_t>(base+14,104); event(0x492290,0);
    check(overdriveBeeps==tones+2,"a new pass through the success zone cues again");
    setSetting(Setting::OverdriveAssist,1); event(0x492290,0);
    check(read<uint16_t>(base)==5 && read<int16_t>(base+2)==123,"Tidus auto success preserves the adjacent native coordinate");
    for(unsigned command: {0x22u,0x32u,0x42u,0x52u}) {
        write<uint16_t>(address(0xf3d6b4),4); write<uint8_t>(address(0xf3d6b2),0);
        Event e; e.rva=0x490ee0; e.args[0]=command;
        listeners[0x490ee0][0].enter(e);
        check(read<uint16_t>(address(0xf3d6b4))==5 && read<uint8_t>(address(0xf3d6b2))==1,"all four Auron commands use the native combo-success phase");
    }
    setSetting(Setting::OverdriveAssist,0); write<uint16_t>(address(0xf3d6c8),4); write<uint8_t>(address(0xf3d6df),2);
    event(0x491af0,0,[]{write<uint8_t>(address(0xf3d6df),3);}); expect(L"3 casts","manual Lulu announces the native meter increase");
    const auto spoken=said.size(); event(0x491af0,0); check(said.size()==spoken,"unchanged Lulu meter is silent");
    setSetting(Setting::OverdriveAssist,1); event(0x491af0,0);
    check(read<uint16_t>(address(0xf3d6c8))==6 && read<uint8_t>(address(0xf3d6df))==16,"Lulu auto-success retains the native maximum cast result");
    resetOverdrive();
}
}
