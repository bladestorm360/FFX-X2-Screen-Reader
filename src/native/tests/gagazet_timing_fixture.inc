#include "gagazet_timing_data.inc"
namespace {
// The original 30-stage schedule gives each ring's native rotation phase.
// Its open intervals synthesize the two raw native hit flags; no timer or
// animation-duration model is substituted for the game's targeting predicate.
unsigned gagScheduledFlags(unsigned tick,unsigned entry) {
    const auto open=[&](unsigned worker) {
        unsigned cycle=0;
        for(const auto& stage:gagazetRingStages) if(stage.worker==worker && stage.entry==entry) cycle+=stage.duration;
        if(!cycle)return false;
        unsigned phase=tick%cycle,ordinal=0;
        for(const auto& stage:gagazetRingStages) if(stage.worker==worker && stage.entry==entry) {
            if(phase<stage.duration)return ordinal==1 || ordinal==3;
            phase-=stage.duration;++ordinal;
        }
        return false;
    };
    const bool innerOpen=open(5),outerOpen=open(7);
    return (innerOpen?0x40u:0u)|(outerOpen?0x80u:0u);
}
void gagScheduledThrow(unsigned tick,unsigned entry,unsigned physicalResult=0) {
    setValue(0x010000d8,3,static_cast<int>(gagScheduledFlags(tick,entry)),2);
    gagQuery(0x24e2,physicalResult);
}
void gagTimingFixtures() {
    check(std::size(gagazetRingStages)==30,"fixture uses all 30 original mtgz0700 ring schedule stages");
    for(unsigned entry:{6u,8u,9u}) {
        gagLoad(); // W22 stays at entry6; only the ring schedule uses speed entries6/8/9.
        // No earlier throw or calibration is performed.
        const auto start=entry==8?12u:entry==9?96u:48u;
        const auto initial=gagScheduledFlags(start,entry);
        check(initial==0xc0,"original schedules synthesize an initial complete targeting window");
        const auto before=gagBeeps;
        gagScheduledThrow(start,entry);
        check(gagBeeps==before+1 && gagLastFrequency==1100.f,"initial simultaneous native flags emit the targeting beep without calibration");
        gagScheduledThrow(start,entry);
        check(gagBeeps==before+1,"repeated aligned input does not repeat the targeting beep");
        const auto cycle=entry==8?72u:entry==9?1152u:288u;
        unsigned crossings=1;bool sawPartial=false,sawOutside=false;
        for(unsigned tick=start+1;tick<start+cycle*8;++tick) {
            const auto flags=gagScheduledFlags(tick,entry);
            if(flags!=0xc0) {
                sawPartial|=flags==0x40 || flags==0x80;
                sawOutside|=flags==0;
            }
            const auto count=gagBeeps;
            const auto prior=gagScheduledFlags(tick-1,entry);
            gagScheduledThrow(tick,entry);
            check(gagBeeps-count==static_cast<unsigned>(flags==0xc0 && prior!=0xc0),
                "only each original complete-window rising edge emits a targeting cue");
            if(flags==0xc0 && gagBeeps>count) ++crossings;
        }
        check(crossings>1 && sawPartial && sawOutside,"each schedule-derived false-to-true alignment beeps once after partial and empty phases");
        setValue(0x010000d8,3,static_cast<int>(gagScheduledFlags(start,entry)),2);
        check(gagPredicate(0x2550),"manual hit succeeds from the unchanged native simultaneous flag predicate");
        check(utilityValue(utilityScript(),0x010000d8,3,2)==static_cast<int>(gagScheduledFlags(start,entry)) && !writes,
            "manual targeting observation makes no native or save-state writes");
        setValue(0x010000d8,3,0,2);gagQuery(0x24e2);
        const auto guarded=gagBeeps;
        gagFocus=false;gagScheduledThrow(start,entry);gagFocus=true;
        check(gagBeeps==guarded,"focus loss suppresses a fresh targeting-window edge");
        setValue(0x010000d8,3,0,2);gagQuery(0x24e2);
        gagCues=false;gagScheduledThrow(start,entry);gagCues=true;
        check(gagBeeps==guarded,"disabled Gagazet cues suppress a fresh targeting-window edge");
        check(gagazetCommand(L"guide").find(L"calibrat")==std::wstring::npos &&
            gagazetCommand(L"guide").find(L"before the tone")!=std::wstring::npos,
            "guide removes calibration and explains the native target tone versus throw start");
    }
    gagLoad();
    const auto noData=gagBeeps;
    // Change the live original declaration after immutable preparation. A
    // ready cached row is not permission to accept a replaced native operand.
    put(chunk,0x200+2*8,0x010000d9u);
    gagQuery(0x24e2);
    check(gagBeeps==noData,"unavailable native flag data cannot emit a targeting beep");
    resetGagazet();gagAutomatic=false;gagCues=gagFocus=true;
    puts("PASS: original 30-stage Gagazet schedules, raw targeting flags at all three speeds, initial and repeated alignment cues, unchanged manual hit predicate, no writes, and focus/settings/data guards. Animation-to-throw lead remains unmodeled; listen through passes and begin before the tone.");
}
}
