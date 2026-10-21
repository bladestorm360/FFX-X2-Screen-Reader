void airshipPassageFixtures() {
    struct Expected { const char* script; unsigned length,checksum,worker,category; int room; const wchar_t* label; unsigned entries; };
    const Expected expected[] = {
        {"hiku0000",42244,0xffb34f6f,26,1,-2,L"Exit 3",0x10},
        {"hiku0000",42244,0xffb34f6f,27,1,-2,L"Exit 4",0x10},
        {"hiku0000",42244,0xffb34f6f,29,1,-2,L"Elevator up. Confirm to ride",0x14},
        {"hiku0000",42244,0xffb34f6f,30,1,-2,L"Elevator down. Confirm to ride",0x14},
        {"hiku0200",21789,0x858fecc9,20,1,-2,L"Exit 3",0x10},
        {"hiku0200",21789,0x858fecc9,22,1,-2,L"Exit 4",0x10},
        {"hiku0200",21789,0x858fecc9,23,1,-2,L"Exit 5",0x10},
        {"hiku0200",21789,0x858fecc9,24,1,-2,L"Exit 6",0x10},
        {"hiku0500",80410,0x733360dc,23,1,-2,L"Exit 3",0x10},
        {"hiku0500",80410,0x733360dc,25,1,-2,L"Exit 4",0x10},
        {"hiku0500",80410,0x733360dc,26,1,-2,L"Exit 5",0x10},
        {"hiku0500",80410,0x733360dc,27,1,-2,L"Exit 6",0x10},
        {"hiku0500",80410,0x733360dc,28,1,-2,L"Exit 7",0x10},
        {"hiku0500",80410,0x733360dc,29,1,-2,L"Exit 8",0x10},
        {"hiku2000",13902,0x2372f284,14,1,-2,L"Exit 3",0x10},
        {"hiku2000",13902,0x2372f284,15,1,-2,L"Exit 4",0x10},
        {"hiku2000",13902,0x2372f284,17,1,-2,L"Elevator up. Confirm to ride",0x14},
        {"hiku2000",13902,0x2372f284,18,1,-2,L"Elevator down. Confirm to ride",0x14},
        {"bvyt0900",37649,0x97fd6083,30,1,209,L"Exit 1",0x40},
        {"stbv0000",19861,0xada03b86,26,1,208,L"Exit 1",0x10},
    };
    for(const auto& item:expected) {
        AtelScript script; script.name=item.script; script.codeLength=item.length; script.checksum=item.checksum;
        const auto catalog=navigationCatalog(script);
        const auto target=navigationTarget(catalog,item.worker);
        check(catalog && target && target->worker==item.worker && target->category==item.category &&
            target->room==item.room && target->properName==-1 && target->identity==2 &&
            std::wstring(target->label)==item.label && target->entries.words[0]==item.entries,
            "supplemental passage requires exact script identity and preserves authored target contract");
        auto changedLength=script; ++changedLength.codeLength;
        check(!navigationCatalog(changedLength),"supplemental passage rejects changed script length");
        auto changedChecksum=script; changedChecksum.checksum^=1;
        check(!navigationCatalog(changedChecksum),"supplemental passage rejects changed script checksum");
        check(!navigationTarget(catalog,0xffffffffu),"supplemental passage leaves unknown workers uncatalogued");
    }
}