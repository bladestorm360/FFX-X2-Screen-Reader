namespace {
void audioEvent(unsigned rva,Event& e,bool before) {
    for(const auto& l:listeners.at(rva)) {const auto fn=before?l.before:l.after;if(fn)fn(e);}
}
void soundStart(unsigned id,unsigned slot,bool success=true,unsigned count=1,unsigned volume=127) {
    std::thread audio([&]{
        audioOwner.store(GetCurrentThreadId());
        auto& group=audioGroups[slot];group.count=group.capacity=count;
        group.records=reinterpret_cast<uintptr_t>(audioRecords[slot]);
        for(unsigned i=0;i<count;++i) audioRecords[slot][i]={++nextAudioEvent,0};
        Event root;root.args[0]=id;root.epoch=epoch;
        Event playFrame;playFrame.ecx=reinterpret_cast<uintptr_t>(&group);playFrame.epoch=epoch;playFrame.args[0]=volume;
        nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
        audioEvent(jecht_voice_rva::Start,root,true);audioEvent(jecht_voice_rva::Events,playFrame,true);
        // Recovered original SfxEvent::start: 8-byte records, playing byte1,
        // callback user data is record+4, then the stdcall FMOD start result.
        // Volume/pan SDK calls are omitted; original ready-array semantics and
        // callback/result ordering are retained. No real FMOD backend runs.
        for(unsigned i=0;i<count;++i) {
            audioRecords[slot][i].playing=1;
            Event sdk;sdk.args[0]=audioRecords[slot][i].event;sdk.parentRva=jecht_voice_rva::Events;sdk.epoch=epoch;sdk.result=success?0:1;
            audioEvent(0x548c9a,sdk,false);
        }
        audioEvent(jecht_voice_rva::Events,playFrame,false);audioEvent(jecht_voice_rva::Start,root,false);
        nativeHeapFixture::observing=false;
        check(!nativeHeapFixture::allocations && !nativeHeapFixture::frees,"audio capture Native heap0/0");
    });audio.join();
}
void soundEnd(unsigned slot,unsigned recordIndex=0,unsigned kind=2,unsigned oldEpoch=0,uintptr_t oldEvent=0) {
    std::thread callback([&]{
        auto& record=audioRecords[slot][recordIndex];Event e;e.args[0]=oldEvent?oldEvent:record.event;
        e.args[1]=kind;e.args[4]=reinterpret_cast<uintptr_t>(&record.playing);e.epoch=oldEpoch?oldEpoch:epoch;
        // Exact original 31-byte stdcall callback clears the native user word.
        if(kind==2 || kind==4) record.playing=0;
        audioEvent(0x307b40,e,false);
    });callback.join();
}
void soundDispose(unsigned slot) {
    Event e;e.ecx=reinterpret_cast<uintptr_t>(&audioGroups[slot]);e.epoch=epoch;
    audioEvent(jecht_voice_rva::Dispose,e,true);
}
void soundSource(const JechtCue& cue) {
    const auto& s=jechtScripts[cue.script];location(s.local,s.entry,s.entryPc);
    Event root;root.args[0]=ptr(worker);root.epoch=epoch;
    audioEvent(0x464180,root,true);
    // One interpreter invocation runs earlier instructions before reaching DF.
    location(s.local,s.entry,cue.soundPc);
    const auto pc=chunk()+read<unsigned>(chunk()+0x30)+cue.soundPc;
    check(read<uint8_t>(pc)==0xd8 && read<uint16_t>(pc+1)==0xdf,"original sound opcode/ID");
    Event command;command.parentRva=0x464180;command.epoch=epoch;command.args[0]=cue.sequence;
    audioEvent(jecht_rva::Sound,command,true);
    audioEvent(0x464180,root,false);
}
JechtCaption showCue(const JechtCue& cue) {
    const auto header=chunk()+read<unsigned>(chunk()+0x38+cue.local*4);
    const auto entries=read<unsigned>(header+0x20);
    const auto entryPc=read<unsigned>(chunk()+entries+cue.entry*4);
    location(cue.local,cue.entry,entryPc);
    Event root;root.args[0]=ptr(worker);root.epoch=epoch;
    audioEvent(0x464180,root,true);
    // Recovered 864180 loop executes position/transparent/text/flags first.
    // Only the actual Common006A callback sees the later show instruction.
    location(cue.local,cue.entry,cue.pc);
    Event command;command.args[0]=ptr(worker);command.epoch=epoch;command.parentRva=0x464180;
    const auto prior=plans;audioEvent(jecht_rva::Show,command,true);
    check(plans==prior,"cue waits for the original visible producer");
    const auto token=captureJechtCaption(cue.window);
    check(!captureJechtCaption(7).epoch,"other message windows cannot inherit Jecht audio");
    audioEvent(jecht_rva::Show,command,false);
    check(!captureJechtCaption(cue.window).epoch,"caption scope ends with original show call");
    put(worker,0x12c+0x4c+0x18,chunk()+read<unsigned>(chunk()+0x30)+cue.pc+3);
    audioEvent(0x464180,root,false);return token;
}
void pacingFixtures(unsigned variant) {
    const auto& s=jechtScripts[variant];
    const auto cue=*std::find_if(std::begin(jechtCues),std::end(jechtCues),[&](const auto& x){return x.script==variant;});
    const auto q=*std::find_if(std::begin(jechtQueries),std::end(jechtQueries),[&](const auto& x){return x.script==variant && x.slot==0 && x.value==0 && x.call==0x51;});
    auto immediate=[&](bool physical=false){return queryAt(q,physical,0x464180,0,false);};
    automatic=true;resetJecht();value(s.selector[0],0);value(s.active[0],1);value(s.timer,1);
    soundSource(cue);const auto token=showCue(cue);
    check(token.sequence==cue.sequence && !jechtCaptionState(token).ready,"actual visible caption waits only for its sound result");
    soundStart(cue.sequence,0);check(jechtCaptionState(token).ready && jechtCaptionState(token).voiced,"successful exact SFX classifies matching subtitle voiced");
    ticks+=749;check(!immediate().overrideResult,"automatic response remains below750ms boundary");
    ticks+=1;check(immediate().overrideResult,"automatic response at750ms allows natural voice overlap");
    value(s.score,11);check(!immediate().overrideResult,"original score11 stops even an admitted prompt");
    value(s.score,255);check(!immediate().overrideResult,"out-of-range original score cannot synthesize input");
    value(s.score,10);check(immediate().overrideResult,"original score10 still permits the eleventh success");value(s.score,0);
    check(jechtVoiceState(cue.sequence,epoch).playing,"response does not wait for whole clip completion");
    soundEnd(0);check(jechtCaptionState(token).voiced,"caption keeps successful playback provenance after clip completion");
    const auto old=audioRecords[0][0].event;
    soundSource(cue);const auto repeated=showCue(cue);soundStart(cue.sequence,0);soundEnd(0,0,2,0,old);
    check(jechtVoiceState(cue.sequence,epoch).playing && jechtCaptionState(repeated).voiced,"stale callback cannot alter the next identical request");
    ticks+=750;check(immediate().overrideResult,"equal next prompt has independent750ms response");soundDispose(0);
    soundSource(cue);const auto failed=showCue(cue);soundStart(cue.sequence,0,false);
    check(jechtCaptionState(failed).ready && !jechtCaptionState(failed).voiced,"failed SFX uses ordinary subtitle speech");
    ticks+=750;check(immediate().overrideResult,"failed audio does not delay auto-success");
    soundSource(cue);const auto silent=showCue(cue);soundStart(cue.sequence,0,true,1,0);
    check(jechtCaptionState(silent).ready && !jechtCaptionState(silent).voiced,"zero original group volume keeps subtitle speech");
    soundDispose(0);
    soundSource(cue);const auto disabled=showCue(cue);
    Event root;root.args[0]=cue.sequence;root.epoch=epoch;
    audioEvent(jecht_voice_rva::Start,root,true);audioEvent(jecht_voice_rva::Start,root,false);
    check(jechtCaptionState(disabled).ready && !jechtCaptionState(disabled).voiced,"disabled/missing native sequence publishes failed playback");
    soundSource(cue);const auto absent=showCue(cue);ticks+=499;
    check(!immediate().overrideResult && !jechtCaptionState(absent).ready,"absent sound keeps independent reaction/subtitle deadlines");
    ticks+=1;check(jechtCaptionState(absent).ready && !jechtCaptionState(absent).voiced,"absent playback subtitle fallback bounded500ms");
    ticks+=250;check(immediate().overrideResult,"missing sound cannot postpone the750ms input response");
    soundSource(cue);soundStart(cue.sequence,0,true,16);ticks+=750;
    check(immediate().overrideResult,"sixteen playing events cannot delay reaction pacing");
    for(unsigned i=0;i<15;++i)soundEnd(0,i);check(jechtVoiceState(cue.sequence,epoch).playing,"sixteen-event capture remains bounded/exact");
    soundEnd(0,15);check(!jechtVoiceState(cue.sequence,epoch).playing,"last original completion retires the group");
    soundSource(cue);showCue(cue);automatic=false;
    check(immediate(true).result && !immediate(true).overrideResult,"manual physical input has no pacing delay");
    automatic=true;resetJecht();check(!immediate().overrideResult,"reset cannot reuse an old request");ticks+=749;
    check(!immediate().overrideResult,"new request after reset has own reaction budget");ticks+=1;
    check(immediate().overrideResult,"bounded lazy assistance recovers after reset");
    automatic=false;resetJecht();
    const auto right=*std::find_if(std::begin(jechtCues),std::end(jechtCues),[&](const auto& x){return x.script==variant && x.local==s.local+2;});
    soundSource(cue);showCue(cue);check(lastInterrupt,"first manual cue interrupts old narration immediately");
    soundSource(right);showCue(right);check(!lastInterrupt,"second pending manual cue cannot cut off the first controls");
    value(s.active[0],0);soundSource(right);showCue(right);
    check(lastInterrupt,"answered other slot cannot hold up the next manual control cue");
    value(s.active[0],1);
    printf("PASS: original multi-opcode interpreter/show/sound route, matching SFX subtitle result,750ms reaction and original score11 cutoff, no manual delay, equal repeats,16-event/owner/failure/reset/epoch/instance guards.\n");
}
}
