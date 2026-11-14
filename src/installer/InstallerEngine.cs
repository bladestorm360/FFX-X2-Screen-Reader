using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Xml;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;

namespace FfxScreenReader.Setup
{
    internal sealed class InstallerEngine
    {
        public static readonly string[] PayloadNames = {
            "dinput8.dll", "MinHook.x86.dll", "SDL3.dll", "FFXLauncherReader.dll",
            "FFXLauncherNative.dll", "FFX_SR_SDL3_LICENSE.txt",
            "FFX_SR_Native_Dependencies.md"
        };
        private const string ConfigName = "FFX&X-2_LAUNCHER.exe.config";
        private const string AssemblyName = "FFXLauncherReader, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null";
        private const string ManagerType = "FfxLauncher.ReaderDomainManager";
        // Payload files are written here first, then renamed into the game folder.
        internal const string StagePrefix = ".FFX_SR_Install.stage.";
        private const string RecordHeader = "Uninstall record (setup reads the lines below; do not edit):";
        private const string InstalledLine = "Setup result: installed";
        private const string RolledBackLine = "Setup result: rolled back";
        private const string UninstalledLine = "Setup result: uninstalled";
        private readonly Func<string, byte[]> readPayload;
        private readonly IDictionary<string, string> hashes;
        private readonly Func<string, Action<string>, MigrationPlan> planFactory;
        private readonly Action closedProcesses;
        // Other added DLL/ASI files that the last Install left untouched.
        public List<string> LeftInPlace { get; private set; }

        public InstallerEngine(Func<string, byte[]> readPayload, IDictionary<string, string> hashes)
            : this(readPayload, hashes, MigrationPlan.Build) { }

        internal InstallerEngine(Func<string, byte[]> readPayload, IDictionary<string, string> hashes,
            Func<string, Action<string>, MigrationPlan> planFactory)
            : this(readPayload, hashes, planFactory, RequireClosedProcesses) { }

        // Fixture-only callers may provide a guard; the production constructors
        // always use the real game/launcher process check.
        internal InstallerEngine(Func<string, byte[]> readPayload, IDictionary<string, string> hashes,
            Func<string, Action<string>, MigrationPlan> planFactory, Action closedProcesses)
        {
            this.readPayload = readPayload;
            this.hashes = hashes;
            this.planFactory = planFactory;
            this.closedProcesses = closedProcesses;
            LeftInPlace = new List<string>();
        }

        public static string Hash(byte[] bytes)
        {
            using (SHA256 sha = SHA256.Create())
                return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
        }

        public static void RequireClosedProcesses()
        {
            foreach (string name in new[] { "FFX", "FFX-2", "FFX-2_LM", "FFX&X-2_Will", "FFX&X-2_LAUNCHER" })
            {
                Process[] processes = Process.GetProcessesByName(name);
                bool running = processes.Length != 0;
                foreach (Process process in processes) process.Dispose();
                if (running) throw new InvalidOperationException("Close the game and launcher before installation. No files were changed.");
            }
        }

        private static XmlDocument PrepareConfiguration(string path, ICollection<string> removed)
        {
            var xml = new XmlDocument { PreserveWhitespace = true, XmlResolver = null };
            var settings = new XmlReaderSettings { DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null };
            if (File.Exists(path))
                using (var stream = File.OpenRead(path))
                using (XmlReader reader = XmlReader.Create(stream, settings)) xml.Load(reader);
            else xml.LoadXml("<?xml version=\"1.0\" encoding=\"utf-8\"?><configuration/>");
            if (xml.DocumentElement.Name != "configuration") throw new InvalidOperationException("Unexpected launcher configuration root. No files were changed.");
            if (xml.SelectNodes("/configuration/runtime").Count > 1)
                throw new InvalidOperationException("Duplicate launcher runtime sections. No files were changed.");
            XmlNode runtime = xml.SelectSingleNode("/configuration/runtime");
            if (runtime == null) runtime = xml.DocumentElement.AppendChild(xml.CreateElement("runtime"));
            XmlNode foreign = runtime.SelectSingleNode("appDomainManagerAssembly");
            bool migrating = false;
            if (foreign is XmlElement) {
                string assembly = ((XmlElement)foreign).GetAttribute("value").Split(',')[0].Trim();
                migrating = removed.Contains(assembly + ".dll");
            }
            foreach (string[] entry in new[] {
                new[] { "appDomainManagerAssembly", AssemblyName }, new[] { "appDomainManagerType", ManagerType }
            })
            {
                XmlNodeList nodes = runtime.SelectNodes(entry[0]);
                if (nodes.Count > 1) throw new InvalidOperationException("Duplicate launcher manager entries. No files were changed.");
                XmlElement node = nodes.Count == 0 ? null : nodes[0] as XmlElement;
                if (node != null && node.GetAttribute("value") != entry[1] && !migrating)
                    throw new InvalidOperationException("Launcher already has another AppDomainManager. No files were changed.");
                if (node == null) node = (XmlElement)runtime.AppendChild(xml.CreateElement(entry[0]));
                node.SetAttribute("value", entry[1]);
            }
            return xml;
        }

        public void ValidateDependencies()
        { new PrismInstall(readPayload, closedProcesses, delegate { }).ValidatePackage(); }

        public string Install(string directory, Action<string> status)
        {
            string game = InstallerPaths.Full(directory);
            if (!Directory.Exists(game)) throw new InvalidOperationException("Select the installed Steam game folder. No files were changed.");
            closedProcesses();
            var prism = new PrismInstall(readPayload, closedProcesses, delegate { });
            prism.ValidatePackage();
            var payload = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
            foreach (string name in PayloadNames)
            {
                string expected;
                if (!hashes.TryGetValue(name, out expected)) throw new InvalidOperationException("Incomplete installer checksum list: " + name);
                byte[] bytes = readPayload(name);
                if (bytes == null || !String.Equals(Hash(bytes), expected, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("Installer checksum failed: " + name + ". Download a fresh installer.");
                payload.Add(name, bytes);
            }
            MigrationPlan plan = planFactory(game, status);
            LeftInPlace = new List<string>(plan.LeftInPlace);
            var removed = new HashSet<string>(plan.Remove, StringComparer.OrdinalIgnoreCase);
            removed.UnionWith(MigrationPlan.LegacyNames);
            var affected = new List<string>(PayloadNames);
            affected.AddRange(new[] { ConfigName, ConfigName + ".ffx_sr_original", ConfigName + ".ffx_sr_pending" });
            affected.AddRange(removed);
            affected = new List<string>(new HashSet<string>(affected, StringComparer.OrdinalIgnoreCase));
            foreach (string name in affected) InstallerPaths.CheckOrdinaryFile(game, name);
            string config = Path.Combine(game, ConfigName);
            XmlDocument xml = PrepareConfiguration(config, removed);
            string backupRoot = Path.Combine(game, "FFX_SR_Install_Backups");
            if (File.Exists(backupRoot) || (Directory.Exists(backupRoot) &&
                (File.GetAttributes(backupRoot) & FileAttributes.ReparsePoint) != 0))
                throw new IOException("The install backup folder is a file or link. No files were changed.");
            string backup = Path.Combine(backupRoot, DateTime.Now.ToString("yyyyMMdd-HHmmss-fffffff") + "-" + Guid.NewGuid().ToString("N"));
            // Complete every backup before changing any file, including PRISM and nested files.
            Directory.CreateDirectory(backup);
            var before = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (string name in affected)
            {
                string target = Path.Combine(game, name);
                if (File.Exists(target)) {
                    string old = InstallerPaths.InGame(backup, name);
                    Directory.CreateDirectory(Path.GetDirectoryName(old));
                    File.Copy(target, old);
                    before[name] = Hash(File.ReadAllBytes(old));
                }
            }
            plan.WriteReport(game, backup);
            string report = Path.Combine(backup, "INSTALL_REPORT.txt");
            if (LeftInPlace.Count != 0) status("Left in place: " + String.Join(", ", LeftInPlace.ToArray()));
            closedProcesses();
            bool prismInstalled = false;
            string stage = Path.Combine(game, StagePrefix + Guid.NewGuid().ToString("N"));
            try
            {
                // Installs a missing PRISM runtime without replacing an existing one.
                prismInstalled = prism.Install(game, status).StartsWith("Installed ", StringComparison.Ordinal);
                foreach (string name in removed) {
                    status("Removing old mod file: " + name);
                    InstallerPaths.CheckOrdinaryFile(game, name);
                    File.Delete(InstallerPaths.InGame(game, name));
                }
                Directory.CreateDirectory(stage);
                foreach (string name in PayloadNames)
                {
                    string staged = Path.Combine(stage, name);
                    File.WriteAllBytes(staged, payload[name]);
                    if (!String.Equals(Hash(File.ReadAllBytes(staged)), hashes[name], StringComparison.OrdinalIgnoreCase))
                        throw new IOException("Staged file checksum failed: " + name);
                }
                foreach (string name in PayloadNames)
                {
                    status("Installing " + name);
                    InstallerPaths.CheckOrdinaryFile(game, name);
                    File.Move(Path.Combine(stage, name), Path.Combine(game, name));
                }
                status("Configuring launcher speech");
                string pending = config + ".ffx_sr_pending";
                using (var stream = new FileStream(InstallerPaths.Native(pending), FileMode.Create, FileAccess.Write, FileShare.None)) xml.Save(stream);
                PrepareConfiguration(pending, new string[0]); // Parse the staged merged XML before changing the live configuration.
                string original = config + ".ffx_sr_original";
                if (!File.Exists(original))
                {
                    if (File.Exists(config)) File.Copy(config, original);
                    else File.WriteAllText(original, "<?xml version=\"1.0\" encoding=\"utf-8\"?><configuration/>");
                }
                File.Copy(pending, config, true);
                File.Delete(pending);
                foreach (string name in PayloadNames)
                    if (!String.Equals(Hash(File.ReadAllBytes(Path.Combine(game, name))), hashes[name], StringComparison.OrdinalIgnoreCase))
                        throw new IOException("Installed file checksum failed: " + name);
                plan.Verify(game, status);
                var record = new List<string> { RecordHeader };
                foreach (string name in affected)
                {
                    string target = Path.Combine(game, name);
                    string was;
                    record.Add("file\t" + name + "\t" + (before.TryGetValue(name, out was) ? was : "absent") + "\t" +
                        (File.Exists(target) ? Hash(File.ReadAllBytes(target)) : "absent"));
                }
                record.Add("prism\t" + (prismInstalled ? "installed" : "kept"));
                record.Add(InstalledLine);
                File.AppendAllLines(report, record);
            }
            catch (Exception failure)
            {
                var restoreFailures = new List<string>();
                foreach (string name in affected)
                {
                    try
                    {
                        string old = Path.Combine(backup, name);
                        InstallerPaths.CheckOrdinaryFile(game, name);
                        string target = InstallerPaths.InGame(game, name);
                        if (File.Exists(old)) {
                            Directory.CreateDirectory(Path.GetDirectoryName(target));
                            File.Copy(old, target, true);
                        }
                        else File.Delete(target);
                    }
                    catch (Exception restore) { restoreFailures.Add(name + ": " + restore.Message); }
                }
                if (prismInstalled)
                {
                    try { prism.Remove(game); }
                    catch (Exception restore) { restoreFailures.Add("PrismRuntime: " + restore.Message); }
                }
                // Uninstall skips this backup; a missing marker makes uninstall refuse instead.
                try { File.AppendAllLines(report, new[] { RolledBackLine }); }
                catch (Exception mark) { restoreFailures.Add("INSTALL_REPORT.txt: " + mark.Message); }
                string result = restoreFailures.Count == 0 ? "Previous files were restored." :
                    "Some files could not be restored: " + String.Join("; ", restoreFailures.ToArray());
                throw new IOException("Installation failed. " + result + " Backup: " + backup + ". " + failure.Message, failure);
            }
            finally { RemoveStage(stage, status); }
            return backup;
        }

        // Undoes the newest completed installation. Every file is checked against the
        // uninstall record first; if anything differs, nothing is changed.
        public string Uninstall(string directory, Action<string> status)
        {
            string game = InstallerPaths.Full(directory);
            if (!Directory.Exists(game)) throw UninstallRefused("Select the installed Steam game folder.");
            closedProcesses();
            string backup = NewestInstall(game);
            string report = Path.Combine(backup, "INSTALL_REPORT.txt");
            var owned = new HashSet<string>(PayloadNames, StringComparer.OrdinalIgnoreCase);
            owned.UnionWith(MigrationPlan.LegacyNames);
            owned.UnionWith(new[] { ConfigName, ConfigName + ".ffx_sr_original", ConfigName + ".ffx_sr_pending" });
            var files = new List<string[]>();
            string prismState = null;
            bool inRecord = false, complete = false;
            foreach (string line in File.ReadAllLines(report))
            {
                if (line == RecordHeader) { inRecord = true; continue; }
                if (!inRecord) continue;
                string[] parts = line.Split('\t');
                if (complete) throw UninstallRefused("The install report has lines after its result: " + report + ".");
                if (line == InstalledLine) complete = true;
                else if (parts.Length == 4 && parts[0] == "file" && owned.Contains(parts[1])) files.Add(parts);
                else if (parts.Length == 2 && parts[0] == "prism" && (parts[1] == "installed" || parts[1] == "kept") && prismState == null) prismState = parts[1];
                else throw UninstallRefused("The install report is not valid: " + report + ".");
            }
            if (!complete || prismState == null || files.Count == 0)
                throw UninstallRefused("The install report is incomplete: " + report + ".");
            foreach (string[] file in files)
            {
                InstallerPaths.CheckOrdinaryFile(game, file[1]);
                if (HashOrAbsent(InstallerPaths.InGame(game, file[1])) != file[3])
                    throw UninstallRefused(file[1] + " changed after setup.");
                if (file[2] != "absent" && HashOrAbsent(InstallerPaths.InGame(backup, file[1])) != file[2])
                    throw UninstallRefused("The backup of " + file[1] + " is missing or changed.");
            }
            var prism = new PrismInstall(readPayload, closedProcesses, delegate { });
            if (prismState == "installed")
            {
                try { prism.Verify(game); }
                catch (Exception ex) { throw UninstallRefused("PrismRuntime changed after setup. " + ex.Message); }
            }
            closedProcesses();
            try
            {
                foreach (string[] file in files)
                {
                    string target = InstallerPaths.InGame(game, file[1]);
                    if (file[2] == "absent") { if (File.Exists(target)) { status("Removing " + file[1]); File.Delete(target); } }
                    else if (file[2] != file[3]) { status("Restoring " + file[1]); File.Copy(InstallerPaths.InGame(backup, file[1]), target, true); }
                }
                if (prismState == "installed") { status("Removing PrismRuntime"); prism.Remove(game); }
                File.AppendAllLines(report, new[] { UninstalledLine });
            }
            catch (Exception ex)
            {
                throw new IOException("Uninstall did not finish. Some files may still be from the screen reader. Backup: " + backup + ". " + ex.Message, ex);
            }
            return "Uninstall complete. Previous files were restored from: " + backup;
        }

        private static string NewestInstall(string game)
        {
            string root = Path.Combine(game, "FFX_SR_Install_Backups");
            if (!Directory.Exists(root) || (File.GetAttributes(root) & FileAttributes.ReparsePoint) != 0)
                throw UninstallRefused("No setup backup was found in this game folder.");
            string[] folders = Directory.GetDirectories(root);
            Array.Sort(folders, StringComparer.OrdinalIgnoreCase);
            for (int i = folders.Length - 1; i >= 0; i--)
            {
                string report = Path.Combine(folders[i], "INSTALL_REPORT.txt");
                if (!File.Exists(report)) throw UninstallRefused("The newest setup backup has no install report: " + folders[i] + ".");
                string[] lines = File.ReadAllLines(report);
                if (Array.IndexOf(lines, UninstalledLine) < 0 && Array.IndexOf(lines, RolledBackLine) < 0) return folders[i];
            }
            throw UninstallRefused("No installation to undo was found.");
        }

        private static string HashOrAbsent(string path)
        {
            return File.Exists(path) ? Hash(File.ReadAllBytes(path)) : "absent";
        }

        private static Exception UninstallRefused(string reason)
        {
            return new InvalidOperationException("Uninstall stopped: " + reason + " Nothing was changed.");
        }

        private static void RemoveStage(string stage, Action<string> status)
        {
            try
            {
                if (!Directory.Exists(stage)) return;
                foreach (string file in Directory.GetFiles(stage)) File.Delete(file);
                Directory.Delete(stage);
            }
            catch (Exception ex) { status("Could not remove the temporary folder " + stage + ": " + ex.Message); }
        }
    }
}
