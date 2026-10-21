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
        private static readonly string[] LegacyNames = {
            "frida-gadget.dll", "frida-gadget.config", "frida-gadget.config.dll", "ffx_sr.js", "Tolk.dll"
        };
        private readonly Func<string, byte[]> readPayload;
        private readonly IDictionary<string, string> hashes;
        private readonly Func<string, Action<string>, MigrationPlan> planFactory;
        private readonly Action closedProcesses;

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
            var removed = new HashSet<string>(plan.Remove, StringComparer.OrdinalIgnoreCase);
            removed.UnionWith(LegacyNames);
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
            string backup = Path.Combine(backupRoot, DateTime.Now.ToString("yyyyMMdd-HHmmss") + "-" + Guid.NewGuid().ToString("N"));
            // Main installer preflight is complete; install a missing dependency
            // without replacing any existing SDK. Stock/mod files are untouched here.
            prism.Install(game, status);
            // Complete every backup before changing installed mod files, including nested added modules.
            Directory.CreateDirectory(backup);
            foreach (string name in affected)
            {
                string target = Path.Combine(game, name);
                if (File.Exists(target)) {
                    string old = InstallerPaths.InGame(backup, name);
                    Directory.CreateDirectory(Path.GetDirectoryName(old));
                    File.Copy(target, old);
                }
            }
            plan.WriteReport(game, backup);
            closedProcesses();
            try
            {
                foreach (string name in removed) {
                    status("Removing old mod file: " + name);
                    InstallerPaths.CheckOrdinaryFile(game, name);
                    File.Delete(InstallerPaths.InGame(game, name));
                }
                foreach (string name in PayloadNames)
                {
                    status("Installing " + name);
                    InstallerPaths.CheckOrdinaryFile(game, name);
                    File.WriteAllBytes(Path.Combine(game, name), payload[name]);
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
                string result = restoreFailures.Count == 0 ? "Previous files were restored." :
                    "Some files could not be restored: " + String.Join("; ", restoreFailures.ToArray());
                throw new IOException("Installation failed. " + result + " Backup: " + backup + ". " + failure.Message, failure);
            }
            return backup;
        }
    }
}
