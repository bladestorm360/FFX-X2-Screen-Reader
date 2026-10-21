using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using File = FfxScreenReader.Setup.InstallerFiles;
using Directory = FfxScreenReader.Setup.InstallerDirectories;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Xml;
using System.Windows.Forms;
using FfxScreenReader.Setup;

internal static class InstallerTests
{
    private static string fixture;
    private static readonly Dictionary<string, byte[]> payload = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
    private static readonly Dictionary<string, string> hashes = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    private static int checks;

    private static void Assert(bool value, string message)
    {
        if (!value) throw new Exception(message);
        checks++;
    }

    private static void Reject(Action action, string message)
    {
        bool rejected = false;
        try { action(); }
        catch (Exception ex) { if (ex.Message.IndexOf(message, StringComparison.OrdinalIgnoreCase) < 0) throw; rejected = true; }
        Assert(rejected, "Expected refusal: " + message);
    }

    private static string Game(string name)
    {
        string path = Path.Combine(fixture, name);
        Directory.CreateDirectory(path);
        File.WriteAllText(Path.Combine(path, "FFX.exe"), "Synthetic fixture; not a game executable.");
        File.WriteAllText(Path.Combine(path, "FFX&X-2_LAUNCHER.exe"), "Synthetic fixture; not a launcher executable.");
        Directory.CreateDirectory(Path.Combine(path, "magicFiles", "stock"));
        File.WriteAllText(Path.Combine(path, "magicFiles", "stock", "original.dll"), "Synthetic stock DLL");
        PrismTestSupport.Prepare(path);
        return path;
    }

    private static Dictionary<string, string> Snapshot(string game)
    {
        var files = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (string file in Directory.GetFiles(game, "*", SearchOption.AllDirectories)) {
            string name = file.Substring(game.Length + 1);
            if (!name.StartsWith("FFX_SR_Install_Backups" + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                files.Add(name, InstallerEngine.Hash(File.ReadAllBytes(file)));
        }
        return files;
    }

    private static void Unchanged(string game, Dictionary<string, string> before)
    {
        var after = Snapshot(game);
        Assert(before.Count == after.Count, "File count changed after refusal/rollback");
        foreach (var entry in before) Assert(after.ContainsKey(entry.Key) && after[entry.Key] == entry.Value, "Changed file: " + entry.Key);
    }

    private static InstallerEngine Engine()
    {
        return new InstallerEngine(delegate(string name) { return name == "prism.lock.json" || name.StartsWith("PrismRuntime.", StringComparison.Ordinal) ? PrismTestSupport.Resource(name) : payload[name]; }, hashes,
            delegate(string game, Action<string> status) { return MigrationPlan.FromCatalog(game, FixtureCatalog(), status); });
    }

    private static StockFile Stock(string name, string content) {
        byte[] bytes = Encoding.UTF8.GetBytes(content);
        using (var sha = SHA1.Create()) return new StockFile(name, BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", ""), bytes.Length);
    }
    private static BaseGameCatalog FixtureCatalog() {
        return new BaseGameCatalog("synthetic", new[] {
            Stock("FFX.exe", "Synthetic fixture; not a game executable."),
            Stock("FFX&X-2_LAUNCHER.exe", "Synthetic fixture; not a launcher executable."),
            Stock("magicFiles/stock/original.dll", "Synthetic stock DLL")
        });
    }

    private static byte[] SyntheticPe()
    {
        byte[] bytes = new byte[512];
        bytes[0] = (byte)'M'; bytes[1] = (byte)'Z';
        bytes[0x3c] = 64;
        bytes[64] = (byte)'P'; bytes[65] = (byte)'E';
        bytes[68] = 0x4c; bytes[69] = 0x01; // IMAGE_FILE_MACHINE_I386
        bytes[84] = 0xe0; // Optional header size
        bytes[86] = 0x02; bytes[87] = 0x01; // Characteristics 0x0102
        bytes[88] = 0x0b; bytes[89] = 0x01; // PE32 optional header
        return bytes;
    }

    private static string Sha1(byte[] bytes)
    {
        using (var sha = SHA1.Create()) return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "");
    }

    private static bool StockMatches(string name, byte[] bytes, string expectedSha1)
    {
        string root = Path.Combine(fixture, "LAA identity", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        string relative = InstallerPaths.Relative(name);
        string path = InstallerPaths.InGame(root, relative);
        Directory.CreateDirectory(Path.GetDirectoryName(path));
        File.WriteAllBytes(path, bytes);
        return new StockFile(name, expectedSha1, bytes.Length).Matches(root);
    }

    private static void LargeAddressIdentity()
    {
        byte[] original = SyntheticPe();
        string expected = Sha1(original);
        byte[] laa = (byte[])original.Clone(); laa[86] |= 0x20;
        Assert(StockMatches("FFX.exe", original, expected), "Exact root FFX.exe identity must match");
        Assert(StockMatches("FFX.exe", laa, expected), "Root FFX.exe may differ only by LARGE_ADDRESS_AWARE");

        byte[] dataEdit = (byte[])laa.Clone(); dataEdit[300] ^= 0x01;
        Assert(!StockMatches("FFX.exe", dataEdit, expected), "Executable data edit must be rejected");
        byte[] adjacentFlag = (byte[])original.Clone(); adjacentFlag[86] |= 0x10;
        Assert(!StockMatches("FFX.exe", adjacentFlag, expected), "Adjacent characteristics bit must be rejected");
        byte[] x64 = (byte[])laa.Clone(); x64[68] = 0x64; x64[69] = 0x86;
        Assert(!StockMatches("FFX.exe", x64, expected), "x64 executable must be rejected");
        byte[] truncated = new byte[200]; Array.Copy(laa, truncated, truncated.Length);
        Assert(!StockMatches("FFX.exe", truncated, expected), "Truncated PE must be rejected");
        Assert(!StockMatches("other.exe", laa, expected), "LAA identity must not apply to another root filename");
        Assert(!StockMatches("nested/FFX.exe", laa, expected), "LAA identity must not apply to nested FFX.exe");
    }

    private static void Discovery()
    {
        string steam = Path.Combine(fixture, "Steam");
        string library = Path.Combine(fixture, "Other drive & library [test]");
        Directory.CreateDirectory(Path.Combine(steam, "steamapps"));
        string game = Game(Path.Combine("Other drive & library [test]", "steamapps", "common", "Custom Name"));
        File.WriteAllText(Path.Combine(steam, "steamapps", "libraryfolders.vdf"),
            "\"libraryfolders\" { \"0\" { \"path\" \"" + library.Replace("\\", "\\\\") + "\" } \"1\" { \"path\" \"" + library.ToUpperInvariant().Replace("\\", "\\\\") + "\" } }");
        File.WriteAllText(Path.Combine(library, "steamapps", "appmanifest_359870.acf"), "\"AppState\" { \"installdir\" \"Custom Name\" }");
        List<string> found = SteamDiscovery.Find(new[] { steam, steam.ToUpperInvariant(), library, library + "\\", game });
        Assert(found.Count == 1 && String.Equals(found[0], game, StringComparison.OrdinalIgnoreCase), "Discovery must collapse case/trailing slash duplicates");
        Assert(SteamDiscovery.Find(new[] { Path.Combine(library, "steamapps") }).Count == 1, "steamapps Browse resolution");
        Assert(SteamDiscovery.Find(new[] { game }).Count == 1, "Direct game Browse resolution");
        File.Delete(Path.Combine(game, "FFX.exe"));
        Assert(SteamDiscovery.Find(new[] { library }).Count == 1, "Damaged game discovered through verified Steam location");
        File.WriteAllText(Path.Combine(library, "steamapps", "appmanifest_359870.acf"), "\"installdir\" \"..\"");
        Assert(SteamDiscovery.Find(new[] { library }).Count == 0, "Reject manifest traversal");
    }

    private static void Installation()
    {
        string game = Game("Installation");
        string config = Path.Combine(game, "FFX&X-2_LAUNCHER.exe.config");
        string original = "<configuration><runtime><gcConcurrent enabled=\"true\"/></runtime><appSettings><add key=\"Preserved\" value=\"yes\"/></appSettings></configuration>";
        File.WriteAllText(config, original);
        foreach (string name in new[] { "dinput8.dll", "frida-gadget.dll", "frida-gadget.config", "frida-gadget.config.dll", "ffx_sr.js", "OtherMod.dll", "FFXAccessibility.ini" })
            File.WriteAllText(Path.Combine(game, name), "original-" + name);
        string firstBackup = Engine().Install(game, delegate { });
        string secondBackup = Engine().Install(game, delegate { });
        PrismTestSupport.AssertPreserved(game);
        Assert(firstBackup != secondBackup && Directory.Exists(secondBackup), "Repeated install needs distinct backups");
        foreach (string name in InstallerEngine.PayloadNames)
            Assert(InstallerEngine.Hash(File.ReadAllBytes(Path.Combine(game, name))) == hashes[name], "Installed payload hash: " + name);
        foreach (string name in new[] { "frida-gadget.dll", "frida-gadget.config", "frida-gadget.config.dll", "ffx_sr.js" })
            Assert(!File.Exists(Path.Combine(game, name)) && File.ReadAllText(Path.Combine(firstBackup, name)) == "original-" + name, "Legacy backup/cleanup " + name);
        Assert(File.ReadAllText(config + ".ffx_sr_original") == original, "Original XML backup preserved across repeat");
        foreach (string name in new[] { "FFXAccessibility.ini" })
            Assert(File.ReadAllText(Path.Combine(game, name)) == "original-" + name, "Unrelated file/settings changed");
        Assert(!File.Exists(Path.Combine(game, "OtherMod.dll")) && File.Exists(Path.Combine(firstBackup, "OtherMod.dll")), "Other added DLL must be backed up and removed");
        var xml = new XmlDocument(); xml.Load(config);
        Assert(xml.SelectNodes("/configuration/runtime/appDomainManagerType").Count == 1, "Repeat duplicated manager");
        Assert(xml.SelectSingleNode("/configuration/appSettings/add").Attributes["value"].Value == "yes", "Unrelated XML changed");
        string receipt = Path.Combine(game, "PrismRuntime", "prism-install.json");
        byte[] receiptBytes = File.ReadAllBytes(receipt);
        var receiptTime = System.IO.File.GetLastWriteTimeUtc(InstallerPaths.Native(receipt));
        File.WriteAllText(receipt, "{}");
        var prereqBefore = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate { }); }, "Prism prerequisite");
        Unchanged(game, prereqBefore);
        File.WriteAllBytes(receipt, receiptBytes);
        System.IO.File.SetLastWriteTimeUtc(InstallerPaths.Native(receipt), receiptTime);
        File.Delete(receipt);
        prereqBefore = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate { }); }, "Prism prerequisite");
        Unchanged(game, prereqBefore);
        File.WriteAllBytes(receipt, receiptBytes);
        System.IO.File.SetLastWriteTimeUtc(InstallerPaths.Native(receipt), receiptTime);
        PrismTestSupport.AssertPreserved(game);

        var before = Snapshot(game);
        byte[] saved = payload["SDL3.dll"];
        payload["SDL3.dll"] = Encoding.UTF8.GetBytes("corrupt");
        Reject(delegate { Engine().Install(game, delegate { }); }, "checksum failed");
        payload["SDL3.dll"] = saved;
        Unchanged(game, before);
        string savedHash = hashes["SDL3.dll"]; hashes.Remove("SDL3.dll");
        Reject(delegate { Engine().Install(game, delegate { }); }, "Incomplete installer");
        hashes["SDL3.dll"] = savedHash;
        Unchanged(game, before);
        File.WriteAllText(config, "<configuration><runtime><appDomainManagerType value=\"Other.Manager\"/></runtime></configuration>");
        before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate { }); }, "another AppDomainManager");
        Unchanged(game, before);
        File.WriteAllText(config, "<configuration><runtime><appDomainManagerType value=\"FfxLauncher.ReaderDomainManager\"/><appDomainManagerType value=\"FfxLauncher.ReaderDomainManager\"/></runtime></configuration>");
        Reject(delegate { Engine().Install(game, delegate { }); }, "Duplicate launcher");
        File.WriteAllText(config, "<configuration><runtime/><runtime><appDomainManagerType value=\"Other.Manager\"/></runtime></configuration>");
        Reject(delegate { Engine().Install(game, delegate { }); }, "Duplicate launcher runtime");
        File.WriteAllText(config, "<!DOCTYPE configuration [<!ENTITY e SYSTEM 'file:///not-read'>]><configuration>&e;</configuration>");
        Reject(delegate { Engine().Install(game, delegate { }); }, "DTD");
        File.WriteAllText(config, original);
        File.WriteAllText(Path.Combine(game, "dinput8.dll"), "old-proxy");
        File.WriteAllText(Path.Combine(game, "frida-gadget.dll"), "old-gadget");
        File.Delete(Path.Combine(game, "SDL3.dll"));
        before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text == "Configuring launcher speech") throw new IOException("Synthetic failure after all payload copies");
        }); }, "Previous files were restored");
        Unchanged(game, before);
        foreach (string blocked in new[] { "SDL3.dll", "FFX&X-2_LAUNCHER.exe.config.ffx_sr_pending" })
        {
            FileStream lockFile = null;
            try
            {
                Reject(delegate { Engine().Install(game, delegate(string text) {
                    if (text == (blocked == "SDL3.dll" ? "Installing SDL3.dll" : "Configuring launcher speech"))
                        lockFile = new FileStream(Path.Combine(game, blocked), FileMode.CreateNew, FileAccess.ReadWrite, FileShare.Read | FileShare.Delete);
                }); }, "Previous files were restored");
            }
            finally { if (lockFile != null) lockFile.Dispose(); }
            Unchanged(game, before);
        }
        FileStream legacyLock = null;
        try
        {
            Reject(delegate { Engine().Install(game, delegate(string text) {
                if (text == "Removing old mod file: frida-gadget.dll")
                    legacyLock = new FileStream(Path.Combine(game, "frida-gadget.dll"), FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            }); }, "Previous files were restored");
        }
        finally { if (legacyLock != null) legacyLock.Dispose(); }
        Unchanged(game, before);
        string proxy = Path.Combine(game, "dinput8.dll");
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text == "Configuring launcher speech") {
                File.SetAttributes(proxy, FileAttributes.ReadOnly);
                throw new IOException("Synthetic locked restoration target");
            }
        }); }, "Some files could not be restored");
        File.SetAttributes(proxy, FileAttributes.Normal);
        bool originalRetained = false;
        foreach (string backedUp in Directory.GetFiles(Path.Combine(game, "FFX_SR_Install_Backups"), "dinput8.dll", SearchOption.AllDirectories))
            if (File.ReadAllText(backedUp) == "old-proxy") originalRetained = true;
        Assert(originalRetained, "Original proxy must remain recoverable when rollback is blocked");
        Directory.CreateDirectory(Path.Combine(game, "SDL3.dll"));
        Reject(delegate { Engine().Install(game, delegate { }); }, "Expected a file");
        Directory.Delete(Path.Combine(game, "SDL3.dll"));

        string fresh = Game("No configuration");
        Engine().Install(fresh, delegate { });
        Assert(File.Exists(Path.Combine(fresh, "FFX&X-2_LAUNCHER.exe.config.ffx_sr_original")), "Fresh configuration original backup missing");
    }

    private static void LongPaths()
    {
        string relative = Path.Combine("Long path & unicode \u00e9", new string('a', 72), new string('b', 72), new string('c', 72));
        string game = Game(relative);
        Assert(game.Length > 260, "Long-path game fixture must exceed the legacy limit");
        Assert(SteamDiscovery.Find(new[] { game }).Count == 1, "Direct game discovery supports long ordinary paths");
        Assert(SteamDiscovery.Find(new[] { game, InstallerPaths.Native(game) }).Count == 1, "Extended/ordinary game paths have one discovery identity");
        string added = Path.Combine("mods", new string('d', 72), new string('e', 72), "foreign.asi");
        Directory.CreateDirectory(Path.GetDirectoryName(Path.Combine(game, added)));
        File.WriteAllText(Path.Combine(game, added), "old nested mod");
        File.WriteAllText(Path.Combine(game, "FFXAccessibility.ini"), "long-path settings");
        File.WriteAllText(Path.Combine(game, "dinput8.dll"), "old proxy");
        string config = Path.Combine(game, "FFX&X-2_LAUNCHER.exe.config");
        File.WriteAllText(config, "<configuration><appSettings><add key=\"Kept\" value=\"yes\"/></appSettings></configuration>");
        string backup = Engine().Install(game, delegate { });
        Assert(Path.Combine(backup, added).Length > 500, "Nested backup exercises paths well beyond MAX_PATH");
        Assert(!File.Exists(Path.Combine(game, added)) && File.ReadAllText(Path.Combine(backup, added)) == "old nested mod", "Long nested module is backed up before cleanup");
        foreach (string name in InstallerEngine.PayloadNames)
            Assert(InstallerEngine.Hash(File.ReadAllBytes(Path.Combine(game, name))) == hashes[name], "Long-path payload hash: " + name);
        Assert(File.ReadAllText(Path.Combine(game, "FFXAccessibility.ini")) == "long-path settings", "Long-path settings preserved");
        Assert(File.ReadAllText(Path.Combine(backup, "dinput8.dll")) == "old proxy", "Long-path old proxy backup preserved");
        string repeat = Engine().Install(InstallerPaths.Native(game), delegate { });
        Assert(repeat != backup && Directory.Exists(repeat), "Long-path reinstall retains separate backups");
        Assert(!repeat.StartsWith(@"\\?\", StringComparison.Ordinal), "Setup reports ordinary paths even for extended-path input");
        File.WriteAllText(Path.Combine(game, added), "rollback nested mod");
        var before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string text) {
            if (text == "Configuring launcher speech") throw new IOException("long-path rollback fixture");
        }); }, "Previous files were restored");
        Unchanged(game, before);
        Assert(File.ReadAllText(Path.Combine(game, added)) == "rollback nested mod", "Long-path rollback restores removed nested module");
        Reject(delegate { InstallerPaths.InGame(game, "..\\outside.dll"); }, "Invalid relative installation path");
        string unc = "\\\\server\\share\\" + new string('u', 80) + "\\" + new string('v', 80) + "\\" + new string('w', 80);
        Assert(Path.GetFullPath(unc) == unc, "Long UNC paths normalize without accessing the network");
        Assert(InstallerPaths.Display(InstallerPaths.Native(unc)) == unc, "Extended UNC IO paths retain ordinary display paths");
        Assert(InstallerPaths.Native(InstallerPaths.Native(game)) == InstallerPaths.Native(game), "Extended IO conversion is idempotent");
        Assert(InstallerPaths.Display(InstallerPaths.Native(game)) == game, "Extended local IO paths retain ordinary display paths");
        Console.WriteLine("PASS: long-path install/reinstall/cleanup/rollback; game folder " + game.Length + " characters, nested backup " + Path.Combine(backup, added).Length + " characters.");
    }

    private static void Migration()
    {
        string game = Game("Migration");
        Directory.CreateDirectory(Path.Combine(game, "mods", "reader"));
        foreach (string name in new[] { "dxgi.dll", "version.DLL", "mods/reader/foreign.dll", "mods/reader/plugin.asi" })
            File.WriteAllText(InstallerPaths.InGame(game, name), "old-" + name);
        File.WriteAllText(Path.Combine(game, "mods", "reader", "settings.ini"), "preserve");
        File.WriteAllText(Path.Combine(game, "FFX&X-2_LAUNCHER.exe.config"), "<configuration><runtime><appDomainManagerAssembly value=\"ForeignReader, Version=1.0.0.0\"/><appDomainManagerType value=\"Foreign.Manager\"/><gcConcurrent enabled=\"true\"/></runtime></configuration>");
        File.WriteAllText(Path.Combine(game, "ForeignReader.dll"), "foreign reader");
        string backup = Engine().Install(game, delegate { });
        foreach (string name in new[] { "dxgi.dll", "version.DLL", "mods/reader/foreign.dll", "mods/reader/plugin.asi", "ForeignReader.dll" })
            Assert(!File.Exists(InstallerPaths.InGame(game, name)) && File.Exists(InstallerPaths.InGame(backup, name)), "Added module cleanup: " + name);
        Assert(File.ReadAllText(Path.Combine(game, "mods", "reader", "settings.ini")) == "preserve", "Mod settings preserved");
        Assert(File.Exists(Path.Combine(game, "magicFiles", "stock", "original.dll")), "Stock nested DLL preserved");
        Assert(File.ReadAllText(Path.Combine(backup, "INSTALL_REPORT.txt")).Contains("mods\\reader\\foreign.dll"), "Relative removal report");
        var oldPayload = new Dictionary<string, byte[]>(payload);
        var oldHashes = new Dictionary<string, string>(hashes);
        const string savedAssistance = "; Upgrade must preserve both On and Off\r\n[Accessibility]\r\noverdrive_assist=1\r\njecht_assist=0\r\nlightning_auto_dodge=1\r\n[Controller]\r\nrepeat=7\r\n";
        File.WriteAllText(Path.Combine(game, "FFXAccessibility.ini"), savedAssistance);
        foreach (string name in InstallerEngine.PayloadNames) { payload[name] = Encoding.UTF8.GetBytes("Version B " + name); hashes[name] = InstallerEngine.Hash(payload[name]); }
        var before = Snapshot(game);
        File.WriteAllText(Path.Combine(game, "mods", "reader", "second.dll"), "second reader");
        before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string message) { if (message == "Configuring launcher speech") throw new IOException("Upgrade failure"); }); }, "Previous files were restored");
        Unchanged(game, before);
        Engine().Install(game, delegate { });
        Assert(File.ReadAllText(Path.Combine(game, "FFXAccessibility.ini")) == savedAssistance, "A to B upgrade preserves explicit assistance choices and unrelated controls");
        foreach (string name in InstallerEngine.PayloadNames) Assert(InstallerEngine.Hash(File.ReadAllBytes(Path.Combine(game, name))) == hashes[name], "A to B upgrade: " + name);
        foreach (string name in InstallerEngine.PayloadNames) { payload[name] = oldPayload[name]; hashes[name] = oldHashes[name]; }
        foreach (string damaged in new[] { "FFX.exe", "FFX&X-2_LAUNCHER.exe", "magicFiles/stock/original.dll" }) {
            string target = InstallerPaths.InGame(game, damaged);
            byte[] bytes = File.ReadAllBytes(target);
            File.Delete(target); before = Snapshot(game);
            Reject(delegate { Engine().Install(game, delegate { }); }, "missing or modified: " + InstallerPaths.Relative(damaged)); Unchanged(game, before);
            File.WriteAllText(target, "patched"); before = Snapshot(game);
            Reject(delegate { Engine().Install(game, delegate { }); }, "missing or modified: " + InstallerPaths.Relative(damaged)); Unchanged(game, before);
            File.WriteAllBytes(target, bytes);
        }
        Reject(delegate { InstallerPaths.InGame(game, "../outside.dll"); }, "Invalid relative");
        Reject(delegate { InstallerPaths.InGame(game, "C:/outside.dll"); }, "Invalid relative");
        File.WriteAllText(Path.Combine(game, "late.dll"), "before"); before = Snapshot(game);
        Reject(delegate { Engine().Install(game, delegate(string message) {
            if (message == "Configuring launcher speech") File.WriteAllText(Path.Combine(game, "late.dll"), "changed during setup");
        }); }, "appeared during setup");
        Unchanged(game, before);
        string linked = Game("Linked folder refusal");
        string outside = Path.Combine(fixture, "linked-outside"); Directory.CreateDirectory(outside);
        File.WriteAllText(Path.Combine(outside, "outside.dll"), "outside preserved");
        using (var junction = Process.Start(new ProcessStartInfo("cmd.exe", "/d /c mklink /J \"" + Path.Combine(linked, "mods") + "\" \"" + outside + "\"") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true })) {
            junction.StandardOutput.ReadToEnd(); junction.WaitForExit(); Assert(junction.ExitCode == 0, "Synthetic junction fixture creation");
        }
        before = Snapshot(game);
        Reject(delegate { Engine().Install(linked, delegate { }); }, "linked file or subfolder");
        Assert(File.ReadAllText(Path.Combine(outside, "outside.dll")) == "outside preserved", "Linked destination untouched");
        Unchanged(game, before);
    }

    private static void ProcessRefusal()
    {
        string game = Game("Closed process refusal");
        string helper = Path.Combine(game, "FFX.exe");
        File.Copy(Assembly.GetExecutingAssembly().Location, helper, true);
        var before = Snapshot(game);
        using (Process process = Process.Start(new ProcessStartInfo(helper, "--synthetic-process") { UseShellExecute = false, CreateNoWindow = true }))
        {
            Reject(delegate { Engine().Install(game, delegate { }); }, "Close the game and launcher");
            Unchanged(game, before);
            process.WaitForExit();
        }
        InstallerEngine.RequireClosedProcesses();
    }

    private static void Controls(Control control, List<Control> all)
    {
        foreach (Control child in control.Controls) { all.Add(child); Controls(child, all); }
    }

    private static void AccessibleControls()
    {
        using (var window = new InstallerForm())
        {
            var all = new List<Control>(); Controls(window, all);
            foreach (string name in new[] { "Detected game installation", "Selected game folder", "Setup status", "Browse", "Install", "Cancel", "Check launch redirection", "Repair launch redirection", "Restore launch backup" })
            {
                Control found = all.Find(delegate(Control c) { return c.AccessibleName == name; });
                Assert(found != null && found.TabStop, "Missing keyboard-accessible named control: " + name);
                Assert(found.AccessibilityObject.Name == name, "Accessible control name mismatch: " + name);
            }
            Control statusControl = all.Find(delegate(Control c) { return c.AccessibleName == "Setup status"; });
            Assert(statusControl is TextBox && ((TextBox)statusControl).ReadOnly && ((TextBox)statusControl).Multiline,
                "Status must remain keyboard-readable standard text");
            Control combo = all.Find(delegate(Control c) { return c.AccessibleName == "Detected game installation"; });
            Assert(combo is ComboBox && ((ComboBox)combo).DropDownStyle == ComboBoxStyle.DropDownList, "Detected installations need ordinary Windows selection");
            Control install = all.Find(delegate(Control c) { return c.AccessibleName == "Install"; });
            Assert(!install.Enabled, "Install must be disabled before a game folder is selected");
            Assert(!all.Find(delegate(Control c) { return c.AccessibleName == "Repair launch redirection"; }).Enabled, "Repair must require selected installation");
            Assert(all.Find(delegate(Control c) { return c.AccessibleName == "Restore launch backup"; }) is Button, "Restore uses standard accessible button");
            Assert(window.AcceptButton == install && window.CancelButton != null, "Enter/Escape keyboard controls");
        }
    }

    private static void RegistryConfirmation() {
        var method = typeof(InstallerForm).GetMethod("RegistryConfirmation", BindingFlags.Static | BindingFlags.NonPublic);
        var entries = new List<LaunchRedirectRepair.Entry> { new LaunchRedirectRepair.Entry(Microsoft.Win32.RegistryView.Registry64, "FFX.exe", Microsoft.Win32.RegistryValueKind.String, "old-launcher.exe") };
        using (var dialog = (Form)method.Invoke(null, new object[] { "Registry repair", "This changes the Windows registry.", entries })) {
            var controls = new List<Control>(); Controls(dialog, controls);
            Assert(dialog.AcceptButton == null, "Enter cannot silently approve registry changes from review text");
            Assert(dialog.CancelButton != null && ((Button)dialog.CancelButton).DialogResult == DialogResult.Cancel, "Escape cancels registry changes");
            var text = (TextBox)controls.Find(delegate(Control c) { return c.AccessibleName == "Registry changes to review"; });
            Assert(text != null && text.ReadOnly && text.Multiline && text.TabStop, "Registry preview is keyboard-readable standard text");
            Assert(text.Text.Contains("This changes the Windows registry.") && text.Text.Contains(LaunchRedirects.KeyRoot) && text.Text.Contains("old-launcher.exe"), "Preview names full key, value and explicit registry warning");
            var yes = (Button)controls.Find(delegate(Control c) { return c.AccessibleName == "Confirm registry change"; });
            Assert(yes != null && yes.DialogResult == DialogResult.Yes && yes.TabStop, "Only explicit confirmation authorizes registry change");
        }
    }

    [STAThread]
    private static int Main(string[] args)
    {
        InstallerRuntime.Initialize();
        if (args.Length != 0 && args[0] == "--synthetic-process") { Thread.Sleep(2500); return 0; }
        try
        {
            if (args[0] == "--embedded") return EmbeddedInstallerTests.Run(args[1], args[2]);
            fixture = Path.GetFullPath(args[0]); Directory.CreateDirectory(fixture);
            foreach (string name in InstallerEngine.PayloadNames) {
                payload.Add(name, Encoding.UTF8.GetBytes("Synthetic payload " + name));
                hashes.Add(name, InstallerEngine.Hash(payload[name]));
            }
            SteamAppManifestTests.Run(fixture, Assert); LaunchRedirectRepairTests.Run(fixture, Assert); LargeAddressIdentity(); Discovery(); LongPaths(); Installation(); Migration(); ProcessRefusal(); AccessibleControls(); RegistryConfirmation();
            Console.WriteLine("PASS: " + checks + " synthetic installer assertions; discovery/case dedup, 7 payload hashes, Prism preflight/runtime preservation, XML/settings/repeat, legacy backups, refusals and rollback.");
            Console.WriteLine("Fixtures retained: " + fixture);
            return 0;
        }
        catch (Exception ex) { Console.Error.WriteLine(ex); return 1; }
    }
}
