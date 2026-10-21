// All executable I/O runs on a launcher worker. UI checks only copied status.
namespace FfxLauncher {
ref class LargeAddressAware abstract sealed {
    static int state=0,announced=0;
    static String^ result;
    static bool Ordinary(String^ path) {
        return (File::GetAttributes(path)&FileAttributes::ReparsePoint)==FileAttributes(0);
    }
    static String^ Digest(array<Byte>^ bytes) {
        auto hash=System::Security::Cryptography::SHA256::Create();
        try { return BitConverter::ToString(hash->ComputeHash(bytes))->Replace("-","")->ToLowerInvariant(); }
        finally { delete hash; }
    }
    static array<Byte>^ ReadImage(String^ path) {
        auto file=gcnew FileStream(path,FileMode::Open,FileAccess::Read,FileShare::Read);
        try {
            if(file->Length<64 || file->Length>64*1024*1024) throw gcnew IOException("Unsupported executable size.");
            auto bytes=gcnew array<Byte>(static_cast<int>(file->Length));
            int position=0;
            while(position<bytes->Length) {
                const int count=file->Read(bytes,position,bytes->Length-position);
                if(!count) throw gcnew IOException("Incomplete executable read.");
                position+=count;
            }
            return bytes;
        } finally { delete file; }
    }
    static void RequireClosed() {
        auto processes=System::Diagnostics::Process::GetProcessesByName("FFX");
        for each(auto process in processes) delete process;
        if(processes->Length) throw gcnew IOException("Close Final Fantasy X before enabling large-address support.");
    }
    static void Check(Object^ argument) {
        String^ stage=nullptr;String^ exe=nullptr;String^ backup=nullptr;String^ failed=nullptr;
        String^ patchedHash=nullptr;bool committed=false;
        try {
            const auto root=Path::GetFullPath(safe_cast<String^>(argument));
            exe=Path::Combine(root,"FFX.exe");
            for(auto parent=gcnew DirectoryInfo(root);parent!=nullptr;parent=parent->Parent)
                if(!Ordinary(parent->FullName)) throw gcnew IOException("The game directory is linked.");
            if(!Directory::Exists(root) || !Ordinary(root) || !File::Exists(exe) || !Ordinary(exe))
                throw gcnew IOException("The game executable is missing or linked.");
            auto stream=gcnew FileStream(exe,FileMode::Open,FileAccess::Read,FileShare::ReadWrite);
            auto header=gcnew array<Byte>(4096);int length=0;
            try {
                while(length<header->Length) {
                    const int count=stream->Read(header,length,header->Length-length);
                    if(!count) break;length+=count;
                }
            } finally { delete stream; }
            ffx::pe::LaaHeader pe;
            { pin_ptr<Byte> bytes=&header[0];
              if(!ffx::pe::inspectLaa(bytes,length,pe)) throw gcnew IOException("Unsupported executable header."); }
            if(pe.enabled) { System::Threading::Interlocked::Exchange(state,2); return; }
            RequireClosed();
            auto original=ReadImage(exe);
            const auto originalHash=Digest(original);
            auto patched=safe_cast<array<Byte>^>(original->Clone());
            { pin_ptr<Byte> bytes=&patched[0];
              if(!ffx::pe::enableLaa(bytes,patched->Length)) throw gcnew IOException("Unsupported executable header."); }
            patchedHash=Digest(patched);
            if(originalHash!="3d13e5ed53821df8db204cd3c27d470c5c220b18e16849fc4440996c15f277e1" &&
                patchedHash!="0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d")
                throw gcnew IOException("This executable version is not verified for an automatic change.");
            const auto backupRoot=Path::Combine(root,"FFX_SR_LAA_Backups");
            if(Directory::Exists(backupRoot) && !Ordinary(backupRoot)) throw gcnew IOException("The backup directory is linked.");
            Directory::CreateDirectory(backupRoot);
            const auto identity=Guid::NewGuid().ToString("N");
            backup=Path::Combine(backupRoot,"FFX-"+identity+"-"+originalHash+".original");
            failed=Path::Combine(backupRoot,"FFX-"+identity+".failed");
            stage=Path::Combine(backupRoot,"FFX-"+identity+".pending");
            auto output=gcnew FileStream(stage,FileMode::CreateNew,FileAccess::Write,FileShare::None);
            try { output->Write(patched,0,patched->Length);output->Flush(true); } finally { delete output; }
            if(Digest(ReadImage(stage))!=patchedHash || Digest(ReadImage(exe))!=originalHash)
                throw gcnew IOException("The executable changed during the check. No change was applied.");
            RequireClosed();
            // Atomic NTFS replacement preserves a complete original backup even
            // if the launcher is closed during the operation.
            File::Replace(stage,exe,backup,true);stage=nullptr;committed=true;
            if(Digest(ReadImage(exe))!=patchedHash || Digest(ReadImage(backup))!=originalHash)
                throw gcnew IOException("Verification failed. The original backup is in FFX_SR_LAA_Backups.");
            result="Large-address support enabled. The original executable was backed up.";
            System::Threading::Interlocked::Exchange(state,2);
        } catch(Exception^ error) {
            // Restore only our exact replacement. Preserve concurrent edits.
            if(committed) {
                try {
                    if(Ordinary(exe) && Ordinary(backup) && Digest(ReadImage(exe))==patchedHash)
                        File::Replace(backup,exe,failed,true);
                } catch(Exception^) {}
            }
            result="Large-address check: "+error->Message;
            System::Threading::Interlocked::Exchange(state,3);
        } finally {
            if(stage!=nullptr) { try { if(File::Exists(stage) && Ordinary(stage)) File::Delete(stage); } catch(Exception^) {} }
        }
    }
public:
    static void Begin(String^ root) {
        if(System::Threading::Interlocked::CompareExchange(state,1,0)!=0) return;
        try { System::Threading::ThreadPool::QueueUserWorkItem(gcnew System::Threading::WaitCallback(&LargeAddressAware::Check),root); }
        catch(Exception^) { result="Large-address check could not start.";System::Threading::Interlocked::Exchange(state,3); }
    }
    static bool AllowLaunch(String^% notice) {
        const auto current=System::Threading::Thread::VolatileRead(state);
        if(current<2) { notice="Checking large-address support. Try again in a moment.";return false; }
        notice=System::Threading::Interlocked::Exchange(announced,current)==current?nullptr:result;
        return true; // A failed optional flag check preserves the original launch route.
    }
};
}
