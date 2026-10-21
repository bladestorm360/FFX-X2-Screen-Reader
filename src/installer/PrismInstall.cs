using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

namespace FfxScreenReader.Setup
{
    // Dependency component of the single mod installer. Existing PRISM
    // runtimes are verified without replacement or rollback.
    internal sealed class PrismInstall
    {
        private readonly Func<string, byte[]> resource;
        private readonly byte[] sourceLock;
        private readonly PrismPrerequisite.Package package;
        private readonly Action closedProcesses;
        private readonly Action<string> validateGame;

        public PrismInstall(Func<string, byte[]> resource)
            : this(resource, InstallerEngine.RequireClosedProcesses,
                delegate(string game) { MigrationPlan.Build(game, delegate { }); }) { }

        internal PrismInstall(Func<string, byte[]> resource, Action closedProcesses,
            Action<string> validateGame)
        {
            this.resource = resource;
            this.closedProcesses = closedProcesses;
            this.validateGame = validateGame;
            sourceLock = resource("prism.lock.json");
            package = PrismPrerequisite.ParseLock(sourceLock);
            if (package.Files.Count != 15 || !package.Files.ContainsKey("prism.dll"))
                throw new InvalidDataException("The locked PRISM runtime inventory is incomplete.");
        }

        public void ValidatePackage()
        {
            foreach (KeyValuePair<string, string> item in package.Files)
            {
                byte[] bytes = resource("PrismRuntime." + item.Key);
                if (InstallerEngine.Hash(bytes) != item.Value)
                    throw new InvalidDataException("Embedded PRISM checksum failed: " + item.Key);
                if (item.Key == "prism.dll")
                {
                    if (bytes.Length < 64 || bytes[0] != 0x4d || bytes[1] != 0x5a)
                        throw new InvalidDataException("Embedded PRISM is not a PE image.");
                    int offset = BitConverter.ToInt32(bytes, 60);
                    if (offset < 0 || offset > bytes.Length - 6 ||
                        BitConverter.ToUInt32(bytes, offset) != 0x4550 ||
                        BitConverter.ToUInt16(bytes, offset + 4) != 0x14c)
                        throw new InvalidDataException("Embedded PRISM must be x86.");
                }
            }
        }

        private static void CheckAncestors(string game)
        {
            for (string check = game; !String.IsNullOrEmpty(check); check = Path.GetDirectoryName(check))
                if ((File.GetAttributes(check) & FileAttributes.ReparsePoint) != 0)
                    throw new IOException("Linked installation paths are unsupported. No files were changed.");
        }

        private string Receipt(string game)
        {
            var data = new Dictionary<string, object> {
                { "schema", 1 }, { "game_directory", game },
                { "platform", "windows" }, { "architecture", "x86" },
                { "package_id", package.Id }, { "version", package.Version },
                { "source_commit", package.Commit },
                { "owners", new[] { "FFX-native", "FFX-launcher" } },
                { "files", package.Files }
            };
            return new JavaScriptSerializer().Serialize(data);
        }

        public string Install(string directory, Action<string> status)
        {
            string game = InstallerPaths.Full(directory).TrimEnd(Path.DirectorySeparatorChar);
            closedProcesses();
            if (!Directory.Exists(game)) throw new DirectoryNotFoundException("Select an existing FFX game folder.");
            CheckAncestors(game);
            validateGame(game);
            ValidatePackage();
            string destination = InstallerPaths.InGame(game, "PrismRuntime");
            InstallerPaths.CheckOrdinaryFile(game, "PrismRuntime/prism-install.json");
            if (File.Exists(destination)) throw new IOException("PrismRuntime is a file. Nothing was changed.");
            string mutexName = "Local\\PrismRuntime-" +
                InstallerEngine.Hash(Encoding.UTF8.GetBytes(game.ToUpperInvariant())).ToUpperInvariant();
            using (var mutex = new Mutex(false, mutexName))
            {
                bool held = false;
                try
                {
                    try { held = mutex.WaitOne(0); }
                    catch (AbandonedMutexException) { held = true; }
                    if (!held) throw new IOException("Another PRISM installation is active. Nothing was changed.");
                    if (Directory.Exists(destination) && Directory.GetFileSystemEntries(destination).Length != 0)
                    {
                        // Exact matches are read-only; incompatible or modified
                        // packages are refused before creating a staging folder.
                        PrismPrerequisite.Require(game, sourceLock);
                        status("PRISM runtime already verified. No files were changed.");
                        return "Already verified; zero writes";
                    }
                    string stageName = ".PrismRuntime.stage." + Guid.NewGuid().ToString("N");
                    string stage = InstallerPaths.InGame(game, stageName);
                    InstallerPaths.CheckOrdinaryFile(game, stageName + "/prism-install.json");
                    Directory.CreateDirectory(stage);
                    try
                    {
                        status("Installing the locked x86 PRISM runtime.");
                        foreach (KeyValuePair<string, string> item in package.Files)
                        {
                            string path = InstallerPaths.InGame(game, stageName + "/" + item.Key);
                            byte[] bytes = resource("PrismRuntime." + item.Key);
                            if (InstallerEngine.Hash(bytes) != item.Value)
                                throw new InvalidDataException("Embedded PRISM checksum failed: " + item.Key);
                            File.WriteAllBytes(path, bytes);
                            if (InstallerEngine.Hash(File.ReadAllBytes(path)) != item.Value)
                                throw new IOException("Staged PRISM verification failed: " + item.Key);
                        }
                        File.WriteAllBytes(InstallerPaths.InGame(game, stageName + "/prism-install.json"),
                            new UTF8Encoding(false).GetBytes(Receipt(game)));
                        closedProcesses();
                        CheckAncestors(game);
                        InstallerPaths.CheckOrdinaryFile(game, stageName + "/prism-install.json");
                        InstallerPaths.CheckOrdinaryFile(game, "PrismRuntime/prism-install.json");
                        // Both resolved paths are children of this exact game
                        // directory. An existing nonempty directory is never removed.
                        if (Directory.Exists(destination)) Directory.Delete(destination);
                        Directory.Move(stage, destination);
                        PrismPrerequisite.Require(game, sourceLock);
                    }
                    catch
                    {
                        status("PRISM installation stopped. Any new staged files are retained at " + stage +
                            ". Existing runtimes and mod files were not replaced.");
                        throw;
                    }
                    status("PRISM runtime and licenses installed. Continuing mod installation.");
                    return "Installed " + package.Id;
                }
                finally { if (held) mutex.ReleaseMutex(); }
            }
        }
    }
}
